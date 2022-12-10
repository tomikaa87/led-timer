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

#include "Clock.h"
#include "Config.h"
#include "System.h"

#include "mcc_generated_files/adc.h"
#include "mcc_generated_files/pin_manager.h"

#include <stdio.h>
#include <xc.h>

volatile uint16_t _System_adcResult = 0;

System_InterruptContext System_interruptContext = {
    .adc = {
        .result = 0,
        .updated = false
    },
    .ldoSense = {
        .updated = false
    }
};

static struct SystemContext
{
    struct _Sleep
    {
        volatile bool enabled;
        volatile Clock_Ticks lastWakeUpTime;
        volatile System_WakeUpReason wakeUpReason;
    } sleep;

    struct _Monitoring
    {
        Clock_Ticks lastUpdateTime;
        volatile bool onBatteryPower;       // True if the system is running from battery power
        uint8_t batteryLevel;               // Estimated batter level: 0..10
        volatile bool powerInputChanged;
    } monitoring;
} context = {
    .sleep = {
        .enabled = true,
        .lastWakeUpTime = 0,
        .wakeUpReason = System_WakeUpReason_None
    },
    .monitoring = {
        .lastUpdateTime = 0,
        .onBatteryPower = true,
        .batteryLevel = 0,
        .powerInputChanged = false
    }
};

static void measureVDD()
{
    if (!FVRCONbits.FVREN) {
        return;
    }

    // Wait for the FVR to stabilize
    while (!FVRCONbits.FVRRDY);

    ADC_SelectChannel(channel_FVR);
    ADC_StartConversion();
}

static void checkLDOSense()
{
    context.monitoring.onBatteryPower = !IO_LDO_SENSE_GetValue();
}

void updateBatteryLevel()
{
    uint16_t vbat = System_getVBatMilliVolts();
    vbat =
        vbat > Config_System_VBatMaxMilliVolts
            ? Config_System_VBatMaxMilliVolts
            : (
                vbat < Config_System_VBatMinMilliVolts
                ? Config_System_VBatMinMilliVolts
                : vbat
            );

    context.monitoring.batteryLevel = (uint8_t)(
        (uint32_t)(vbat - Config_System_VBatMinMilliVolts) * 10u
        / (Config_System_VBatMaxMilliVolts - Config_System_VBatMinMilliVolts)
    );

    printf(
        "SYS:VDD=%u(ADC=%u),VBat=%u,L=%u\r\n",
        System_getVDDMilliVolts(),
        System_interruptContext.adc.result,
        System_getVBatMilliVolts(),
        context.monitoring.batteryLevel
    );
}

void System_init()
{
    ADC_SelectChannel(channel_FVR);

    checkLDOSense();
    measureVDD();
    updateBatteryLevel();
}

System_TaskResult System_task()
{
    System_TaskResult result = {
        .action = System_TaskResult_NoActionNeeded,
        .powerInputChanged = context.monitoring.powerInputChanged
    };

    checkLDOSense();

    if (
        context.monitoring.powerInputChanged
        || Clock_getElapsedTicks(context.monitoring.lastUpdateTime)
            >= Config_System_MonitoringUpdateIntervalTicks
    ) {
        context.monitoring.powerInputChanged = false;
        context.monitoring.lastUpdateTime = Clock_getTicks();
//        checkLDOSense();
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
                if (elapsedSinceWakeUp >= Config_System_StartupAwakeLengthTicks) {
                    context.sleep.enabled = context.monitoring.onBatteryPower;
                }
                break;

            case System_WakeUpReason_KeyPress:
                if (elapsedSinceWakeUp >= Config_System_KeyPressWakeUpLengthTicks) {
                    context.sleep.enabled = context.monitoring.onBatteryPower;
                }
                break;

            case System_WakeUpReason_PowerInputChanged:
                if (elapsedSinceWakeUp >= Config_System_PowerInputChangeWakeUpLengthTicks) {
                    context.sleep.enabled = context.monitoring.onBatteryPower;
                }
                break;
        }
    } else {
        result.action = System_TaskResult_EnterSleepMode;
    }

    return result;
}

void System_onWakeUp(const System_WakeUpReason reason)
{
    context.sleep.enabled = false;
    context.sleep.lastWakeUpTime = Clock_getTicks();
    context.sleep.wakeUpReason = reason;

    if (reason == System_WakeUpReason_PowerInputChanged) {
        context.monitoring.powerInputChanged = true;
    }
}

inline System_WakeUpReason System_getLastWakeUpReason()
{
    return context.sleep.wakeUpReason;
}

void System_sleep()
{
    printf(
        "SYS:SLP,T=%u,FT=%u,MSM=%u,SEC=%u\r\n",
        Clock_getTicks(),
        Clock_getFastTicks(),
        Clock_getMinutesSinceMidnight(),
        Clock_getSeconds()
    );

    // Send out all the data before going to sleep
    while (TRMT == 0);

    // Disable the FVR to conserve power
    uint8_t fvren = FVRCONbits.FVREN;
    FVRCONbits.FVREN = 0;

    context.sleep.wakeUpReason = System_WakeUpReason_None;

    SLEEP();

    // The next instruction will always be executed before the ISR

    FVRCONbits.FVREN = fvren;

    // Wait for the oscillator to stabilize
    while (OSCSTATbits.HFIOFS == 0);

    if (context.sleep.wakeUpReason == System_WakeUpReason_PowerInputChanged) {
        checkLDOSense();
        measureVDD();
        updateBatteryLevel();
    }
}

inline bool System_isRunningFromBackupBattery()
{
    return context.monitoring.onBatteryPower;
}

uint16_t System_getVDDMilliVolts()
{
    return (uint16_t)(
        Config_System_VDDCalMilliVolts
        * Config_System_VDDCalADCValue
        / System_interruptContext.adc.result
    );
}

uint16_t System_getVBatMilliVolts()
{
    return System_getVDDMilliVolts() + Config_System_VBatDiodeDropMilliVolts;
}

inline uint8_t System_getBatteryLevel()
{
    return context.monitoring.batteryLevel;
}