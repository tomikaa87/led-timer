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

static struct OutputControllerContext
{
    uint8_t outputOverride : 1;
    uint8_t prevOutputState : 1;
    uint8_t forceOutputStateUpdate : 1;
    uint8_t switchedOnBySchedule : 1;
    uint8_t prevStateFromSchedule : 1;
    uint8_t rampingUp : 1;
    uint8_t active : 1;
    uint8_t : 1;

    uint8_t rampingCurrentBrightness;
    Clock_Ticks rampingUpStepTime;
    Clock_Ticks rampingStepTimer;

    Clock_Ticks rampingUpTaskTimer;
} context = {
    .outputOverride = 0,
    .prevOutputState = 0,
    .forceOutputStateUpdate = 0,
    .switchedOnBySchedule = 0,
    .prevStateFromSchedule = 0,
    .rampingUp = 0,
    .active = 1,
    .rampingCurrentBrightness = 0,
    .rampingUpStepTime = 0,
    .rampingStepTimer = 0,
    .rampingUpTaskTimer = 0
};

static void startRampingUp(void);
static void stopRampingUp(void);
static void rampingUpTask(void);

Clock_Time calculateSunEventTime(const Clock_Time eventTime, const int8_t offset) {
    int16_t t = (int16_t)(eventTime) + (int16_t)(offset);

    if (t < 0) {
        return (Clock_Time)(1440 + t);
    }

    if (t > 1440) {
        return (Clock_Time)(t - 1440);
    }

    return (Clock_Time)t;
}

Clock_Time OutputController_calculateSwitchTime(struct IntervalSwitch* sw)
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
            Clock_Time currentTime = Clock_getMinutesSinceMidnight();

            for (uint8_t i = 0; i < Config_Settings_IntervalScheduleCount; ++i) {
                if (!Settings_data.scheduler.intervals[i].active) {
                    continue;
                }

                Clock_Time onTime = OutputController_calculateSwitchTime(
                    &Settings_data.scheduler.intervals[i].onSwitch
                );

                Clock_Time offTime = OutputController_calculateSwitchTime(
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
#if !NIGHTLIGHT_TIMER
            uint8_t segmentIndex = Types_calculateScheduleSegmentIndex(
                Clock_getMinutesSinceMidnight()
            );

            return Types_getScheduleSegmentBit(
                Settings_data.scheduler.segmentData,
                segmentIndex
            );
#else
            break;
#endif
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
    if (!context.active) {
        return OutputController_TaskResult_StateUnchanged;
    }

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
        PWM5_LoadDutyValue(0);

        if (outputState) {
            startRampingUp();
        } else {
            stopRampingUp();
        }

        return OutputController_TaskResult_OutputStateChanged;
    }

    if (context.rampingUp) {
        if (Clock_getTicks() != context.rampingUpTaskTimer) {
            context.rampingUpTaskTimer = Clock_getTicks();

            rampingUpTask();
        }
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

void OutputController_activate()
{
    context.active = 1;
}

void OutputController_deactivate()
{
    context.active = 0;
}

typedef struct {
    Clock_Time on;
    Clock_Time off;
} Schedule;

static Clock_Time calcOffTime(const Schedule s)
{
    if (s.on <= s.off) {
        return s.off;
    } else {
        return s.off + 1440;
    }
}

bool OutputController_getNextTransition(
    const Clock_Time time,
    int8_t* const index,
    bool* const on
) {
    if (!index || !on) {
        return false;
    }

    Schedule schedules[Config_Settings_IntervalScheduleCount];

    // Gather all the active schedules
    int8_t scheduleCount = 0;
    for (int8_t i = 0; i < Config_Settings_IntervalScheduleCount; ++i) {
        if (Settings_data.scheduler.intervals[i].active) {
            schedules[scheduleCount].on = OutputController_calculateSwitchTime(
                &Settings_data.scheduler.intervals[i].onSwitch
            );

            schedules[scheduleCount].off = OutputController_calculateSwitchTime(
                &Settings_data.scheduler.intervals[i].offSwitch
            );

            ++scheduleCount;
        }
    }

    if (scheduleCount == 0) {
        return false;
    }

    // Calculate the state for the specified time
    bool currentlyOn = false;
    for (int8_t i = 0; i < scheduleCount; ++i) {
        Schedule s = schedules[i];
        if (
            (s.on <= s.off && time >= s.on && time < s.off)
            || (s.off < s.on && (time >= s.on || time < s.off))
        ) {
            currentlyOn = true;
            break;
        }
    }

    // Only one schedule, nothing to calculate
    if (scheduleCount == 1) {
        *on = !currentlyOn;
        *index = 0;
        return true;
    }

    // Sort the schedules by ON time using an index array
    int8_t sortedIndices[Config_Settings_IntervalScheduleCount];
    for (int8_t i = 0; i < scheduleCount; ++i) {
        sortedIndices[i] = i;
    }
    for (int8_t i = 1; i < scheduleCount; ++i) {
        int8_t tmp = sortedIndices[i];
        int8_t j = i - 1;
        while (j >= 0 && schedules[sortedIndices[j]].on > schedules[tmp].on) {
            sortedIndices[j + 1] = sortedIndices[j];
            --j;
        }
        sortedIndices[j + 1] = tmp;
    }

    typedef struct {
        int8_t onIdx;
        int8_t offIdx;
    } StackElem;

    // Merge overlapping intervals, results will be on the stack
    StackElem stack[Config_Settings_IntervalScheduleCount];
    int8_t stackIndex = 0;
    stack[0].onIdx = sortedIndices[0];
    stack[0].offIdx = sortedIndices[0];
    for (int8_t i = 1; i < scheduleCount; ++i) {
        StackElem top = stack[stackIndex];

        if (calcOffTime(schedules[top.offIdx]) < schedules[sortedIndices[i]].on) {
            StackElem e = { .onIdx = sortedIndices[i], .offIdx = sortedIndices[i] };
            stack[++stackIndex] = e;
        } else if (calcOffTime(schedules[top.offIdx]) < calcOffTime(schedules[sortedIndices[i]])) {
            top.offIdx = sortedIndices[i];
            stack[stackIndex] = top;
        }
    }

    // Find the matching interval
    Clock_Time lastDiff = 2880;
    int8_t foundIndex = -1;
    while (stackIndex >= 0) {
        StackElem s = stack[stackIndex--];

        Clock_Time onTime = schedules[s.onIdx].on;
        Clock_Time offTime = calcOffTime(schedules[s.offIdx]);

        if (offTime - onTime >= 1440) {
            return false;
        }

        if (currentlyOn) {
            // Find the active schedule and store its OFF time
            if ((time >= onTime && time < offTime) || time + 1440 < offTime) {
                foundIndex = s.offIdx;
                break;
            }
        } else {
            // Find the nearest schedule and store its ON time
            Clock_Time diff;
            if (onTime < time) {
                diff = 1440 - time + onTime;
            } else {
                diff = onTime - time;
            }

            if (foundIndex < 0 || diff < lastDiff) {
                foundIndex = s.onIdx;
                lastDiff = diff;
            }
        }
    }

    *index = foundIndex;
    *on = !currentlyOn;

    return foundIndex >= 0;
}

static void startRampingUp(void)
{
    context.rampingUpStepTime = 2;
    context.rampingStepTimer = context.rampingUpStepTime;
    context.rampingCurrentBrightness = 0;
    context.rampingUp = 1;
}

static void stopRampingUp(void)
{
    context.rampingUp = false;
}

static void rampingUpTask(void)
{
    // Run the timer
    if (context.rampingStepTimer > 0) {
        --context.rampingStepTimer;
        return;
    }

    // Finish when the target brightness is reached
    if (context.rampingCurrentBrightness >= Settings_data.output.brightness) {
        stopRampingUp();
        return;
    }

    ++context.rampingCurrentBrightness;

    context.rampingStepTimer = context.rampingUpStepTime;

    PWM5_LoadDutyValue(context.rampingCurrentBrightness);
}