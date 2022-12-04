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
    Created on 2022-12-01
*/

#pragma once

#include "Clock.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    System_WakeUpReason_None,
    System_WakeUpReason_Startup,
    System_WakeUpReason_KeyPress
} System_WakeUpReason;

typedef struct
{
    struct _Sleep
    {
        bool enabled;
        Clock_Ticks lastWakeUpTime;
        System_WakeUpReason wakeUpReason;
    } sleep;

    struct _Monitoring
    {
        volatile bool vddReadingUpdated;
        volatile uint16_t vddADCValue;
        Clock_Ticks lastUpdateTime;
        volatile bool onBatteryPower;       // True if the system is running from battery power
        uint8_t batteryLevel;               // Estimated batter level: 0..10
    } monitoring;
} System;

extern System system;

typedef enum
{
    System_TaskResult_NoActionNeeded,
    System_TaskResult_EnterSleepMode
} System_TaskResult;

void System_init(void);
System_TaskResult System_task(void);

void System_wakeUp(System_WakeUpReason reason);
void System_sleep(void);

inline bool System_isVDDReadingUpdated(void);
uint16_t System_getVDDMilliVolts(void);
uint16_t System_getVBatMilliVolts(void);