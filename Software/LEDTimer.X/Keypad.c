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

#include "mcc_generated_files/device_config.h"
#include "mcc_generated_files/pin_manager.h"

#include <xc.h>

#define ScanSampleCount         (3)
#define ScanSamplingDelayUs     (1)
#define HoldTimeoutTicks        (50)

static struct KeypadContext
{
    enum {
        State_Idle,
        State_KeyPressed,
        State_KeyHeld
    } state;

    Clock_Ticks pressTicks;
    uint8_t lastScanCode;
} context = {
    .state = State_Idle,
    .pressTicks = 0,
    .lastScanCode = 0
};

static void keyPressInterruptHandler()
{
    System_wakeUp(System_WakeUpReason_KeyPress);
}

static uint8_t scanKeys()
{
    uint8_t scanCode = 0;

    for (uint8_t i = 0; i < ScanSampleCount; ++i) {
        scanCode |=
            (IO_SW1_GetValue() ? 0 : (1 << 0))
            | (IO_SW2_GetValue() ? 0 : (1 << 1))
            | (IO_SW3_GetValue() ? 0 : (1 << 2));

        __delay_us(ScanSamplingDelayUs);
    }

    return scanCode;
}

void Keypad_init()
{
    IOCAF0_SetInterruptHandler(keyPressInterruptHandler);
    IOCAF1_SetInterruptHandler(keyPressInterruptHandler);
}

uint8_t Keypad_task()
{
    uint8_t scanCode = scanKeys();

    // Prevent sleeping if the keypad is in use
    if (scanCode != 0) {
        System_wakeUp(System_WakeUpReason_KeyPress);
    }

    switch (context.state) {
        case State_Idle: {
            if (scanCode != 0) {
                context.state = State_KeyPressed;
                context.pressTicks = Clock_getFastTicks();
                context.lastScanCode = scanCode;
                return scanCode;
            }

            break;
        }

        case State_KeyPressed: {
            if (scanCode != context.lastScanCode) {
                context.state = State_Idle;
            } else {
                if (Clock_getElapsedFastTicks(context.pressTicks) >= HoldTimeoutTicks) {
                    context.state = State_KeyHeld;
                    return scanCode | (1 << 7);
                } else {
                    return scanCode;
                }
            }

            break;
        }

        case State_KeyHeld: {
            if (scanCode != context.lastScanCode) {
                context.state = State_Idle;
            } else {
                return scanCode | (1 << 7);
            }

            break;
        }
    }

    return 0;
}