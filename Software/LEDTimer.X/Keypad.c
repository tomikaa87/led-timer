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
#include "Keypad.h"

#include "mcc_generated_files/device_config.h"
#include "mcc_generated_files/pin_manager.h"

#include <xc.h>

#include <stdbool.h>

static struct KeypadContext
{
    enum {
        State_Idle,
        State_KeyPressed,
        State_KeyRepeat
    } state;

    Clock_Ticks pressTimerTicks;
    Clock_Ticks delayTimerTicks;
    Clock_Ticks delayTimeoutTicks;

    uint8_t lastScanCode;
} context = {
    .state = State_Idle,
    .pressTimerTicks = 0,
    .delayTimerTicks = 0,
    .delayTimeoutTicks = 0,
    .lastScanCode = 0
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

void Keypad_init()
{
}

uint8_t Keypad_task()
{
    uint8_t scanCode = scanKeys();

    // Simple de-bouncing logic
    if (
        context.delayTimeoutTicks != 0
        && Clock_getElapsedFastTicks(context.delayTimerTicks)
            < context.delayTimeoutTicks
    ) {
        return 0;
    }

    context.delayTimeoutTicks = 0;

    switch (context.state) {
        case State_Idle: {
            if (scanCode != 0) {
                context.state = State_KeyPressed;
                context.pressTimerTicks = Clock_getFastTicks();
                context.lastScanCode = scanCode;
                return scanCode;
            }

            break;
        }

        case State_KeyPressed: {
            if (scanCode != context.lastScanCode) {
                context.state = State_Idle;
                context.delayTimerTicks = Clock_getFastTicks();
                context.delayTimeoutTicks = Config_Keypad_DelayAfterKeysChangedTicks;
            } else {
                if (
                    Clock_getElapsedFastTicks(context.pressTimerTicks)
                        >= Config_Keypad_RepeatTimeoutTicks
                ) {
                    context.state = State_KeyRepeat;
                    context.delayTimerTicks = Clock_getFastTicks();
                    context.delayTimeoutTicks = Config_Keypad_RepeatIntervalTicks;
                    return scanCode | Keypad_Hold;
                }
            }

            break;
        }

        case State_KeyRepeat: {
            if (scanCode != context.lastScanCode) {
                context.state = State_Idle;
                context.delayTimerTicks = Clock_getFastTicks();
                context.delayTimeoutTicks = Config_Keypad_DelayAfterKeysChangedTicks;
            } else {
                context.delayTimerTicks = Clock_getFastTicks();
                context.delayTimeoutTicks = Config_Keypad_RepeatIntervalTicks;
                return scanCode | Keypad_Hold;
            }

            break;
        }
    }

    return 0;
}