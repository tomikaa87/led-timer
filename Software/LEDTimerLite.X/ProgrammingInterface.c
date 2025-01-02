/*
    This file is part of LEDTimer.

    LEDTimer is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LEDTimer is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with LEDTimer.  If not, see <http://www.gnu.org/licenses/>.

    Author: Tamas Karpati
    Created on 2024-12-16
*/

#include "ProgrammingInterface.h"

#include "Clock.h"
#include "OutputController.h"
#include "Settings.h"
#include "Types.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <xc.h>

#define LOG_QUEUE_SIZE 20
#define INPUT_BUFFER_SIZE 20

#define PACKET_HANDLER_BUFFER_SIZE 10
#define PACKET_HANDLER_MAX_FIELDS 10

static const char* EventNames[] = {
    "Startup",
    "LDOPowerUp",
    "LDOPowerDown",
    "ButtonPress",
    "EnterSleepMode",
    "LeaveSleepMode"
};

typedef struct {
    PI_LogEvent event;
    time_t time;
} LogEntry;


/*
 * Packet format
 *
 * *TYPE;
 * *TYPE:FIELD1:FIELDN;
 *
 * Tokens
 *  [*]: Reset parser, for synchronization purposes
 *  [;]: End of packet indicator
 *  [:]: Packet type and field delimiter
 *  [0-9A-Z]: Field value
 *
 * Request packets
 *  Legend:
 *      [x<len>]: [len] number of hex numbers
 *      PACKET_TYPE(FIELD1[type], ..., FIELDN[type]): *PACKET_TYPE:FIELD1:...:FIELDN;
 *      PACKET_TYPE(): *PACKET_TYPE;
 *
 *  TIME(               Set the time
 *      HOUR[x2],       00-17
 *      MIN[x2],        00-3C
 *      SEC[x2],        00-3C
 *      ZONE[x2]        00-6F (15-minute slots, -14:00 .. 14:00, 0:00 is 38)
 *  )
 *
 *  DATE(               Set the date
 *      YEAR[x4],       0000-FFFF (years from 1970)
 *      MONTH[x1],      0-B
 *      DAY[x2]         00-1F
 *  )
 *
 *  SCHSET(             // Set the scheduler type
 *      TYPE[x1]        0: interval, 1: segment, 2: off
 *  )
 *
 *  SCHINTEN(           Turn an interval schedule on or off
 *      INDEX[x1],      0-7
 *      STATE[x1]       0: off, 1: on
 *  )
 *
 *  SCHINT(             Set interval schedule
 *      INDEX[x1],      0-7
 *      ON_TYPE[x1],    0-2 (0: time, 1: sunrise, 2: sunset)
 *      ON_SUNOFFS[x1], 0-8 (15-minute slots, -60 .. 60, 0 is 4)
 *      ON_TIME_H[x2],  00-17
 *      ON_TIME_M[x2],  00-3C
 *      OFF_TYPE[x1],   0-2 (0: time, 1: sunrise, 2: sunset)
 *      OFF_SUNOFFS[x1],0-8 (15-minute slots, -60 .. 60, 0 is 4)
 *      OFF_TIME_H[x2], 00-17
 *      OFF_TIME_M[x2]  00-3C
 *  )
 *
 *  SCHSEG(             // Set segment schedule data
 *      DATA_0[x2],     00-FF
 *      DATA_1[x2],     00-FF
 *      DATA_2[x2],     00-FF
 *      DATA_3[x2],     00-FF
 *      DATA_4[x2],     00-FF
 *      DATA_5[x2]      00-FF
 *  )
 *
 *  OUTPUT(             // Set the output override state
 *      ON[x1]          0: off, 1: on
 *  )
 *
 *  SAVE()              // Save settings
 *
 * Response packets
 *
 *  OK()
 *  ERR(CODE[x2])       00-FF
 *
 */

typedef enum {
    PPS_RESET,
    PPS_READ_PACKET_TYPE,
    PPS_READ_FIELD
} PacketParserState;

typedef enum {
    PP_NONE,

    PP_ENUM_FIRST,

    PP_TIME = PP_ENUM_FIRST,
    PP_DATE,
    PP_SCHSET,
    PP_SCHINTEN,
    PP_SCHINT,
    PP_SCHSEG,
    PP_OUTPUT,
    PP_SAVE,

    PP_ENUM_MAX
} PacketProcessor;

typedef union {
    struct TimeArgs {
        uint8_t hour;
        uint8_t minute;
        uint8_t second;
        uint8_t timeZone;
    } time;

    struct DateArgs {
        uint16_t year;
        uint8_t month;
        uint8_t day;
    } date;

    struct ScheduleSetArgs {
        uint8_t type;
    } scheduleSet;

    struct ScheduleIntervalEnableArgs {
        uint8_t index;
        uint8_t state;
    } scheduleIntervalEnable;

    struct ScheduleIntervalArgs {
        uint8_t index;
        uint8_t onType;
        uint8_t onSunOffset;
        uint8_t onTimeH;
        uint8_t onTimeM;
        uint8_t offType;
        uint8_t offSunOffset;
        uint8_t offTimeH;
        uint8_t offTimeM;
    } scheduleInterval;

    struct ScheduleSegmentArgs {
        uint8_t data[6];
    } scheduleSegment;

    struct OutputArgs {
        uint8_t on;
    } output;
} ReceivedArguments;

static uint8_t FieldCounts[PP_ENUM_MAX - PP_ENUM_FIRST] = {
    4,      // PP_TIME
    3,      // PP_DATE
    1,      // PP_SCHSET
    2,      // PP_SCHINTEN
    9,      // PP_SCHINT
    6,      // PP_SCHSEG
    1,      // PP_OUTPUT
    0,      // PP_SAVE
};

static struct ProgrammingInterfaceContext {
    volatile LogEntry logQueue[LOG_QUEUE_SIZE];

    volatile uint8_t logQueueIn;
    volatile uint8_t logQueueOut;
    volatile uint8_t logQueueEntries;

    volatile uint8_t inputBuffer[INPUT_BUFFER_SIZE];
    volatile uint8_t inputBufferIn;
    volatile uint8_t inputBufferOut;
    volatile uint8_t inputBufferCount;

    PacketParserState state;
    char buffer[PACKET_HANDLER_BUFFER_SIZE];
    uint8_t bufferIndex;

    PacketProcessor selectedProcessor;
    uint8_t fieldIndex;
    ReceivedArguments receivedArguments;
} ProgrammingInterface_context = {
    .logQueueIn = 0,
    .logQueueOut = 0,
    .logQueueEntries = 0,
    .inputBuffer = {0,},
    .inputBufferIn = 0,
    .inputBufferOut = 0,
    .inputBufferCount = 0,
    .state = PPS_RESET,
    .buffer = {0,},
    .bufferIndex = 0,
    .selectedProcessor = PP_NONE,
    .fieldIndex = 0
};

void writeOK()
{
    printf("*OK;\r\n");
}

void writeError(const uint8_t code)
{
    printf("*ERR:%02X;\r\n", code);
}

bool isAllowedToken(const char c) {
    if (c == '*') {
        return true;
    }

    switch (ProgrammingInterface_context.state) {
        case PPS_READ_PACKET_TYPE:
        case PPS_READ_FIELD:
            if (c == ':' || c == ';' || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')) {
                return true;
            }
        break;

        default:
            break;
    }

    return false;
}

static void handleReset()
{
    ProgrammingInterface_context.bufferIndex = 0;
    ProgrammingInterface_context.state = PPS_RESET;
    ProgrammingInterface_context.selectedProcessor = PP_NONE;
}

typedef enum {
    HPT_NO_ERROR,
    HPT_UNKNOWN_TYPE
} HPT_Result;

static HPT_Result handlePacketType(const char* type)
{
    if (strncmp(type, "TIME", 5) == 0) {
        ProgrammingInterface_context.selectedProcessor = PP_TIME;
    } else if (strncmp(type, "DATE", 5) == 0) {
        ProgrammingInterface_context.selectedProcessor = PP_DATE;
    } else if (strncmp(type, "SCHSET", 7) == 0) {
        ProgrammingInterface_context.selectedProcessor = PP_SCHSET;
    } else if (strncmp(type, "SCHINTEN", 9) == 0) {
        ProgrammingInterface_context.selectedProcessor = PP_SCHINTEN;
    } else if (strncmp(type, "SCHINT", 7) == 0) {
        ProgrammingInterface_context.selectedProcessor = PP_SCHINT;
    } else if (strncmp(type, "SCHSEG", 7) == 0) {
        ProgrammingInterface_context.selectedProcessor = PP_SCHSEG;
    } else if (strncmp(type, "OUTPUT", 7) == 0) {
        ProgrammingInterface_context.selectedProcessor = PP_OUTPUT;
    } else if (strncmp(type, "SAVE", 5) == 0) {
        ProgrammingInterface_context.selectedProcessor = PP_SAVE;
    } else {
        return HPT_UNKNOWN_TYPE;
    }

    ProgrammingInterface_context.fieldIndex = 0;

    return HPT_NO_ERROR;
}

static bool processPacketTypeOrWriteError()
{
    switch (handlePacketType(ProgrammingInterface_context.buffer)) {
        case HPT_NO_ERROR:
            return true;
        case HPT_UNKNOWN_TYPE:
            writeError(PI_ERR_UNKNOWN_PACKET_TYPE);
            break;
    }

    return false;
}

static uint8_t hexCharToU8(const char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return 0xFF;
}

static bool fromHex(const char* s, const uint8_t maxLength, uint16_t* out)
{
    if (maxLength == 0 || maxLength > 4) {
        return false;
    }

    uint8_t nibbles = (uint8_t)strlen(s);
    if (nibbles == 0 || nibbles > maxLength) {
        return false;
    }

    uint16_t value = 0;

    for (uint8_t i = 0; i < nibbles; i++) {
        uint8_t nibble = hexCharToU8(s[nibbles - i - 1]);
        if (nibble == 0xFF) {
            return false;
        }

        value |= (uint16_t)nibble << (i * 4);
    }

    *out = value;

    return true;
}

static bool hexToU4(const char* s, uint8_t* out)
{
    uint8_t nibble = hexCharToU8(*s);
    if (nibble == 0xFF) {
        return false;
    }
    *out = nibble;

    return true;
}

static inline bool hexToU8(const char* s, uint8_t* out)
{
    return fromHex(s, 2, (uint16_t*)out);
}

static inline bool hexToU16(const char* s, uint16_t* out)
{
    return fromHex(s, 4, out);
}

#pragma region Command field processing

static bool handleTimeFieldValue(const char* value);
static bool handleDateFieldValue(const char* value);

static bool handleScheduleSetFieldValue(const char* value);
static bool handleScheduleIntervalEnableFieldValue(const char* value);
static bool handleScheduleIntervalFieldValue(const char* value);
static bool handleScheduleSegmentFieldValue(const char* value);

static bool handleOutputFieldValue(const char* value);

typedef enum {
    HFV_NO_ERROR,
    HFV_TOO_MANY_FIELDS,
    HFV_INVALID_FIELD_VALUE
} HFV_Result;

static HFV_Result handleFieldValue(const char* value)
{
    if ((ProgrammingInterface_context.fieldIndex + 1) >= PACKET_HANDLER_MAX_FIELDS) {
        return HFV_TOO_MANY_FIELDS;
    }

    switch (ProgrammingInterface_context.selectedProcessor) {
        case PP_TIME:
            if (!handleTimeFieldValue(value)) {
                return HFV_INVALID_FIELD_VALUE;
            }
            break;
        case PP_DATE:
            if (!handleDateFieldValue(value)) {
                return HFV_INVALID_FIELD_VALUE;
            }
            break;
        case PP_SCHSET:
            if (!handleScheduleSetFieldValue(value)) {
                return HFV_INVALID_FIELD_VALUE;
            }
            break;
        case PP_SCHINTEN:
            if (!handleScheduleIntervalEnableFieldValue(value)) {
                return HFV_INVALID_FIELD_VALUE;
            }
            break;
        case PP_SCHINT:
            if (!handleScheduleIntervalFieldValue(value)) {
                return HFV_INVALID_FIELD_VALUE;
            }
            break;
        case PP_SCHSEG:
            if (!handleScheduleSegmentFieldValue(value)) {
                return HFV_INVALID_FIELD_VALUE;
            }
            break;
        case PP_OUTPUT:
            if (!handleOutputFieldValue(value)) {
                return HFV_INVALID_FIELD_VALUE;
            }
            break;
        default:
            return HFV_INVALID_FIELD_VALUE;
    }

    ++ProgrammingInterface_context.fieldIndex;

    return HFV_NO_ERROR;
}

static bool processFieldValueOrWriteError()
{
    switch (handleFieldValue(ProgrammingInterface_context.buffer)) {
        case HFV_NO_ERROR:
            return true;
        case HFV_TOO_MANY_FIELDS:
            writeError(PI_ERR_FIELD_BUFFER_FULL);
            break;
        case HFV_INVALID_FIELD_VALUE:
            writeError(PI_ERR_INVALID_FIELD_VALUE);
            break;
        default:
            break;
    }

    return false;
}

#pragma endregion

#pragma region End-of-packet handling

static bool executeTimeCommand(void);
static bool executeDateCommand(void);
static bool executeScheduleSetCommand(void);
static bool executeScheduleIntervalEnableCommand(void);
static bool executeScheduleIntervalCommand(void);
static bool executeScheduleSegmentCommand(void);
static bool executeOutputCommand(void);
static bool executeSaveCommand(void);

static void handleEndOfPacket()
{
    if (ProgrammingInterface_context.selectedProcessor < PP_ENUM_FIRST || ProgrammingInterface_context.selectedProcessor >= PP_ENUM_MAX) {
        writeError(PI_ERR_INTERNAL_ERROR);
        return;
    }

    if (ProgrammingInterface_context.fieldIndex != FieldCounts[ProgrammingInterface_context.selectedProcessor - PP_ENUM_FIRST]) {
        writeError(PI_ERR_FIELD_COUNT_MISMATCH);
        return;
    }

    switch (ProgrammingInterface_context.selectedProcessor) {
        case PP_TIME:
            if (!executeTimeCommand()) {
                writeError(PI_ERR_COMMAND_EXECUTION_FAILED);
                return;
            }
            break;

        case PP_DATE:
            if (!executeDateCommand()) {
                writeError(PI_ERR_COMMAND_EXECUTION_FAILED);
                return;
            }
            break;

        case PP_SCHSET:
            if (!executeScheduleSetCommand()) {
                writeError(PI_ERR_COMMAND_EXECUTION_FAILED);
                return;
            }
            break;

        case PP_SCHINTEN:
            if (!executeScheduleIntervalEnableCommand()) {
                writeError(PI_ERR_COMMAND_EXECUTION_FAILED);
                return;
            }
            break;

        case PP_SCHINT:
            if (!executeScheduleIntervalCommand()) {
                writeError(PI_ERR_COMMAND_EXECUTION_FAILED);
                return;
            }
            break;

        case PP_SCHSEG:
            if (!executeScheduleSegmentCommand()) {
                writeError(PI_ERR_COMMAND_EXECUTION_FAILED);
                return;
            }
            break;

        case PP_OUTPUT:
            if (!executeOutputCommand()) {
                writeError(PI_ERR_COMMAND_EXECUTION_FAILED);
                return;
            }
            break;

        case PP_SAVE:
            if (!executeSaveCommand()) {
                writeError(PI_ERR_COMMAND_EXECUTION_FAILED);
                return;
            }
            break;

        default:
            writeError(PI_ERR_INTERNAL_ERROR);
            return;
    }

    writeOK();
}

#pragma endregion

#pragma region Input processing

static void handleInputChar(const char c)
{
    if (!isAllowedToken(c)) {
        writeError(PI_ERR_INVALID_INPUT_CHAR);
        handleReset();
        return;
    }

    switch (ProgrammingInterface_context.state) {
        // RESET -(*)-> READ_PACKET_TYPE
        case PPS_RESET:
            if (c == '*') {
                handleReset();
                ProgrammingInterface_context.state = PPS_READ_PACKET_TYPE;
            }
            break;

        // READ_PACKET_TYPE -(*)-> RESET
        // READ_PACKET_TYPE -(:, buf > 0)-> handlePacketType -> READ_FIELD
        // READ_PACKET_TYPE -(:)-> RESET
        // READ_PACKET_TYPE -(;, buf > 0)-> handlePacketType -> RESET
        // READ_PACKET_TYPE -(;)-> RESET

        // READ_FIELD -(*)-> RESET
        // READ_FIELD -(:, buf > 0)-> handleFieldValue -> READ_FIELD
        // READ_FIELD -(:)-> RESET
        // READ_FIELD -(;, buf > 0)-> handleFieldValue -> RESET
        // READ_FIELD -(;)-> RESET
        case PPS_READ_PACKET_TYPE:
        case PPS_READ_FIELD:
            if (c == ':') {
                if (ProgrammingInterface_context.bufferIndex == 0) {
                    if (ProgrammingInterface_context.state == PPS_READ_PACKET_TYPE) {
                        writeError(PI_ERR_MISSING_PACKET_TYPE);
                    } else {
                        writeError(PI_ERR_MISSING_FIELD_VALUE);
                    }
                    handleReset();
                } else {
                    bool handled = false;

                    if (ProgrammingInterface_context.state == PPS_READ_PACKET_TYPE) {
                        handled = processPacketTypeOrWriteError();
                    } else {
                        handled = processFieldValueOrWriteError();
                    }

                    if (handled) {
                        ProgrammingInterface_context.state = PPS_READ_FIELD;
                        ProgrammingInterface_context.bufferIndex = 0;
                    } else {
                        handleReset();
                    }
                }
            } else if (c == ';') {
                if (ProgrammingInterface_context.bufferIndex > 0) {
                    if (ProgrammingInterface_context.state == PPS_READ_PACKET_TYPE) {
                        if (processPacketTypeOrWriteError()) {
                            handleEndOfPacket();
                        }
                    } else {
                        if (processFieldValueOrWriteError()) {
                            handleEndOfPacket();
                        }
                    }
                } else {
                    if (ProgrammingInterface_context.state == PPS_READ_PACKET_TYPE) {
                        writeError(PI_ERR_MISSING_PACKET_TYPE);
                    } else {
                        writeError(PI_ERR_MISSING_FIELD_VALUE);
                    }
                }
                handleReset();
            } else if (c == '*') {
                handleReset();
            } else {
                if (ProgrammingInterface_context.bufferIndex < sizeof(ProgrammingInterface_context.buffer) - 1) {
                    ProgrammingInterface_context.buffer[ProgrammingInterface_context.bufferIndex++] = c;
                    ProgrammingInterface_context.buffer[ProgrammingInterface_context.bufferIndex] = '\0';
                } else {
                    writeError(PI_ERR_BUFFER_FULL);
                    handleReset();
                }
            }
            break;
    }
}

#pragma endregion

static void transmitLog(void);
static void processInputBuffer(void);

void ProgrammingInterface_init(void)
{

}

void ProgrammingInterface_runTasks(void)
{
    transmitLog();
    processInputBuffer();
}

void ProgrammingInterface_logEvent(const PI_LogEvent event)
{
    LogEntry entry = {
        .event = event,
        .time = Clock_getUtcEpoch()
    };

    ProgrammingInterface_context.logQueue[ProgrammingInterface_context.logQueueIn] = entry;

    if (++ProgrammingInterface_context.logQueueIn >= LOG_QUEUE_SIZE) {
        ProgrammingInterface_context.logQueueIn = 0;
    }

    if (ProgrammingInterface_context.logQueueEntries == LOG_QUEUE_SIZE) {
        if (++ProgrammingInterface_context.logQueueOut >= LOG_QUEUE_SIZE) {
            ProgrammingInterface_context.logQueueOut = 0;
        }
    }

    if (ProgrammingInterface_context.logQueueEntries < LOG_QUEUE_SIZE) {
        ++ProgrammingInterface_context.logQueueEntries;
    }
}

void ProgrammingInterface_processInputChar(const char c)
{
    ProgrammingInterface_context.inputBuffer[ProgrammingInterface_context.inputBufferIn] = c;

    if (++ProgrammingInterface_context.inputBufferIn == INPUT_BUFFER_SIZE) {
        ProgrammingInterface_context.inputBufferIn = 0;
    }

    if (ProgrammingInterface_context.inputBufferCount == INPUT_BUFFER_SIZE) {
        if (++ProgrammingInterface_context.inputBufferOut >= INPUT_BUFFER_SIZE) {
            ProgrammingInterface_context.inputBufferOut = 0;
        }
    }

    if (ProgrammingInterface_context.inputBufferCount < INPUT_BUFFER_SIZE) {
        ++ProgrammingInterface_context.inputBufferCount;
    }
}

static void transmitLog(void)
{
    while (ProgrammingInterface_context.logQueueEntries > 0) {
        --ProgrammingInterface_context.logQueueEntries;

        LogEntry entry = ProgrammingInterface_context.logQueue[ProgrammingInterface_context.logQueueOut];

        if (++ProgrammingInterface_context.logQueueOut >= LOG_QUEUE_SIZE) {
            ProgrammingInterface_context.logQueueOut = 0;
        }

//        printf(";L%ld,%u:\r\n", entry.time, entry.event);
        printf(";L%ld,%s:\r\n", entry.time, EventNames[entry.event]);
    }
}

static void processInputBuffer(void)
{
    while (ProgrammingInterface_context.inputBufferCount > 0) {
        --ProgrammingInterface_context.inputBufferCount;

        handleInputChar(ProgrammingInterface_context.inputBuffer[ProgrammingInterface_context.inputBufferOut]);

        if (++ProgrammingInterface_context.inputBufferOut >= INPUT_BUFFER_SIZE) {
            ProgrammingInterface_context.inputBufferOut = 0;
        }
    }
}

void putch(char txData)
{
    while (!TRMT) {
        continue;
    }

    TXREG1 = txData;
}

#pragma region Date/Time commands

static bool handleTimeFieldValue(const char* value)
{
    if (ProgrammingInterface_context.fieldIndex == 0) {
        if (
            hexToU8(value, &ProgrammingInterface_context.receivedArguments.time.hour)
            && ProgrammingInterface_context.receivedArguments.time.hour <= 23
        ) {
            return true;
        }
    } else if (ProgrammingInterface_context.fieldIndex == 1) {
        if (
            hexToU8(value, &ProgrammingInterface_context.receivedArguments.time.minute)
            && ProgrammingInterface_context.receivedArguments.time.minute <= 59
        ) {
            return true;
        }
    } else if (ProgrammingInterface_context.fieldIndex == 2) {
        if (
            hexToU8(value, &ProgrammingInterface_context.receivedArguments.time.second)
            && ProgrammingInterface_context.receivedArguments.time.second <= 59
        ) {
            return true;
        }
    } else if (ProgrammingInterface_context.fieldIndex == 3) {
        if (
            hexToU8(value, &ProgrammingInterface_context.receivedArguments.time.timeZone)
            && ProgrammingInterface_context.receivedArguments.time.timeZone <= 0x6F
        ) {
            return true;
        }
    }

    return false;
}

static bool handleDateFieldValue(const char* value)
{
    if (ProgrammingInterface_context.fieldIndex == 0) {
        if (
            hexToU16(value, &ProgrammingInterface_context.receivedArguments.date.year)
            && ProgrammingInterface_context.receivedArguments.date.year >= 2024
        ) {
            return true;
        }
    } else if (ProgrammingInterface_context.fieldIndex == 1) {
        if (
            hexToU4(value, &ProgrammingInterface_context.receivedArguments.date.month)
            && ProgrammingInterface_context.receivedArguments.date.month < 0x0C
        ) {
            return true;
        }
    } else if (ProgrammingInterface_context.fieldIndex == 2) {
        if (
            hexToU8(value, &ProgrammingInterface_context.receivedArguments.date.day)
            && ProgrammingInterface_context.receivedArguments.date.day < 0x1F
        ) {
            return true;
        }
    }

    return false;
}

static bool executeTimeCommand()
{
    Clock_setTime(
        ProgrammingInterface_context.receivedArguments.time.hour,
        ProgrammingInterface_context.receivedArguments.time.minute,
        ProgrammingInterface_context.receivedArguments.time.second
    );

    // TODO timezone

    return true;
}

static bool executeDateCommand()
{
    Clock_setDate(
        (uint8_t)(ProgrammingInterface_context.receivedArguments.date.year - 1970),
        ProgrammingInterface_context.receivedArguments.date.month + 1,
        ProgrammingInterface_context.receivedArguments.date.day + 1
    );

    return true;
}

#pragma endregion

#pragma region Scheduler commands

static bool handleScheduleSetFieldValue(const char* value)
{
    if (ProgrammingInterface_context.fieldIndex == 0) {
        if (
            hexToU4(value, &ProgrammingInterface_context.receivedArguments.scheduleSet.type)
            && ProgrammingInterface_context.receivedArguments.scheduleSet.type <= 2
        ) {
            return true;
        }
    }

    return false;
}

static bool handleScheduleIntervalEnableFieldValue(const char* value)
{
    if (ProgrammingInterface_context.fieldIndex == 0) {
        if (
            hexToU8(value, &ProgrammingInterface_context.receivedArguments.scheduleIntervalEnable.index)
            && ProgrammingInterface_context.receivedArguments.scheduleIntervalEnable.index <= 7
        ) {
            return true;
        }
    } else if (ProgrammingInterface_context.fieldIndex == 1) {
        if (
            hexToU4(value, &ProgrammingInterface_context.receivedArguments.scheduleIntervalEnable.state)
            && ProgrammingInterface_context.receivedArguments.scheduleIntervalEnable.state <= 1
        ) {
            return true;
        }
    }

    return false;
}

static bool handleScheduleIntervalFieldValue(const char* value)
{
    if (ProgrammingInterface_context.fieldIndex == 0) {
        if (
            hexToU4(value, &ProgrammingInterface_context.receivedArguments.scheduleInterval.index)
            && ProgrammingInterface_context.receivedArguments.scheduleInterval.index <= 7
        ) {
            return true;
        }
    } else if (ProgrammingInterface_context.fieldIndex == 1) {
        if (
            hexToU4(value, &ProgrammingInterface_context.receivedArguments.scheduleInterval.onType)
            && ProgrammingInterface_context.receivedArguments.scheduleInterval.onType <= 0x2
        ) {
            return true;
        }
    } else if (ProgrammingInterface_context.fieldIndex == 2) {
        if (
            hexToU4(value, &ProgrammingInterface_context.receivedArguments.scheduleInterval.onSunOffset)
            && ProgrammingInterface_context.receivedArguments.scheduleInterval.onSunOffset <= 0x8
        ) {
            return true;
        }
    } else if (ProgrammingInterface_context.fieldIndex == 3) {
        if (
            hexToU8(value, &ProgrammingInterface_context.receivedArguments.scheduleInterval.onTimeH)
            && ProgrammingInterface_context.receivedArguments.scheduleInterval.onTimeH <= 0x17
        ) {
            return true;
        }
    } else if (ProgrammingInterface_context.fieldIndex == 4) {
        if (
            hexToU8(value, &ProgrammingInterface_context.receivedArguments.scheduleInterval.onTimeM)
            && ProgrammingInterface_context.receivedArguments.scheduleInterval.onTimeM <= 0x3C
        ) {
            return true;
        }
    } else if (ProgrammingInterface_context.fieldIndex == 5) {
        if (
            hexToU4(value, &ProgrammingInterface_context.receivedArguments.scheduleInterval.offType)
            && ProgrammingInterface_context.receivedArguments.scheduleInterval.offType <= 0x2
        ) {
            return true;
        }
    } else if (ProgrammingInterface_context.fieldIndex == 6) {
        if (
            hexToU4(value, &ProgrammingInterface_context.receivedArguments.scheduleInterval.offSunOffset)
            && ProgrammingInterface_context.receivedArguments.scheduleInterval.offSunOffset <= 0x8
        ) {
            return true;
        }
    } else if (ProgrammingInterface_context.fieldIndex == 7) {
        if (
            hexToU8(value, &ProgrammingInterface_context.receivedArguments.scheduleInterval.offTimeH)
            && ProgrammingInterface_context.receivedArguments.scheduleInterval.offTimeH <= 0x17
        ) {
            return true;
        }
    } else if (ProgrammingInterface_context.fieldIndex == 8) {
        if (
            hexToU8(value, &ProgrammingInterface_context.receivedArguments.scheduleInterval.offTimeM)
            && ProgrammingInterface_context.receivedArguments.scheduleInterval.offTimeM <= 0x3C
        ) {
            return true;
        }
    }

    return false;
}

static bool handleScheduleSegmentFieldValue(const char* value)
{
    if (ProgrammingInterface_context.fieldIndex >= 0 && ProgrammingInterface_context.fieldIndex <= 6) {
        if (hexToU8(value, &ProgrammingInterface_context.receivedArguments.scheduleSegment.data[ProgrammingInterface_context.fieldIndex - 1])) {
            return true;
        }
    }

    return false;
}

static bool executeScheduleSetCommand()
{
    Settings_data.scheduler.type = ProgrammingInterface_context.receivedArguments.scheduleSet.type;

    return true;
}

static bool executeScheduleIntervalEnableCommand()
{
    Settings_data.scheduler.intervals[ProgrammingInterface_context.receivedArguments.scheduleIntervalEnable.index].active
        = ProgrammingInterface_context.receivedArguments.scheduleIntervalEnable.state;

    return true;
}

static bool executeScheduleIntervalCommand()
{
    struct IntervalScheduler* sch =
        &Settings_data.scheduler.intervals[ProgrammingInterface_context.receivedArguments.scheduleInterval.index];

    sch->onSwitch.type = ProgrammingInterface_context.receivedArguments.scheduleInterval.onType;
    sch->onSwitch.sunOffset = ((int8_t)ProgrammingInterface_context.receivedArguments.scheduleInterval.onSunOffset - 4) * 15;
    sch->onSwitch.timeHour = ProgrammingInterface_context.receivedArguments.scheduleInterval.onTimeH;
    sch->onSwitch.timeMinute = ProgrammingInterface_context.receivedArguments.scheduleInterval.onTimeM;

    sch->offSwitch.type = ProgrammingInterface_context.receivedArguments.scheduleInterval.offType;
    sch->offSwitch.sunOffset = ((int8_t)ProgrammingInterface_context.receivedArguments.scheduleInterval.offSunOffset - 4) * 15;
    sch->offSwitch.timeHour = ProgrammingInterface_context.receivedArguments.scheduleInterval.offTimeH;
    sch->offSwitch.timeMinute = ProgrammingInterface_context.receivedArguments.scheduleInterval.offTimeM;

    return true;
}

static bool executeScheduleSegmentCommand()
{
    memcpy(
        Settings_data.scheduler.segmentData,
        ProgrammingInterface_context.receivedArguments.scheduleSegment.data,
        sizeof(ScheduleSegmentData)
    );

    return true;
}

#pragma endregion

#pragma region Output control commands

static bool handleOutputFieldValue(const char* value)
{
    if (ProgrammingInterface_context.fieldIndex == 0) {
        if (
            hexToU4(value, &ProgrammingInterface_context.receivedArguments.output.on)
            && ProgrammingInterface_context.receivedArguments.output.on <= 1
        ) {
            return true;
        }
    }

    return false;
}

static bool executeOutputCommand()
{
    OutputController_setOverrideState(ProgrammingInterface_context.receivedArguments.output.on);
    return true;
}

#pragma endregion

#pragma region Settings control commands

static bool executeSaveCommand()
{
    Settings_save();

    return true;
}

#pragma endregion