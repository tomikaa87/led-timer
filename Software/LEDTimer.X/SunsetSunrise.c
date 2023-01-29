#include "SunsetSunrise.h"

#include <math.h>

#define COS_SUN_ANGLE_BELOW_HORIZON (-0.014538080502497) // cos(rad(90.833))

static double rad(const double deg)
{
    return deg * M_PI / 180.0;
}

static double deg(const double rad)
{
    return rad * 180 / M_PI;
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

SunriseSunset_Time SunriseSunset_calculate(
    SunriseSunsetData* const data,
    const bool sunset,
    const int dayOfYear
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

    SunriseSunset_Time time;
    time.hour = (int)(floor(adjustedTime));
    time.minute = (int)(round((adjustedTime - time.hour) * 60.0));

    return time;
}