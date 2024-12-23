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

#include <stdio.h>

#include <xc.h>

#define LOG_QUEUE_SIZE 20
#define INPUT_BUFFER_SIZE 20

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
 *  [;]: Packet type delimiter, end of packet indicator
 *  [:]: Packet field delimiter
 *  [0-9A-Z]: Field value
 *
 * Packets
 *  Legend:
 *      [x<len>]: [len] number of hex numbers
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
 */

typedef enum {
    PPS_RESET,
    PPS_READ_PACKET_TYPE,
    PPS_READ_FIELD
} PacketParserState;

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
    char buffer[11];
    uint8_t bufferIndex;
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
};

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

}

static void handlePacketType(const char* type)
{
    printf("handlePacketType: %s\r\n", type);
}

static void handleFieldValue(const char* value)
{
    printf("handleFieldValue: %s\r\n", value);
}

static void handleEndOfPacket()
{

}

static void handleInputChar(const char c)
{
    if (!isAllowedToken(c)) {
        context.state = PPS_RESET;
        return;
    }

    switch (context.state) {
        // RESET -(*)-> READ_PACKET_TYPE
        case PPS_RESET:
            if (c == '*') {
                context.state = PPS_READ_PACKET_TYPE;
                context.bufferIndex = 0;
                handleReset();
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
                    context.state = PPS_RESET;
                    handleReset();
                } else {
                    if (context.state == PPS_READ_PACKET_TYPE) {
                        handlePacketType(context.buffer);
                    } else {
                        handleFieldValue(context.buffer);
                    }
                    context.state = PPS_READ_FIELD;
                    context.bufferIndex = 0;
                }
            } else if (c == ';') {
                if (context.bufferIndex > 0) {
                    if (context.state == PPS_READ_PACKET_TYPE) {
                        handlePacketType(context.buffer);
                    } else {
                        handleFieldValue(context.buffer);
                    }
                }
                handleEndOfPacket();
                context.state = PPS_RESET;
                handleReset();
            } else if (c == '*') {
                context.state = PPS_RESET;
                handleReset();
            } else if (context.bufferIndex < sizeof(context.buffer) - 1) {
                context.buffer[context.bufferIndex++] = c;
                context.buffer[context.bufferIndex] = '\0';
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