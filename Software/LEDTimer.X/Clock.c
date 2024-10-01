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
#include "SunsetSunrise.h"

#include "mcc_generated_files/tmr1.h"
#include "mcc_generated_files/tmr4.h"

#include <stdlib.h>
#include <xc.h>

Clock_InterruptContext Clock_interruptContext = {
    .ticks = 0,
    .fastTicks = 0,
    .epoch = 0
};

static struct Clock_Context {
    uint8_t hour;
    uint8_t minute;
    uint8_t day : 5;
    uint8_t weekday : 3;
    uint8_t leapYear : 1;
    uint8_t month : 6;
    uint8_t initialUpdate : 1;
    YearsFrom2023 year;
    uint16_t dayOfYear;
} context = {
    .hour = 0,
    .minute = 0,
    .day = 1,
    .weekday = 0,
    .month = 1,
    .initialUpdate = 1,
    .year = 0,
    .dayOfYear = 0
};

//inline uint8_t Clock_getSeconds()
//{
//    return Clock_interruptContext.epoch;
//}

inline Clock_Time Clock_getMinutesSinceMidnight()
{
    return context.hour * 60 + context.minute;
}

void Clock_setMinutesSinceMidnight(const Clock_Time value)
{
    context.hour = (uint8_t)(value / 60);
    context.minute = (uint8_t)(value - context.hour * 60);

    struct tm time = {};
    time.tm_hour = context.hour;
    time.tm_min = context.minute;
    time.tm_mday = context.day;
    time.tm_mon = context.month;
    time.tm_year = (int)context.year + 2023 - 1900;

    Clock_interruptContext.epoch = mktime(&time);
    Clock_interruptContext.updateCalendar = true;

    TMR1_WriteTimer(0);
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

void Clock_task()
{
    if (Clock_interruptContext.updateCalendar) {
        Clock_interruptContext.updateCalendar = false;

        struct tm* time = gmtime((const time_t*)Clock_interruptContext.epoch);

        bool updateSunriseSunset = context.dayOfYear != time->tm_yday || context.initialUpdate;

        context.initialUpdate = false;

        context.hour = (uint8_t)time->tm_hour;
        context.minute = (uint8_t)time->tm_min;
        context.day = (uint8_t)time->tm_mday;
        context.weekday = (uint8_t)time->tm_wday;
        context.month = (uint8_t)time->tm_mon + 1;
        context.year = (uint8_t)(time->tm_year + 1900 - 2023);
        context.leapYear = Date_isLeapYear(context.year);
        context.dayOfYear = (uint16_t)time->tm_yday;

        if (updateSunriseSunset) {
            SunriseSunset_update();
        }
    }
}

void Clock_setDate(
    const YearsFrom2023 year,
    const uint8_t month,
    const uint8_t day,
    const uint8_t weekday
) {
    if (month > 12 || day > 31 || weekday > 6) {
        return;
    }

    context.year = year;
    context.month = month;
    context.day = day;

    struct tm time = {};
    time.tm_hour = context.hour;
    time.tm_min = context.minute;
    time.tm_mday = context.day;
    time.tm_mon = context.month;
    time.tm_year = (int)context.year + 2023 - 1900;

    Clock_interruptContext.epoch = mktime(&time);
    Clock_interruptContext.updateCalendar = true;

    SunriseSunset_update();
}

inline YearsFrom2023 Clock_getYear()
{
    return context.year;
}

inline uint8_t Clock_getMonth()
{
    return context.month;
}

inline uint8_t Clock_getDay()
{
    return context.day;
}

inline uint8_t Clock_getWeekday()
{
    return context.weekday;
}

inline bool Clock_isLeapYear()
{
    return context.leapYear;
}

uint16_t Clock_getDayOfYear()
{
    return context.dayOfYear;
}