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
    Created on 2024-12-16
*/

#include "UserInterface.h"

#include "Clock.h"
#include "Config.h"
#include "OutputController.h"
#include "System.h"

#include <xc.h>

static struct {
    uint8_t ledLeaveOnAfterBlinking : 1;
    uint8_t ledBlinkCount : 3;
    uint8_t ledFastBlink : 1;
    uint8_t : 3;
    Clock_Ticks ledTicks;


} context = {
    .ledLeaveOnAfterBlinking = 0,
    .ledBlinkCount = 0,
    .ledFastBlink = 0,
    .ledTicks = 0
};

// Indicator LED control logic

static void LED_turnOn(void)
{
    context.ledBlinkCount = 0;
    INDICATOR_LED_OUTPUT = 1;
}

static void LED_turnOff(void)
{
    context.ledBlinkCount = 0;
    INDICATOR_LED_OUTPUT = 0;
}

static void LED_blink(const uint8_t count, const bool fast)
{
    if (count == 0) {
        INDICATOR_LED_OUTPUT = 0;
        return;
    }

    context.ledBlinkCount = count;
    context.ledFastBlink = fast;
    context.ledTicks = Clock_getFastTicks();
    INDICATOR_LED_OUTPUT = 1;
}

static void LED_task()
{
    if (context.ledBlinkCount > 0) {
        uint8_t interval = context.ledFastBlink ? 10 : 30;
        if (Clock_getFastTicks() - context.ledTicks >= interval) {
            if (!INDICATOR_LED_OUTPUT) {
                // Decrement only after a full on-off cycle
                --context.ledBlinkCount;
            }
            if (context.ledBlinkCount > 0 || context.ledLeaveOnAfterBlinking) {
                INDICATOR_LED_OUTPUT ^= 1;
            }
            context.ledTicks = Clock_getFastTicks();
        }
    }
}

// User interface logic

void UserInterface_init(void)
{
    LED_blink(1, false);
}

void UserInterface_runTasks(void)
{

}

void UserInterface_runLightweightTasks(void)
{
    LED_task();
}

bool UserInterface_hasPendingLightweightTasks(void)
{
    return context.ledBlinkCount > 0;
}

void UserInterface_buttonPressEvent(void)
{
    if (System_isRunningFromBackupBattery()) {
        LED_blink(3, true);
    } else {
        OutputController_toggle();
    }
}

inline void UserInterface_handleExternalEvent(const UI_ExternalEvent event)
{
    switch (event) {
        case UI_ExternalEvent_PowerInputChanged:
            if (System_isRunningFromBackupBattery()) {
                LED_blink(3, true);
            } else {
                if (!OutputController_isOutputEnabled()) {
                    LED_blink(1, false);
                }
            }
            break;

        case UI_ExternalEvent_OutputStateChanged:
            if (OutputController_isOutputEnabled()) {
                LED_turnOn();
            } else {
                LED_turnOff();
            }
            break;

        default:
            break;
    }
}
