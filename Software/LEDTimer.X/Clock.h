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

#pragma once

#include "Clock.h"
#include "Utils.h"

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef int16_t Clock_Ticks;
typedef int16_t Clock_Time;

typedef volatile struct
{
    volatile Clock_Ticks ticks;
    volatile Clock_Ticks fastTicks;
    volatile time_t epoch;
    volatile bool updateCalendar;
} Clock_InterruptContext;

#define Clock_handleRTCTimerInterrupt() {\
    extern Clock_InterruptContext Clock_interruptContext; \
    Clock_interruptContext.ticks += 1; \
    Clock_interruptContext.epoch += 2; \
    Clock_interruptContext.updateCalendar = true; \
}

#define Clock_handleFastTimerInterrupt() { \
    extern Clock_InterruptContext Clock_interruptContext; \
    ++Clock_interruptContext.fastTicks; \
}

inline Clock_Time Clock_getMinutesSinceMidnight(void);
void Clock_setMinutesSinceMidnight(Clock_Time value);
inline Clock_Ticks Clock_getTicks(void);
inline Clock_Ticks Clock_getFastTicks(void);
inline Clock_Ticks Clock_getElapsedTicks(Clock_Ticks since);
inline Clock_Ticks Clock_getElapsedFastTicks(Clock_Ticks since);
void Clock_task(void);
void Clock_setDate(YearsFrom2023 year, uint8_t month, uint8_t day, uint8_t weekday);
inline YearsFrom2023 Clock_getYear(void);
inline uint8_t Clock_getMonth(void);
inline uint8_t Clock_getDay(void);
inline uint8_t Clock_getWeekday(void);
inline bool Clock_isLeapYear(void);
uint16_t Clock_getDayOfYear(void);
