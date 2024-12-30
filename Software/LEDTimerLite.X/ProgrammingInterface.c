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
 *      ZONE            00-6F (15-minute slots, -14:00 .. 14:00, 0:00 is 38)
 *  )
 *
 *  DATE(               Set the date
 *      YEAR[x4],       0000-FFFF (years from 1970)
 *      MONTH[x1],      0-B
 *      DAY[x2]         00-1F
 *  )
 *
 *  SCHOFF(             Turn off a schedule
 *      INDEX[x1],      0-7
 *  )
 *
 *  SCHINT(             Set interval schedule
 *      INDEX[x1],      0-7
 *      ON_TYPE[x1],    0-2 (0: time, 1: sunrise, 2: sunset)
 *      ON_OFFS[x1],    0-7 (15-minute slots, -60 .. 60, 0 is 3)
 *      ON_TIME_H[x2],  00-17
 *      ON_TIME_M[x2],  00-3C
 *      OFF_TYPE[x1],   0-2 (0: time, 1: sunrise, 2: sunset)
 *      OFF_OFFS[x1],   0-7 (15-minute slots, -60 .. 60, 0 is 3)
 *      OFF_TIME_H[x2], 00-17
 *      OFF_TIME_M[x2]  00-3C
 *  )
 *
 *  SCHSEG(             // Set segment schedule
 *      INDEX[x1],      0-7
 *      DATA_0[x2],     00-FF
 *      DATA_1[x2],     00-FF
 *      DATA_2[x2],     00-FF
 *      DATA_3[x2],     00-FF
 *      DATA_4[x2],     00-FF
 *      DATA_5[x2]      00-FF
 *  )
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

    PP_TIME,
    PP_DATE,
    PP_SCHOFF,
    PP_SCHINT,
    PP_SCHSEG
} PacketProcessor;

static struct Context {
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
} context = {
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

    switch (context.state) {
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
    context.bufferIndex = 0;
    context.state = PPS_RESET;
    context.selectedProcessor = PP_NONE;
}

typedef enum {
    HPT_NO_ERROR,
    HPT_UNKNOWN_TYPE
} HPT_Result;

static HPT_Result handlePacketType(const char* type)
{
    if (strncmp(type, "TIME", 5) == 0) {
        context.selectedProcessor = PP_TIME;
    } else if (strncmp(type, "DATE", 5) == 0) {
        context.selectedProcessor = PP_DATE;
    } else if (strncmp(type, "SCHOFF", 7) == 0) {
        context.selectedProcessor = PP_SCHOFF;
    } else if (strncmp(type, "SCHINT", 7) == 0) {
        context.selectedProcessor = PP_SCHINT;
    } else if (strncmp(type, "SCHSEG", 7) == 0) {
        context.selectedProcessor = PP_SCHSEG;
    } else {
        return HPT_UNKNOWN_TYPE;
    }

    context.fieldIndex = 0;

    return HPT_NO_ERROR;
}

static bool processPacketTypeOrWriteError()
{
    switch (handlePacketType(context.buffer)) {
        case HPT_NO_ERROR:
            return true;
        case HPT_UNKNOWN_TYPE:
            writeError(PI_ERR_UNKNOWN_PACKET_TYPE);
            break;
    }

    return false;
}

typedef enum {
    HFV_NO_ERROR,
    HFV_TOO_MANY_FIELDS,
    HFV_INVALID_FIELD_VALUE
} HFV_Result;

static HFV_Result handleFieldValue(const char* value)
{
    if (++context.fieldIndex > PACKET_HANDLER_MAX_FIELDS) {
        return HFV_TOO_MANY_FIELDS;
    }

    return HFV_NO_ERROR;
}

static bool processFieldValueOrWriteError()
{
    switch (handleFieldValue(context.buffer)) {
        case HFV_NO_ERROR:
            return true;
        case HFV_TOO_MANY_FIELDS:
            writeError(PI_ERR_TOO_MANY_FIELDS);
            break;
        case HFV_INVALID_FIELD_VALUE:
            writeError(PI_ERR_INVALID_FIELD_VALUE);
            break;
        default:
            break;
    }

    return false;
}

static void handleEndOfPacket()
{
    writeOK();
}

static void handleInputChar(const char c)
{
    if (!isAllowedToken(c)) {
        writeError(PI_ERR_INVALID_INPUT_CHAR);
        handleReset();
        return;
    }

    switch (context.state) {
        // RESET -(*)-> READ_PACKET_TYPE
        case PPS_RESET:
            if (c == '*') {
                handleReset();
                context.state = PPS_READ_PACKET_TYPE;
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
                if (context.bufferIndex == 0) {
                    if (context.state == PPS_READ_PACKET_TYPE) {
                        writeError(PI_ERR_MISSING_PACKET_TYPE);
                    } else {
                        writeError(PI_ERR_MISSING_FIELD_VALUE);
                    }
                    handleReset();
                } else {
                    bool handled = false;

                    if (context.state == PPS_READ_PACKET_TYPE) {
                        handled = processPacketTypeOrWriteError();
                    } else {
                        handled = processFieldValueOrWriteError();
                    }

                    if (handled) {
                        context.state = PPS_READ_FIELD;
                        context.bufferIndex = 0;
                    } else {
                        handleReset();
                    }
                }
            } else if (c == ';') {
                if (context.bufferIndex > 0) {
                    if (context.state == PPS_READ_PACKET_TYPE) {
                        processPacketTypeOrWriteError();
                    } else {
                        if (processFieldValueOrWriteError()) {
                            handleEndOfPacket();
                        }
                    }
                } else {
                    if (context.state == PPS_READ_PACKET_TYPE) {
                        writeError(PI_ERR_MISSING_PACKET_TYPE);
                    } else {
                        writeError(PI_ERR_MISSING_FIELD_VALUE);
                    }
                }
                handleReset();
            } else if (c == '*') {
                handleReset();
            } else {
                if (context.bufferIndex < sizeof(context.buffer) - 1) {
                    context.buffer[context.bufferIndex++] = c;
                    context.buffer[context.bufferIndex] = '\0';
                } else {
                    writeError(PI_ERR_BUFFER_FULL);
                    handleReset();
                }
            }
            break;
    }
}

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

    context.logQueue[context.logQueueIn] = entry;

    if (++context.logQueueIn >= LOG_QUEUE_SIZE) {
        context.logQueueIn = 0;
    }

    if (context.logQueueEntries == LOG_QUEUE_SIZE) {
        if (++context.logQueueOut >= LOG_QUEUE_SIZE) {
            context.logQueueOut = 0;
        }
    }

    if (context.logQueueEntries < LOG_QUEUE_SIZE) {
        ++context.logQueueEntries;
    }
}

void ProgrammingInterface_processInputChar(const char c)
{
    context.inputBuffer[context.inputBufferIn] = c;

    if (++context.inputBufferIn == INPUT_BUFFER_SIZE) {
        context.inputBufferIn = 0;
    }

    if (context.inputBufferCount == INPUT_BUFFER_SIZE) {
        if (++context.inputBufferOut >= INPUT_BUFFER_SIZE) {
            context.inputBufferOut = 0;
        }
    }

    if (context.inputBufferCount < INPUT_BUFFER_SIZE) {
        ++context.inputBufferCount;
    }
}

static void transmitLog(void)
{
    while (context.logQueueEntries > 0) {
        --context.logQueueEntries;

        LogEntry entry = context.logQueue[context.logQueueOut];

        if (++context.logQueueOut >= LOG_QUEUE_SIZE) {
            context.logQueueOut = 0;
        }

//        printf(";L%ld,%u:\r\n", entry.time, entry.event);
        printf(";L%ld,%s:\r\n", entry.time, EventNames[entry.event]);
    }
}

static void processInputBuffer(void)
{
    while (context.inputBufferCount > 0) {
        --context.inputBufferCount;

        handleInputChar(context.inputBuffer[context.inputBufferOut]);

        if (++context.inputBufferOut >= INPUT_BUFFER_SIZE) {
            context.inputBufferOut = 0;
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