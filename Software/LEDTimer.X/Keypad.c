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

#include "mcc_generated_files/device_config.h"
#include "mcc_generated_files/pin_manager.h"

#include <xc.h>

#include <stdbool.h>

static struct KeypadContext
{
    enum {
        State_Idle,
        State_KeyPressed,
        State_KeyHeld
    } state;

    Clock_Ticks timerTicks;
    uint8_t lastScanCode;
    bool coolDown;
} context = {
    .state = State_Idle,
    .timerTicks = 0,
    .lastScanCode = 0,
    .coolDown = false
};

static uint8_t scanKeys()
{
    uint8_t scanCode = 0;

    for (uint8_t i = 0; i < Config_Keypad_ScanSampleCount; ++i) {
        scanCode |=
            (IO_SW1_GetValue() ? 0 : (1 << 0))
            | (IO_SW2_GetValue() ? 0 : (1 << 1))
            | (IO_SW3_GetValue() ? 0 : (1 << 2));

        __delay_us(Config_Keypad_ScanSamplingDelayUs);
    }

    return scanCode;
}

// TODO implement the keypad handling logic used in pic-thermostat
// The current implementation makes hard to handle repeated presses
// of the same key

void Keypad_init()
{
}

uint8_t Keypad_task()
{
    uint8_t scanCode = scanKeys();

    // Simple de-bouncing logic
    if (
        context.coolDown
        && Clock_getElapsedFastTicks(context.timerTicks) < Config_Keypad_DeBounceCoolDownTicks
    ) {
        return 0;
    }

    context.coolDown = false;

    switch (context.state) {
        case State_Idle: {
            if (scanCode != 0) {
                context.state = State_KeyPressed;
                context.timerTicks = Clock_getFastTicks();
                context.lastScanCode = scanCode;
                return scanCode;
            }

            break;
        }

        case State_KeyPressed: {
            if (scanCode != context.lastScanCode) {
                context.state = State_Idle;
                context.coolDown = true;
                context.timerTicks = Clock_getFastTicks();
            } else {
                if (Clock_getElapsedFastTicks(context.timerTicks) >= Config_Keypad_HoldTimeoutTicks) {
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
                context.coolDown = true;
                context.timerTicks = Clock_getFastTicks();
            } else {
                return scanCode | (1 << 7);
            }

            break;
        }
    }

    return 0;
}