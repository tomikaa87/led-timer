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
    Created on 2022-11-29
*/

#pragma once

#include "Types.h"

#include <stdbool.h>
#include <stdint.h>

#define Graphics_BulbIconWidth      15
#define Graphics_BulbIconPages      3
extern const uint8_t Graphics_BulbIcon[Graphics_BulbIconPages][Graphics_BulbIconWidth];

void Graphics_drawBitmap(
    const uint8_t* bitmap,
    uint8_t width,
    uint8_t x,
    uint8_t page,
    bool invert
);

void Graphics_drawMultipageBitmap(
    const uint8_t* bitmap,
    uint8_t width,
    uint8_t pageCount,
    uint8_t x,
    uint8_t startPage,
    bool invert
);

void Graphics_drawScheduleBar(
    const ScheduleSegmentData segmentData,
    bool invert
);

void Graphics_drawScheduleSegmentIndicator(
    uint8_t segmentIndex,
    bool invert
);