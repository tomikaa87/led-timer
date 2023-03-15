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
#include "SunsetSunrise.h"
#include "System.h"
#include "Types.h"

#include "mcc_generated_files/pwm5.h"

#if DEBUG_ENABLE
#include "UI.h"
#endif

#include <stdbool.h>
#include <stdio.h>

static struct OutputControllerContext
{
    uint8_t outputOverride : 1;
    uint8_t prevOutputState : 1;
    uint8_t forceOutputStateUpdate : 1;
    uint8_t switchedOnBySchedule : 1;
    uint8_t prevStateFromSchedule : 1;
} context = {
    .outputOverride = 0,
    .prevOutputState = 0,
    .forceOutputStateUpdate = 0,
    .switchedOnBySchedule = 0,
    .prevStateFromSchedule = 0
};

uint16_t calculateSunEventTime(const uint16_t eventTime, const int8_t offset) {
    int16_t t = (int16_t)(eventTime) + (int16_t)(offset);

    if (t < 0) {
        return (uint16_t)(1440 + t);
    }

    if (t > 1440) {
        return (uint16_t)(t - 1440);
    }

    return (uint16_t)t;
}

uint16_t calculateSwitchTime(struct IntervalSwitch* sw)
{
    switch (sw->type) {
        case Settings_IntervalSwitchType_Time:
            return sw->timeHour * 60 + sw->timeMinute;

        case Settings_IntervalSwitchType_Sunrise:
            return calculateSunEventTime(SunriseSunset_getSunrise(), sw->sunOffset);

        case Settings_IntervaSwitchType_Sunset:
            return calculateSunEventTime(SunriseSunset_getSunset(), sw->sunOffset);
    }

    return 0;
}

static inline bool isSwitchedOnBySchedule()
{
    switch (Settings_data.scheduler.type) {
        case Settings_SchedulerType_Interval: {
            uint16_t currentTime = Clock_getMinutesSinceMidnight();

            for (uint8_t i = 0; i < Config_Settings_IntervalScheduleCount; ++i) {
                if (!Settings_data.scheduler.intervals[i].active) {
                    continue;
                }

                uint16_t onTime = calculateSwitchTime(
                    &Settings_data.scheduler.intervals[i].onSwitch
                );

                uint16_t offTime = calculateSwitchTime(
                    &Settings_data.scheduler.intervals[i].offSwitch
                );

                if (onTime <= offTime) {
                    if (currentTime >= onTime && currentTime < offTime) {
                        return true;
                    }
                } else {
                    if (currentTime >= onTime || currentTime < offTime) {
                        return true;
                    }
                }
            }

            return false;
        }

        case Settings_SchedulerType_Segment: {
            uint8_t segmentIndex = Types_calculateScheduleSegmentIndex(
                Clock_getMinutesSinceMidnight()
            );

            return Types_getScheduleSegmentBit(
                Settings_data.scheduler.segmentData,
                segmentIndex
            );
        }

        default:
            break;
    }

    return false;
}

/*
    A = switchedOnBySchedule
    B = runningFromBattery
    C = outputOverride
    output: F = A'B'C + AB'C'

    A	B	C	F
    0	0	0	0
    0	0	1	1
    0	1	0	0
    0	1	1	0
    1	0	0	1
    1	0	1	0
    1	1	0	0
    1	1	1	0
 */
static bool calculateOutputState(
    const bool switchedOnBySchedule,
    const bool runningFromBattery,
    const bool outputOverride
) {
    return
        (!switchedOnBySchedule && !runningFromBattery && outputOverride)
        || (switchedOnBySchedule && !runningFromBattery && !outputOverride);
}

/*
    A = switchedOnBySchedule
    B = prevStateFromSchedule
    C = outputOverride
    newOverride: F = A'B'C + ABC

    A	B	C	F
    0	0	0	0
    0	0	1	1
    0	1	0	0
    0	1	1	0
    1	0	0	0
    1	0	1	0
    1	1	0	0
    1	1	1	1
 */
static bool calculateOverrideState(
    const bool switchedOnBySchedule,
    const bool prevStateFromSchedule,
    const bool outputOverride
) {
    return
        (!switchedOnBySchedule && !prevStateFromSchedule && outputOverride)
        || (switchedOnBySchedule && prevStateFromSchedule && outputOverride);
}

/*
    A = output
    B = prevOutput
    C = forceUpdate
    update: F = A'B + AB' + C

    A	B	C	F
    0	0	0	0
    0	0	1	1
    0	1	0	1
    0	1	1	1
    1	0	0	1
    1	0	1	1
    1	1	0	0
    1	1	1	1
 */
static bool calculateOutputUpdateState(
    const bool outputState,
    const bool prevOutputState,
    const bool forceUpdate
) {
    return
        (!outputState && prevOutputState)
        || (outputState && !prevOutputState)
        || forceUpdate;
}

void OutputController_toggle()
{
#if DEBUG_ENABLE_PRINT
    puts("OC:toggle");
#endif

    context.outputOverride = !context.outputOverride;
    OutputController_updateState();
}

OutputController_TaskResult OutputController_task()
{
    context.switchedOnBySchedule = isSwitchedOnBySchedule();

    context.outputOverride = calculateOverrideState(
        context.switchedOnBySchedule,
        context.prevStateFromSchedule,
        context.outputOverride
    );

    bool outputState = calculateOutputState(
        context.switchedOnBySchedule,
        System_isRunningFromBackupBattery(),
        context.outputOverride
    );

    bool updateOutput = calculateOutputUpdateState(
        outputState,
        context.prevOutputState,
        context.forceOutputStateUpdate
    );

#if DEBUG_ENABLE
    _DebugState.oc_switchedOnBySchedule = context.switchedOnBySchedule;
    _DebugState.oc_prevStateFromSchedule = context.prevStateFromSchedule;
    _DebugState.oc_outputOverride = context.outputOverride;
    _DebugState.oc_outputState = outputState;
    _DebugState.oc_forceUpdate = context.forceOutputStateUpdate;
    UI_updateDebugDisplay();
#endif

    context.prevOutputState = outputState;
    context.prevStateFromSchedule = context.switchedOnBySchedule;
    context.forceOutputStateUpdate = 0;

    if (updateOutput) {
#if DEBUG_ENABLE_PRINT
        puts(outputState ? "OC:outputOn" : "OC:outputOff");
#endif
        PWM5_LoadDutyValue(
            outputState
                ? Settings_data.output.brightness
                : 0
        );

        return OutputController_TaskResult_OutputStateChanged;
    }

    return OutputController_TaskResult_StateUnchanged;
}

/*
    A = switchedOnBySchedule
    B = outputOverride
    outputStateOnExtPwr: F = A'B + AB'

    A	B	F
    0	0	0
    0	1	1
    1	0	1
    1	1	0
 */
inline bool OutputController_outputEnableTargetState()
{
    return
        (!context.switchedOnBySchedule && context.outputOverride)
        || (context.switchedOnBySchedule && !context.outputOverride);
}

inline bool OutputController_isOutputEnabled()
{
    return context.prevOutputState;
}

void OutputController_updateState()
{
#if DEBUG_ENABLE_PRINT
    printf("OC:forceUpdate");
#endif

    context.forceOutputStateUpdate = 1;
}