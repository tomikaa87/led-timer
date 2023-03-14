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

    Calculations are based on:
    https://www.omnicalculator.com/physics/sunrise-sunset
*/

#include "Clock.h"
#include "Settings.h"
#include "SunsetSunrise.h"

static struct SunriseSunsetContext {
    uint16_t sunrise;
    uint16_t sunset;
} context;

#if !SUNRISE_SUNSET_USE_LUT
#include <math.h>

#define COS_SUN_ANGLE_BELOW_HORIZON (-0.014538080502497)    // cos(rad(90.833))
#define DEG_TO_RAD_FACTOR           (0.017453292519943)     // Pi / 180
#define RAD_TO_DEG_FACTOR           (57.295779513082321)    // 180 / Pi

static double rad(const double deg)
{
    return deg * DEG_TO_RAD_FACTOR;
}

static double deg(const double rad)
{
    return rad * RAD_TO_DEG_FACTOR;
}

static double quadrant(const double deg)
{
    return 90 * floor(deg / 90);
}

static double clamp(const double deg)
{
    return deg - 360 * floor(deg / 360);
}

void SunriseSunset_setPosition(
    SunriseSunsetData* const data,
    const double longitude,
    const double latitude
) {
    double latitudeRad = rad(latitude);
    data->latitudeCos = cos(latitudeRad);
    data->latitudeSin = sin(latitudeRad);
    data->longitudeHour = longitude / 15;
}

void SunriseSunset_setTimeZone(
    SunriseSunsetData* const data,
    const int8_t timeZoneOffset,
    const bool dst
) {
    data->timeOffset = timeZoneOffset + (dst ? 1 : 0);
}

uint16_t SunriseSunset_calculate(
    SunriseSunsetData* const data,
    const bool sunset,
    const uint16_t dayOfYear
) {
    double t = dayOfYear + ((sunset ? 18 : 6) - data->longitudeHour) / 24;

    // Mean anomaly
    double mDeg = 0.9856 * t - 3.289;
    double mRad = rad(mDeg);

    // True latitude of the Sun
    double lDeg = clamp(mDeg + 1.916 * sin(mRad) + 0.02 * sin(2 * mRad) + 282.634);
    double lRad = rad(lDeg);

    // Right ascension
    double ra1Deg = clamp(deg(atan(0.91764 * tan(lRad))));
    double raDeg = ra1Deg + quadrant(lDeg) - quadrant(ra1Deg);

    double sunDeclinationRad = asin(0.39782 * sin(lRad));

    double hourAngleDeg = deg(
        acos(
            (COS_SUN_ANGLE_BELOW_HORIZON - sin(sunDeclinationRad) * data->latitudeSin)
            / (cos(sunDeclinationRad) * data->latitudeCos)
        )
    );

    if (!sunset) {
        hourAngleDeg = 360 - hourAngleDeg;
    }

    double localTime = hourAngleDeg / 15 + raDeg / 15 - 0.06571 * t - 6.622 - data->longitudeHour;

    double adjustedTime = localTime + data->timeOffset;
    adjustedTime = (adjustedTime - 24 * floor(adjustedTime / 24));

    uint8_t hour = (uint8_t)(floor(adjustedTime));
    uint8_t minute = (uint8_t)(round((adjustedTime - hour) * 60.0));

    // Convert to minutes since midnight
    return (uint16_t)hour * 60 + minute;
}
#else
extern const uint16_t SunriseSunsetLUT[2][365];

uint16_t SunriseSunset_calculate(
    const bool sunset,
    const uint16_t dayOfYear
) {
    if (dayOfYear >= 365) {
        return 0;
    }

    return SunriseSunsetLUT[sunset ? 1 : 0][dayOfYear];
}
#endif

uint16_t SunriseSunset_getSunrise(void)
{
    return context.sunrise;
}

uint16_t SunriseSunset_getSunset(void)
{
    return context.sunset;
}

void SunriseSunset_update()
{
#if !SUNRISE_SUNSET_USE_LUT
    SunriseSunsetData data;

    SunriseSunset_setPosition(
        &data,
        Types_bcdToDouble(
            Settings_data.location.latitudeBcd,
            Settings_data.location.latitudeSign
        ),
        Types_bcdToDouble(
            Settings_data.location.longitudeBcd,
            Settings_data.location.longitudeSign
        )
    );

    // TODO get time zone data from settings
    SunriseSunset_setTimeZone(&data, 1, false);
#endif

    uint16_t dayOfYear = Clock_calculateDayOfYear();

#if SUNRISE_SUNSET_USE_LUT
    // Adjust the index for non-leap years after February 29th
    if (!Clock_isLeapYear() && dayOfYear > 59) {
        dayOfYear += 1;
    }
#endif

    context.sunrise = SunriseSunset_calculate(
#if !SUNRISE_SUNSET_USE_LUT
        &data,
#endif
        false,
        dayOfYear
    );

    context.sunset = SunriseSunset_calculate(
#if !SUNRISE_SUNSET_USE_LUT
        &data,
#endif
        true,
        dayOfYear
    );
}