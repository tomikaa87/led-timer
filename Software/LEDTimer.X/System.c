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

#include "System.h"

#include "mcc_generated_files/adc.h"

#include <stdio.h>
#include <xc.h>

#define KeyPressWakeUpLength        (3)
#define MonitoringUpdateInterval    (2)

#define VDDCalMilliVolts            (3140ul)
#define VDDCalADCValue              (332ul)

System system = {
    .sleep = {
        .enabled = true,
        .lastWakeUpTime = 0,
        .wakeUpReason = System_WakeUpReason_None
    },
    .monitoring = {
        .vddReadingUpdated = false,
        .vddADCValue = 0,
        .lastUpdateTime = 0
    }
};

void adcISR()
{
    system.monitoring.vddADCValue = ADC_GetConversionResult();
    system.monitoring.vddReadingUpdated = true;
}

static void measureVDD()
{
    ADC_SelectChannel(channel_FVR);
    ADC_StartConversion();
}

void System_init()
{
    ADC_SetInterruptHandler(adcISR);
    ADC_SelectChannel(channel_FVR);
}

System_TaskResult System_task()
{
    if (Clock_getElapsedTicks(system.monitoring.lastUpdateTime) >= MonitoringUpdateInterval) {
        system.monitoring.lastUpdateTime = Clock_getTicks();
        measureVDD();
    }
    
    if (!system.sleep.enabled) {
        Clock_Ticks elapsedSinceWakeUp
            = Clock_getElapsedTicks(system.sleep.lastWakeUpTime);
 
        switch (system.sleep.wakeUpReason) {
            case System_WakeUpReason_None:
                break;
                
            case System_WakeUpReason_KeyPress:
                if (elapsedSinceWakeUp >= KeyPressWakeUpLength) {
                    system.sleep.enabled = true;
                }
                break;
        }
    } else {
        return System_TaskResult_EnterSleepMode;
    }
    
    return System_TaskResult_NoActionNeeded;
}

void System_wakeUp(const System_WakeUpReason reason)
{
    system.sleep.enabled = false;
    system.sleep.lastWakeUpTime = Clock_getTicks();
    system.sleep.wakeUpReason = reason;
}

void System_sleep()
{
    printf("SYS:slp,t=%u\r\n", Clock_getTicks());

    // Send out all the data before going to sleep
    while (TRMT == 0);

    SLEEP();

    // Wait for the oscillator to stabilize
    while (OSCSTATbits.HFIOFS == 0);
}

inline bool System_isVDDReadingUpdated()
{
    return system.monitoring.vddReadingUpdated;
}

uint16_t System_getVDDMilliVolts()
{
    system.monitoring.vddReadingUpdated = false;
    return (uint16_t)(
        VDDCalMilliVolts * VDDCalADCValue / system.monitoring.vddADCValue
    );
}