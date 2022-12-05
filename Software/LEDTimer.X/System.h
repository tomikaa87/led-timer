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
    System_WakeUpReason_KeyPress,
    System_WakeUpReason_PowerInputChanged
} System_WakeUpReason;

typedef enum
{
    System_TaskResult_NoActionNeeded,
    System_TaskResult_EnterSleepMode
} System_TaskResult_Action;

typedef struct
{
    System_TaskResult_Action action;
    bool powerInputChanged;
} System_TaskResult;

typedef volatile struct
{
    struct
    {
        uint16_t result;
        bool updated;
    } adc;

    struct
    {
        bool updated;
    } ldoSense;
} System_InterruptContext;

#define System_handleADCInterrupt(RESULT) { \
    extern volatile System_InterruptContext System_interruptContext; \
    System_interruptContext.adc.result = (RESULT); \
    System_interruptContext.adc.updated = true; \
}

#define System_handleLDOSenseInterrupt() { \
    extern volatile System_InterruptContext System_interruptContext; \
    System_interruptContext.ldoSense.updated = true; \
}

void System_init(void);
System_TaskResult System_task(void);

/**
 * This function must be called after a wake up event when it's caused by
 * an external event, letting the system know when to go to sleep mode again.
 * @param reason Reason why the system woke up from sleep.
 */
void System_onWakeUp(System_WakeUpReason reason);

/**
 * Returns the last reason why the system was woken up from sleep mode.
 * @return Reason why the system woke up from sleep.
 */
inline System_WakeUpReason System_getLastWakeUpReason();

/**
 * Puts the MCU into sleep mode. The system wakes up if there is an external
 * interrupt or the main clock timer (Timer1) overflows.
 */
void System_sleep(void);

/**
 * Indicates that the system is running on the backup battery by reading the
 * logic level of the LDO_SENSE input.
 * @return True: running from backup battery, false; running from main power
 */
inline bool System_isRunningFromBackupBattery();

/**
 * Returns the estimated VDD voltage of the MCU. It;s measured via the ADC
 * using the internal fixed voltage reference (FVR).
 * @return Voltage in millivolts.
 */
uint16_t System_getVDDMilliVolts(void);

/**
 * Returns the estimated voltage of the backup battery. It's calculated from
 * the measured VDD value by adding the forward voltage drop of the backup
 * battery diode. If the system is running from the main power supply, the
 * returned value is the estimated voltage of the LDO's output.
 * @return Voltage in millivolts.
 */
uint16_t System_getVBatMilliVolts(void);

/**
 * Returns the estimated charge level of the backup battery. The value is
 * calculated from VBat and because the discharge curve of a lithium cell
 * is far from linear, the value is only a rough indication of its state.
 * @return A value from 0 to 10.
 */
inline uint8_t System_getBatteryLevel(void);