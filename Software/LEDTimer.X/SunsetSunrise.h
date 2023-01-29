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
    Created on 2023-01-29
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    double latitudeCos;
    double latitudeSin;
    double longitudeHour;
    int8_t timeOffset;
} SunriseSunsetData;

typedef struct {
    int hour;
    int minute;
} SunriseSunset_Time;

void SunriseSunset_setPosition(
    SunriseSunsetData* data,
    double longitude,
    double latitude
);

void SunriseSunset_setTimeZone(
    SunriseSunsetData* data,
    int8_t timeZoneOffset,
    bool dst
);

SunriseSunset_Time SunriseSunset_calculate(
    SunriseSunsetData* data,
    bool sunset,
    const int dayOfYear
);

