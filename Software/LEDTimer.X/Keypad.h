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
    Created on 2022-12-01
*/

#pragma once

#include <stdint.h>

enum
{
    Keypad_Key1 = (1 << 0),
    Keypad_Key2 = (1 << 1),
    Keypad_Key3 = (1 << 2),
    Keypad_Hold = (1 << 7)
};

void Keypad_init(void);
uint8_t Keypad_task(void);