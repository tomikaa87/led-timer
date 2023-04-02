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
    .minutesSinceMidnight = 0,
    .seconds = 0
};

static struct Clock_Context {
    uint8_t day : 5;
    uint8_t weekday : 3;
    uint8_t leapYear : 1;
    uint8_t month : 7;
    uint8_t yearsFrom2023;
} context = {
    .day = 1,
    .weekday = 0,
    .month = 1,
    .yearsFrom2023 = 0
};

inline uint8_t Clock_getSeconds()
{
    return Clock_interruptContext.seconds;
}

inline Clock_Time Clock_getMinutesSinceMidnight()
{
    return Clock_interruptContext.minutesSinceMidnight;
}

void Clock_setMinutesSinceMidnight(const Clock_Time value)
{
    TMR1_WriteTimer(0);
    Clock_interruptContext.minutesSinceMidnight = value;
    Clock_interruptContext.seconds = 0;
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

static bool isLeapYear()
{
    uint16_t year = context.yearsFrom2023 + 2023;
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

static uint8_t lastDayOfMonth(const uint8_t month)
{
    if (month == 2) {
        if (isLeapYear()) {
            return 29;
        }

        return 28;
    }

    if (month == 4 || month == 6 || month == 9 || month == 11) {
        return 30;
    }

    return 31;
}

void Clock_task()
{
    if (Clock_interruptContext.updateCalendar) {
        Clock_interruptContext.updateCalendar = false;

        if (++context.weekday > 6) {
            context.weekday = 0;
        }

        if (++context.day > lastDayOfMonth(context.month)) {
            context.day = 1;

            if (++context.month > 11) {
                context.month = 1;
                ++context.yearsFrom2023;

                context.leapYear = isLeapYear();
            }

            SunriseSunset_update();
        }
    }
}

void Clock_setDate(
    const uint8_t yearsFrom2023,
    const uint8_t month,
    const uint8_t day,
    const uint8_t weekday
) {
    if (month > 12 || day > 31 || weekday > 6) {
        return;
    }

    context.yearsFrom2023 = yearsFrom2023;
    context.month = month;
    context.day = day;
    context.weekday = weekday;
    context.leapYear = isLeapYear();

    SunriseSunset_update();
}

inline uint8_t Clock_getYearsFrom2023()
{
    return context.yearsFrom2023;
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

inline uint8_t Clock_isLeapYear()
{
    return context.leapYear;
}

inline uint16_t Clock_calculateDayOfYear()
{
    uint16_t day = 0;

    for (uint8_t i = 1; i < context.month; ++i) {
        day += lastDayOfMonth(i);
    }

    day += context.day;

    return day;
}