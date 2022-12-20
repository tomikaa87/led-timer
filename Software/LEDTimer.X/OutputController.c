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
    Clock_Ticks lastOutputStateUpdateTicks;
} context = {
    .outputEnabled = false,
    .outputShouldBeEnabled = false,
    .lastOutputStateUpdateTicks = 0
};

static inline bool getStateFromSchedule()
{
    uint8_t segmentIndex = Types_calculateScheduleSegmentIndex(
        Clock_getMinutesSinceMidnight()
    );

    return Types_getScheduleSegmentBit(
        Settings_data.scheduler.data,
        segmentIndex
    );
}

void OutputController_toggle()
{
//    puts("OC:toggle");

    context.outputEnabled = !context.outputEnabled;
    OutputController_updateState();
}

void OutputController_task()
{
    bool shouldEnableOutput = getStateFromSchedule();
    bool updateOutputState = false;

    // Update the output state on schedule changes
    if (context.outputShouldBeEnabled != shouldEnableOutput) {
        context.outputShouldBeEnabled = shouldEnableOutput;
        context.outputEnabled = shouldEnableOutput;

//        puts(shouldEnableOutput ? "OC:outputOn" : "OC:outputOff");

        updateOutputState = true;
    }

    // TODO check why it this needed
    // Update the output states periodically
    if (Clock_getElapsedFastTicks(context.lastOutputStateUpdateTicks) >= 50) {
        context.lastOutputStateUpdateTicks = Clock_getFastTicks();
        updateOutputState = true;
    }

    if (updateOutputState) {
        OutputController_updateState();
    }
}

inline bool OutputController_isOutputEnabled()
{
    return context.outputEnabled;
}

void OutputController_updateState()
{
#if 0
    printf("OC:output=%u\r\n", context.outputEnabled ? context.brightness : 0);
#endif

    EPWM2_LoadDutyValue(
        (context.outputEnabled && !System_isRunningFromBackupBattery())
            ? Settings_data.output.brightness
            : 0
    );
}