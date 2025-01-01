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

static void TMR1_StartTimer(void)
{
    // Start the Timer by writing to TMRxON bit
    T1CONbits.TMR1ON = 1;
}

static void TMR1_StopTimer(void)
{
    // Stop the Timer by writing to TMRxON bit
    T1CONbits.TMR1ON = 0;
}

static void TMR1_WriteTimer(uint16_t timerVal)
{
    if (T1CONbits.T1SYNC == 1)
    {
        // Stop the Timer by writing to TMRxON bit
        T1CONbits.TMR1ON = 0;

        // Write to the Timer1 register
        TMR1H = (uint8_t)(timerVal >> 8);
        TMR1L = (uint8_t)timerVal;

        // Start the Timer after writing to the register
        T1CONbits.TMR1ON =1;
    }
    else
    {
        // Write to the Timer1 register
        TMR1H = (uint8_t)(timerVal >> 8);
        TMR1L = (uint8_t)timerVal;
    }
}

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

inline Clock_Time Clock_getMinutesSinceMidnight()
{
    return (Clock_Time)context.hour * 60 + context.minute;
}

void Clock_setTime(const uint8_t hour, const uint8_t minute, const uint8_t seconds)
{
    context.hour = hour;
    context.minute = minute;

    struct tm time = {};
    time.tm_hour = context.hour;
    time.tm_min = context.minute;
    time.tm_sec = seconds;
    time.tm_mday = context.day;
    time.tm_mon = context.month - 1;
    time.tm_year = (int)context.year + 70;

    TMR1_StopTimer();
    Clock_interruptContext.utcEpoch = mktime(&time)
        - (time_t)Settings_data.time.timeZoneOffsetHalfHours * 30 * 60;
    TMR1_WriteTimer(0);
    TMR1_StartTimer();

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

        bool updateSunriseSunset = context.dayOfYear != time->tm_yday || context.initialUpdate;

        context.initialUpdate = false;

        context.hour = (uint8_t)time->tm_hour;
        context.minute = (uint8_t)time->tm_min;
        context.day = (uint8_t)time->tm_mday;
        context.weekday = (uint8_t)time->tm_wday;
        context.month = (uint8_t)time->tm_mon + 1;
        context.year = (uint8_t)(time->tm_year - 70);
        context.leapYear = Date_isLeapYear(context.year);
        context.dayOfYear = (uint16_t)time->tm_yday;

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

    context.year = year;
    context.month = month;
    context.day = day;

    struct tm time = {};
    time.tm_hour = context.hour;
    time.tm_min = context.minute;
    time.tm_mday = context.day;
    time.tm_mon = context.month - 1;
    time.tm_year = (int)context.year + 70;

    TMR1_StopTimer();
    Clock_interruptContext.utcEpoch = mktime(&time) -
        (time_t)Settings_data.time.timeZoneOffsetHalfHours * 30 * 60;
    TMR1_StartTimer();

    Clock_interruptContext.updateCalendar = true;

    SunriseSunset_update();
}

inline YearsFrom1970 Clock_getYear()
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

inline uint8_t Clock_getHour()
{
    return context.hour;
}

inline uint8_t Clock_getMinute()
{
    return context.minute;
}

inline time_t Clock_getUtcEpoch()
{
    return Clock_interruptContext.utcEpoch;
}