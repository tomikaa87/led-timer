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

#define Graphics_BulbIconWidth                      15
#define Graphics_BulbIconPages                      3
extern const uint8_t Graphics_BulbIcon[Graphics_BulbIconPages][Graphics_BulbIconWidth];

#define Graphics_BatteryIndicatorCapWidth           3
extern const uint8_t Graphics_BatteryIndicatorCap[Graphics_BatteryIndicatorCapWidth];

#define Graphics_BatteryIndicatorBodyFullWidth      1
extern const uint8_t Graphics_BatteryIndicatorBodyFull[Graphics_BatteryIndicatorBodyFullWidth];

#define Graphics_BatteryIndicatorBodyEmptyWidth     1
extern const uint8_t Graphics_BatteryIndicatorBodyEmpty[Graphics_BatteryIndicatorBodyEmptyWidth];

#define Graphics_BatteryIndicatorEndCapWidth        2
extern const uint8_t Graphics_BatteryIndicatorEndCap[Graphics_BatteryIndicatorEndCapWidth];

#define Graphics_ExternalPowerIndicatorWidth        15
extern const uint8_t Graphics_ExternalPowerIndicator[Graphics_ExternalPowerIndicatorWidth];

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