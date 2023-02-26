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

#include "Config.h"

#include <stdbool.h>
#include <stdint.h>

#if !SUNRISE_SUNSET_USE_LUT

typedef struct {
    uint8_t hour;
    uint8_t minute;
} SunriseSunset_Time;

typedef struct {
    double latitudeCos;
    double latitudeSin;
    double longitudeHour;
    int8_t timeOffset;
} SunriseSunsetData;

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

uint16_t SunriseSunset_calculate(
    SunriseSunsetData* data,
    bool sunset,
    uint16_t dayOfYear
);
#else
uint16_t SunriseSunset_calculate(
    bool sunset,
    uint16_t dayOfYear
);
#endif

void SunriseSunset_update(void);