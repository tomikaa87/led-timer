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

inline uint8_t Clock_getSeconds()
{
    return Clock_interruptContext.seconds;
}

inline uint16_t Clock_getMinutesSinceMidnight()
{
    return Clock_interruptContext.minutesSinceMidnight;
}

void Clock_setMinutesSinceMidnight(const uint16_t value)
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