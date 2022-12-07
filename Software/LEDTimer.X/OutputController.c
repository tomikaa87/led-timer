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
    Created on 2022-12-06
*/

#include "Clock.h"
#include "OutputController.h"
#include "Settings.h"
#include "System.h"
#include "Types.h"

#include "mcc_generated_files/epwm2.h"

#include <stdbool.h>
#include <stdio.h>

static struct OutputControllerContext
{
    bool outputEnabled;
    bool outputShouldBeEnabled;
    uint8_t brightness;
} context = {
    .outputEnabled = false,
    .outputShouldBeEnabled = false,
    .brightness = 255
};

static inline bool getStateFromSchedule()
{
    uint8_t segmentIndex = Types_calculateScheduleSegmentIndex(
        Clock_getMinutesSinceMidnight()
    );

    uint8_t bitIndex = segmentIndex & 0b111;
    uint8_t byteIndex = segmentIndex >> 3;

    return Settings_data.scheduler.data[byteIndex] & (1 << bitIndex);
}

static inline void updateOutputState()
{
    printf("OC:output=%u\r\n", context.outputEnabled ? context.brightness : 0);

    EPWM2_LoadDutyValue(
        context.outputEnabled
            ? context.brightness
            : 0
    );
}

void OutputController_toggle()
{
    puts("OC:toggle");

    context.outputEnabled = !context.outputEnabled;
    updateOutputState();
}

void OutputController_task()
{
    bool shouldEnableOutput =
        !System_isRunningFromBackupBattery()
        && getStateFromSchedule();

    if (context.outputShouldBeEnabled != shouldEnableOutput) {
        context.outputShouldBeEnabled = shouldEnableOutput;
        context.outputEnabled = shouldEnableOutput;

        puts(shouldEnableOutput ? "OC:outputOn" : "OC:outputOff");

        updateOutputState();
    }
}

inline bool OutputController_isOutputEnabled()
{
    return context.outputEnabled;
}