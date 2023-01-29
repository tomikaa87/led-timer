/*
 * File:   SunsetSunrise.h
 * Author: tkarpati
 *
 * Created on January 29, 2023, 10:41 AM
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

