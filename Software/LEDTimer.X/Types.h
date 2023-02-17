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

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t ScheduleSegmentData[6];

/**
 * Calculates the segment index by the elapsed minutes from midnight.
 * The index can be used to address the corresponding bit in the
 * schedule segment data or drawing the schedule segment indicator to the
 * right position.
 * @param minutes Elapsed minutes from midnight.
 * @return Segment index from 0 to 47.
 */
inline uint8_t Types_calculateScheduleSegmentIndex(uint16_t minutes);

bool Types_getScheduleSegmentBit(
    const ScheduleSegmentData data,
    uint8_t segmentIndex
);

void Types_setScheduleSegmentBit(
    ScheduleSegmentData data,
    uint8_t segmentIndex,
    bool value
);

double Types_bcdToDouble(uint32_t bcd, bool negative);