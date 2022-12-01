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

#include <stdint.h>

void Graphics_drawBitmap(
    const uint8_t* bitmap,
    uint8_t width,
    uint8_t x,
    uint8_t page
);

void Graphics_drawMultipageBitmap(
    const uint8_t* mp_bitmap,
    uint8_t width,
    uint8_t page_count,
    uint8_t x,
    uint8_t start_page
);

void Graphics_drawScheduleBar(ScheduleSegmentData segmentData);
void Graphics_drawScheduleSegmentIndicator(uint8_t segmentIndex);