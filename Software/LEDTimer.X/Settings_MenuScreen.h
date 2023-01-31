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
    Created on 2023-01-31
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    Settings_MenuScreen_KeyHandled,
    Settings_MenuScreen_Exited,
    Settings_MenuScreen_ItemSelected
} Settings_MenuScreen_KeyPressResult;

void Settings_MenuScreen_init(void);
void Settings_MenuScreen_update(bool redraw);
Settings_MenuScreen_KeyPressResult Settings_MenuScreen_handleKeyPress(uint8_t keyCode, bool hold);
uint8_t Settings_MenuScreen_lastSelectionIndex(void);