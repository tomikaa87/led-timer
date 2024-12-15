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

#pragma once

#include "Clock.h"
#include "Settings.h"

#include <stdbool.h>

/**
 * Toggles the output temporarily without altering the schedule.
 */
void OutputController_toggle(void);

typedef enum {
    OutputController_TaskResult_StateUnchanged,
    OutputController_TaskResult_OutputStateChanged
} OutputController_TaskResult;

/**
 * Runs the essential tasks, like switching the output based on
 * the saved schedule.
 * @return Result of the task representing internal state changes
 */
OutputController_TaskResult OutputController_task(void);

/**
 * Returns the output state in case there is external power
 * @return True if the output is enabled on external power
 */
inline bool OutputController_outputEnableTargetState(void);

/**
 * Returns the actual state of the output
 * @return True of the output actually enabled
 */
inline bool OutputController_isOutputEnabled(void);

/**
 * Forces turn on or off the output based on the current state. If the output
 * is enabled, this function will update the PWM duty cycle as well.
 */
void OutputController_updateState(void);

/**
 * Calculates the next state transition based on the configured schedules
 * and the specified time.
 *
 * @param time Time value in minutes from midnight used for the calculation
 * @param index Output parameter, set to the index of the schedule involved in the next transition
 * @param on Output parameter, set to true for ON transition and false for OFF transition
 * @return true If a transition can be calculated, false otherwise
 */
bool OutputController_getNextTransition(
    Clock_Time time,
    int8_t* index,
    bool* on
);

/**
 * Calculates switch time from based on the specified switch data.
 *
 * @param sw Switch data from an interval-type schedule
 * @return Clock_Time Calculated switch time in minutes from midnight
 */
Clock_Time OutputController_calculateSwitchTime(struct IntervalSwitch* sw);