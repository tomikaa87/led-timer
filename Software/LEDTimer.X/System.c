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
#include "mcc_generated_files/pin_manager.h"

#include <stdio.h>
#include <xc.h>

#define StartupAwakeLengthTicks             (6)
#define KeyPressWakeUpLengthTicks           (6)
#define PowerInputChangeWakeUpLengthTicks   (6)
#define MonitoringUpdateIntervalTicks       (2)

#define VDDCalMilliVolts                    (3140ul)
#define VDDCalADCValue                      (332ul)

#define VBatDiodeDropMilliVolts             (221)

#define VBatMinMilliVolts                   (2500u)
#define VBatMaxMilliVolts                   (3000u)

static struct SystemContext
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
} context = {
    .sleep = {
        .enabled = true,
        .lastWakeUpTime = 0,
        .wakeUpReason = System_WakeUpReason_None
    },
    .monitoring = {
        .vddReadingUpdated = false,
        .vddADCValue = 0,
        .lastUpdateTime = 0,
        .onBatteryPower = true,
        .batteryLevel = 5
    }
};

static void handleADCInterrupt()
{
    context.monitoring.vddADCValue = ADC_GetConversionResult();
    context.monitoring.vddReadingUpdated = true;
}

static void handleLDOSenseInterrupt()
{
    System_onWakeUp(System_WakeUpReason_PowerInputChanged);
}

static void measureVDD()
{
    ADC_SelectChannel(channel_FVR);
    ADC_StartConversion();
}

void updateBatteryLevel()
{
    uint16_t vbat = System_getVBatMilliVolts();
    vbat =
        vbat > VBatMaxMilliVolts
            ? VBatMaxMilliVolts
            : (
                vbat < VBatMinMilliVolts
                ? VBatMinMilliVolts
                : vbat
            );

    context.monitoring.batteryLevel = (uint8_t)(
        (uint32_t)(vbat - VBatMinMilliVolts) * 10u
        / (VBatMaxMilliVolts - VBatMinMilliVolts)
    );

    printf(
        "SYS:VDD=%u(ADC=%u),VBat=%u,L=%u\r\n",
        System_getVDDMilliVolts(),
        context.monitoring.vddADCValue,
        System_getVBatMilliVolts(),
        context.monitoring.batteryLevel
    );
}

void System_init()
{
    ADC_SetInterruptHandler(handleADCInterrupt);
    ADC_SelectChannel(channel_FVR);

    IOCAF2_SetInterruptHandler(handleLDOSenseInterrupt);
}

System_TaskResult System_task()
{
    if (Clock_getElapsedTicks(context.monitoring.lastUpdateTime) >= MonitoringUpdateIntervalTicks) {
        context.monitoring.lastUpdateTime = Clock_getTicks();
        context.monitoring.onBatteryPower = !IO_LDO_SENSE_GetValue();
        measureVDD();
        updateBatteryLevel();
    }

    if (!context.sleep.enabled) {
        Clock_Ticks elapsedSinceWakeUp
            = Clock_getElapsedTicks(context.sleep.lastWakeUpTime);

        switch (context.sleep.wakeUpReason) {
            case System_WakeUpReason_None:
                break;

            case System_WakeUpReason_Startup:
                if (elapsedSinceWakeUp >= StartupAwakeLengthTicks) {
                    context.sleep.enabled = true;
                }
                break;

            case System_WakeUpReason_KeyPress:
                if (elapsedSinceWakeUp >= KeyPressWakeUpLengthTicks) {
                    context.sleep.enabled = true;
                }
                break;

            case System_WakeUpReason_PowerInputChanged:
                if (elapsedSinceWakeUp >= PowerInputChangeWakeUpLengthTicks) {
                    context.sleep.enabled = true;
                }
                break;
        }
    } else {
        return System_TaskResult_EnterSleepMode;
    }

    return System_TaskResult_NoActionNeeded;
}

void System_onWakeUp(const System_WakeUpReason reason)
{
    context.sleep.enabled = false;
    context.sleep.lastWakeUpTime = Clock_getTicks();
    context.sleep.wakeUpReason = reason;

    if (reason == System_WakeUpReason_PowerInputChanged) {
        // Trigger a VDD measurement on power input change
        context.monitoring.lastUpdateTime = context.sleep.lastWakeUpTime;
    }
}

void System_sleep()
{
    printf(
        "SYS:slp,t=%u,ft=%u,msm=%u,s=%u\r\n",
        Clock_getTicks(),
        Clock_getFastTicks(),
        Clock_getMinutesSinceMidnight(),
        Clock_getSeconds()
    );

    // Send out all the data before going to sleep
    while (TRMT == 0);

    SLEEP();

    // Wait for the oscillator to stabilize
    while (OSCSTATbits.HFIOFS == 0);
}

inline bool System_isRunningFromBackupBattery()
{
    return context.monitoring.onBatteryPower;
}

uint16_t System_getVDDMilliVolts()
{
    context.monitoring.vddReadingUpdated = false;
    return (uint16_t)(
        VDDCalMilliVolts * VDDCalADCValue / context.monitoring.vddADCValue
    );
}

uint16_t System_getVBatMilliVolts()
{
    return System_getVDDMilliVolts() + VBatDiodeDropMilliVolts;
}

inline uint8_t System_getBatteryLevel()
{
    return context.monitoring.batteryLevel;
}