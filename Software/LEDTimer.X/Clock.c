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

#include <stdlib.h>
#include <xc.h>

// In the current configuration, every tick means 2 seconds
volatile static Clock_Ticks _ticks = 0;
volatile static uint16_t _minutesSinceMidnight = 0;
volatile static uint8_t _seconds = 0;

static void handleTimerInterrupt();

void Clock_init()
{
	TMR1_SetInterruptHandler(handleTimerInterrupt);
}

static void handleTimerInterrupt()
{
    _ticks += 1;
    _seconds += 2;
	if (_seconds >= 60) {
        _seconds = 0;
        
        if (++_minutesSinceMidnight >= 1440) {
            _minutesSinceMidnight = 0;
        }
    }
}

inline uint8_t Clock_getSeconds()
{
    return _seconds;
}

inline uint16_t Clock_getMinutesSinceMidnight()
{
    return _minutesSinceMidnight;
}

inline Clock_Ticks Clock_getTicks()
{
    return _ticks;
}

inline Clock_Ticks Clock_getElapsedTicks(const Clock_Ticks since)
{
    return (Clock_Ticks)abs((int16_t)_ticks - (int16_t)since);
}