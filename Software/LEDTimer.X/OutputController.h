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

#include <stdbool.h>

/**
 * Loads the the current settings from the persistent storage
 */
void OutputController_loadSettings(void);

/**
 * Toggles the output temporarily without altering the schedule.
 */
void OutputController_toggle(void);

/**
 * Runs the essential tasks, like switching the output based on
 * the saved schedule.
 */
void OutputController_task(void);

/**
 * Returns the current state of the output.
 * @return True if the output is enabled
 */
inline bool OutputController_isOutputEnabled(void);