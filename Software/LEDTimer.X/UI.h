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
    Created on 2022-12-02
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * Initializes the display and the internal logic.
 */
void UI_init(void);

/**
 * Runs the essential tasks of the UI, including handling timers and periodic
 * screen updates.
 */
void UI_task(void);

/**
 * Handles keypad key presses
 * @param keyCode Current key code read from the keypad.
 */
void UI_keyEvent(uint8_t keyCode);

typedef enum
{
    UI_ExternalEvent_SystemWakeUp =                     (1 << 0),
    UI_ExternalEvent_PowerInputChanged =                (1 << 1),
    UI_ExternalEvent_BatteryLevelMeasurementFinished =  (1 << 2)
} UI_ExternalEvent;

inline void UI_setExternalEvent(UI_ExternalEvent event);