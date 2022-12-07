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

#include "Types.h"

inline uint8_t Types_calculateScheduleSegmentIndex(const uint16_t minutes)
{
    if (minutes >= 1440) {
        return 0;
    }

    return (uint8_t)(minutes / 30);
}

bool Types_getScheduleSegmentBit(
    const ScheduleSegmentData data,
    const uint8_t segmentIndex
) {
    uint8_t bitIndex = segmentIndex & 0b111;
    uint8_t byteIndex = segmentIndex >> 3;

    return data[byteIndex] & (1 << bitIndex);
}

void Types_setScheduleSegmentBit(
    ScheduleSegmentData data,
    const uint8_t segmentIndex,
    const bool value
) {
    uint8_t bitIndex = segmentIndex & 0b111;
    uint8_t byteIndex = segmentIndex >> 3;
    uint8_t mask = (uint8_t)(1 << bitIndex);

    if (value) {
        data[byteIndex] |= mask;
    } else {
        data[byteIndex] &= ~mask;
    }
}