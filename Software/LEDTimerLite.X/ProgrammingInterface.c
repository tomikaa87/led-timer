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

#define LOG_QUEUE_SIZE 10

typedef struct {
    PI_LogEvent event;
    time_t time;
} LogEntry;

static struct Context {
    volatile LogEntry logQueue[LOG_QUEUE_SIZE];

    volatile uint8_t logQueueIn;
    volatile uint8_t logQueueOut;
    volatile uint8_t logQueueEntries;
} context = {
    .logQueueIn = 0,
    .logQueueOut = 0,
    .logQueueEntries = 0
};

static void transmitLog(void);

void ProgrammingInterface_init(void)
{

}

void ProgrammingInterface_task(void)
{
    transmitLog();
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

}

static void transmitLog(void)
{
    while (context.logQueueEntries > 0) {
        --context.logQueueEntries;

        LogEntry entry = context.logQueue[context.logQueueOut];

        if (++context.logQueueOut >= LOG_QUEUE_SIZE) {
            context.logQueueOut = 0;
        }

        printf(";L%ld,%u:\r\n", entry.time, entry.event);
    }
}

void putch(char txData)
{
    while (!TRMT) {
        continue;
    }

    TXREG1 = txData;
}