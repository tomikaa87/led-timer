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

#include <stdint.h>

typedef uint16_t Clock_Ticks;

typedef volatile struct
{
    volatile Clock_Ticks ticks;
    volatile Clock_Ticks fastTicks;
    volatile uint16_t minutesSinceMidnight;
    volatile uint8_t seconds;
} Clock_InterruptContext;

#define Clock_handleRTCTimerInterrupt() {\
    extern Clock_InterruptContext Clock_interruptContext; \
    Clock_interruptContext.ticks += 1; \
    Clock_interruptContext.seconds += 2; \
	if (Clock_interruptContext.seconds >= 60) { \
        Clock_interruptContext.seconds = 0; \
        if (++Clock_interruptContext.minutesSinceMidnight >= 1440) { \
            Clock_interruptContext.minutesSinceMidnight = 0; \
        } \
    } \
}

#define Clock_handleFastTimerInterrupt() { \
    extern Clock_InterruptContext Clock_interruptContext; \
    ++Clock_interruptContext.fastTicks; \
}

inline uint8_t Clock_getSeconds(void);
inline uint16_t Clock_getMinutesSinceMidnight(void);
inline Clock_Ticks Clock_getTicks(void);
inline Clock_Ticks Clock_getFastTicks(void);
inline Clock_Ticks Clock_getElapsedTicks(Clock_Ticks since);
inline Clock_Ticks Clock_getElapsedFastTicks(Clock_Ticks since);