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

#include "Clock.h"
#include "Settings.h"
#include "SunsetSunrise.h"

#include <stdint.h>
#include <stdlib.h>
#include <xc.h>

Clock_InterruptContext Clock_interruptContext = {
    .ticks = 0,
    .fastTicks = 0,
    .utcEpoch = 1704067200, // 2024-01-01 00:00:00
    .updateCalendar = true
};

static struct Clock_Context {
    uint8_t hour;
    uint8_t minute;
    uint8_t day : 5;
    uint8_t weekday : 3;
    uint8_t leapYear : 1;
    uint8_t month : 6;
    uint8_t initialUpdate : 1;
    YearsFrom1970 year;
    uint16_t dayOfYear;
} Clock_context = {
    .hour = 0,
    .minute = 0,
    .day = 1,
    .weekday = 0,
    .month = 1,
    .initialUpdate = 1,
    .year = 0,
    .dayOfYear = 0
};

inline Clock_Time Clock_getMinutesSinceMidnight()
{
    return (Clock_Time)Clock_context.hour * 60 + Clock_context.minute;
}

void Clock_setTime(const uint8_t hour, const uint8_t minute, const uint8_t seconds)
{
    Clock_context.hour = hour;
    Clock_context.minute = minute;

    struct tm time = {};
    time.tm_hour = Clock_context.hour;
    time.tm_min = Clock_context.minute;
    time.tm_sec = seconds;
    time.tm_mday = Clock_context.day;
    time.tm_mon = Clock_context.month - 1;
    time.tm_year = (int)Clock_context.year + 70;

    TMR1ON = 0;
    Clock_interruptContext.utcEpoch = mktime(&time)
        - (time_t)Settings_data.time.timeZoneOffsetHalfHours * 30 * 60;
    TMR1H = 0;
    TMR1L = 0;
    TMR1ON = 1;

    Clock_interruptContext.updateCalendar = true;
}

inline Clock_Ticks Clock_getTicks()
{
    return Clock_interruptContext.ticks;
}

inline Clock_Ticks Clock_getFastTicks()
{
    return Clock_interruptContext.fastTicks;
}

inline Clock_Ticks Clock_getElapsedTicks(const Clock_Ticks since)
{
    return (Clock_Ticks)abs((int16_t)Clock_interruptContext.ticks - (int16_t)since);
}

inline Clock_Ticks Clock_getElapsedFastTicks(Clock_Ticks since)
{
    return (Clock_Ticks)abs((int16_t)Clock_interruptContext.fastTicks - (int16_t)since);
}

void Clock_runTasks()
{
    if (Clock_interruptContext.updateCalendar) {
        Clock_interruptContext.updateCalendar = false;

        time_t localTime = Clock_interruptContext.utcEpoch +
            (time_t)Settings_data.time.timeZoneOffsetHalfHours * 30 * 60;

        struct tm* time = gmtime(&localTime);

        bool updateSunriseSunset = Clock_context.dayOfYear != time->tm_yday || Clock_context.initialUpdate;

        Clock_context.initialUpdate = false;

        Clock_context.hour = (uint8_t)time->tm_hour;
        Clock_context.minute = (uint8_t)time->tm_min;
        Clock_context.day = (uint8_t)time->tm_mday;
        Clock_context.weekday = (uint8_t)time->tm_wday;
        Clock_context.month = (uint8_t)time->tm_mon + 1;
        Clock_context.year = (uint8_t)(time->tm_year - 70);
        Clock_context.leapYear = Date_isLeapYear(Clock_context.year);
        Clock_context.dayOfYear = (uint16_t)time->tm_yday;

        if (updateSunriseSunset) {
            SunriseSunset_update();
        }
    }
}

void Clock_setDate(
    const YearsFrom1970 year,
    const uint8_t month,
    const uint8_t day
) {
    if (month == 0 || month > 12 || day == 0 || day > 31) {
        return;
    }

    Clock_context.year = year;
    Clock_context.month = month;
    Clock_context.day = day;

    struct tm time = {};
    time.tm_hour = Clock_context.hour;
    time.tm_min = Clock_context.minute;
    time.tm_mday = Clock_context.day;
    time.tm_mon = Clock_context.month - 1;
    time.tm_year = (int)Clock_context.year + 70;

    // FIXME fill tm_sec as well

    TMR1ON = 0;
    Clock_interruptContext.utcEpoch = mktime(&time) -
        (time_t)Settings_data.time.timeZoneOffsetHalfHours * 30 * 60;
    TMR1ON = 1;

    Clock_interruptContext.updateCalendar = true;

    SunriseSunset_update();
}

inline YearsFrom1970 Clock_getYear()
{
    return Clock_context.year;
}

inline uint8_t Clock_getMonth()
{
    return Clock_context.month;
}

inline uint8_t Clock_getDay()
{
    return Clock_context.day;
}

inline uint8_t Clock_getWeekday()
{
    return Clock_context.weekday;
}

inline bool Clock_isLeapYear()
{
    return Clock_context.leapYear;
}

uint16_t Clock_getDayOfYear()
{
    return Clock_context.dayOfYear;
}

inline uint8_t Clock_getHour()
{
    return Clock_context.hour;
}

inline uint8_t Clock_getMinute()
{
    return Clock_context.minute;
}

inline time_t Clock_getUtcEpoch()
{
    return Clock_interruptContext.utcEpoch;
}