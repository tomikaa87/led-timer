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

// Debug
#include "UI.h"

volatile uint16_t _System_adcResult = 0;

System_InterruptContext System_interruptContext = {
    .adc = {
        .result = 0,
        .updated = false
    },
    .ldoSense = {
        .updated = false
    },
    .externalWakeUpSource = false
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
        uint8_t batteryLevel;               // Estimated battery level: 0..10
    } monitoring;
} context = {
    .sleep = {
        .enabled = true,
        .lastWakeUpTime = 0,
        .wakeUpReason = System_WakeUpReason_None
    },
    .monitoring = {
        .lastUpdateTime = 0,
        .batteryLevel = 0,
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

#if DEBUG_ENABLE_PRINT
    printf(
        "SYS:VDD=%u(ADC=%u),VBat=%u,L=%u\r\n",
        System_getVDDMilliVolts(),
        System_interruptContext.adc.result,
        System_getVBatMilliVolts(),
        context.monitoring.batteryLevel
    );
#endif
}

void System_init()
{
    ADC_SelectChannel(channel_FVR);

    measureVDD();

    // Wait for the initial measurement to avoid invalid readings after startup
    while (!System_interruptContext.adc.updated);

    updateBatteryLevel();
}

System_TaskResult System_task()
{
    System_TaskResult result = {
        .action = System_TaskResult_NoActionNeeded,
        .powerInputChanged = System_interruptContext.ldoSense.updated
    };

    // To keep the screen on for a while after a power input change,
    // but avoid overwriting the existing reason (if any)
    if (
        context.sleep.wakeUpReason == System_WakeUpReason_None
        && result.powerInputChanged
    ) {
        System_onWakeUp(System_WakeUpReason_PowerInputChanged);
    }

    System_interruptContext.ldoSense.updated = false;

    if (
        result.powerInputChanged
        || Clock_getElapsedTicks(context.monitoring.lastUpdateTime)
            >= Config_System_MonitoringUpdateIntervalTicks
    ) {
        context.monitoring.lastUpdateTime = Clock_getTicks();
        measureVDD();
        updateBatteryLevel();
    }

    if (!context.sleep.enabled) {
        Clock_Ticks elapsedSinceWakeUp
            = Clock_getElapsedTicks(context.sleep.lastWakeUpTime);

        switch (context.sleep.wakeUpReason) {
            default:
            case System_WakeUpReason_None:
                context.sleep.enabled = System_isRunningFromBackupBattery();
                break;

            case System_WakeUpReason_Startup:
                if (elapsedSinceWakeUp >= Config_System_StartupAwakeLengthTicks) {
                    context.sleep.enabled = System_isRunningFromBackupBattery();
                }
                break;

            case System_WakeUpReason_KeyPress:
                if (elapsedSinceWakeUp >= Config_System_KeyPressWakeUpLengthTicks) {
                    context.sleep.enabled = System_isRunningFromBackupBattery();
                }
                break;

            case System_WakeUpReason_PowerInputChanged:
                if (elapsedSinceWakeUp >= Config_System_PowerInputChangeWakeUpLengthTicks) {
                    context.sleep.enabled = System_isRunningFromBackupBattery();
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
}

inline System_WakeUpReason System_getLastWakeUpReason()
{
    return context.sleep.wakeUpReason;
}

System_SleepResult System_sleep()
{
#if DEBUG_ENABLE_PRINT
    printf(
        "SYS:SLP,T=%u,FT=%u,MSM=%u,SEC=%u\r\n",
        Clock_getTicks(),
        Clock_getFastTicks(),
        Clock_getMinutesSinceMidnight(),
        Clock_getSeconds()
    );
#endif

    // Send out all the data before going to sleep
    while (UART1MD == 0 && TRMT == 0);

    // Disable the FVR to conserve power
    FVRCONbits.FVREN = 0;

    context.sleep.wakeUpReason = System_WakeUpReason_None;

#if DEBUG_ENABLE
    _DebugState.sleeping = true;
    _DebugState.externalWakeUp = false;
    _DebugState.ldoSenseValue = IO_LDO_SENSE_GetValue();
    UI_updateDebugDisplay();
#endif

    SLEEP();

    // The next instruction will always be executed before the ISR

    FVRCONbits.FVREN = 1;

    // Clear the flag so the next task() call can update it properly
    context.sleep.enabled = false;

#if DEBUG_ENABLE
    _DebugState.sleeping = false;
    _DebugState.externalWakeUp = System_interruptContext.externalWakeUpSource;
    _DebugState.ldoSenseValue = IO_LDO_SENSE_GetValue();
    UI_updateDebugDisplay();
#endif

    // Wait for the oscillator to stabilize
    while (
        OSCCON3bits.ORDY == 0
        || OSCSTAT1bits.HFOR == 0
    );

    if (System_interruptContext.externalWakeUpSource) {
        System_interruptContext.externalWakeUpSource = false;
        return System_SleepResult_WakeUpFromExternalSource;
    }

    return System_SleepResult_WakeUpFromInternalSource;
}

inline bool System_isRunningFromBackupBattery()
{
    return IO_LDO_SENSE_GetValue() == 1;
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