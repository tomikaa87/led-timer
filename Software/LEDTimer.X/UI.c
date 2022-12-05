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
    Created on 2022-12-02
*/

#include "Clock.h"
#include "Config.h"
#include "Keypad.h"
#include "SSD1306.h"
#include "System.h"
#include "UI.h"

#include "MainScreen.h"
#include "SettingsScreen.h"

#include "stdbool.h"
#include "stdio.h"

/*
 *  Screen width = 21 chars (128 / (5 + 1))
 *
 *      Main screen
 *
 *      [K 1]   [K 2]   [K 3]
 *     [123456789012345678901]
 *
 *      STNGS |  OFF  |  ---
 *      brightness      power
 *        CLOCK    BULB
 *        CLOCK    BULB
 *        CLOCK    BULB
 *      ...Segment indicator..
 *      ...Segment bar........
 *      ...Hour numbers.......
 *
 *  Main screen key mapping:
 *      Key1: show Settings screen
 *      Key2: turn the output ON/OFF
 *      Key3: does nothing
 *
 *
 *      Settings screen (default state)
 *
 *      [K 1]   [K 2]   [K 3]
 *     [123456789012345678901]
 *
 *      EXIT  |  SEL  |  NEXT
 *      SETTING NAME      n/m
 *      [ setting details   ]
 *      [ setting details   ]
 *      [ setting details   ]
 *      [ setting details   ]
 *      [ setting details   ]
 *      [ setting details   ]
 *
 *  Settings screen (default) mapping:
 *      Key1: exit to main scren
 *
 */

typedef enum {
    UI_Screen_Main,
    UI_Screen_Settings
} UI_Screen;

static struct UIContext {
    UI_Screen screen;
    uint8_t lastKeyCode;
    Clock_Ticks keyRepeatTimer;
    bool displayOn;
    Clock_Ticks displayTimer;
    Clock_Ticks updateTimer;
    bool forceUpdate;
    bool lastKeyHandled;
} context = {
    .screen = UI_Screen_Main,
    .lastKeyCode = 0,
    .keyRepeatTimer = 0,
    .displayOn = true,
    .updateTimer = 0,
    .forceUpdate = true,
    .lastKeyHandled = false
};

static void updateScreen(const bool redraw)
{
    switch (context.screen) {
        case UI_Screen_Main:
            MainScreen_update(redraw);
            break;

        case UI_Screen_Settings:
            SettingsScreen_update(redraw);
            break;

        default:
            break;
    }
}

static bool wakeUpDisplay()
{
    context.displayTimer = Clock_getFastTicks();

    if (!context.displayOn) {
        puts("UI:wakeUpDisplay\r\n");

        context.displayOn = true;

        SSD1306_setDisplayEnabled(true);

        updateScreen(true);

        return true;
    }

    return false;
}

static void switchToScreen(const UI_Screen screen)
{
    context.screen = screen;
    SSD1306_clear();
    updateScreen(true);
}

static bool handleKeyPress(const uint8_t keyCode, const bool hold)
{
    printf("UI:handleKey:%u %u\r\n", keyCode, hold);

    System_onWakeUp(System_WakeUpReason_KeyPress);

    if (wakeUpDisplay()) {
        return true;
    }

    bool handled = false;

    switch (context.screen) {
        case UI_Screen_Main:
            if (!MainScreen_handleKeyPress(keyCode, hold)) {
                // Key1 -> Show Settings
                if (keyCode == Keypad_Key1) {
                    puts("UI:ShowSettings\r\n");
                    handled = true;
                    switchToScreen(UI_Screen_Settings);
                }
            }
            break;

        case UI_Screen_Settings:
            if (!SettingsScreen_handleKeyPress(keyCode, hold)) {
                // Key1 -> Back to Main
                if (keyCode == Keypad_Key1) {
                    puts("UI:BackToMain\r\n");
                    handled = true;
                    switchToScreen(UI_Screen_Main);
                }
            }
            break;

        default:
            break;
    }

    return handled;
}

void UI_init()
{
    context.displayOn = true;
    context.displayTimer = Clock_getFastTicks();
    updateScreen(true);
}

bool UI_task(const uint8_t keyCode)
{
    if (keyCode != 0) {
        if (keyCode != context.lastKeyCode) {
            context.lastKeyCode = keyCode;
            context.keyRepeatTimer = Clock_getFastTicks();
            context.lastKeyHandled = handleKeyPress(keyCode & 0b111, false);
        }

        if (
            (keyCode & Keypad_Hold)
            && Clock_getElapsedFastTicks(context.keyRepeatTimer) >= Config_UI_KeyRepeatIntervalTicks
        ) {
            context.keyRepeatTimer = Clock_getFastTicks();
            context.lastKeyHandled = handleKeyPress(keyCode & 0b111, true);
        }
    } else {
        context.lastKeyHandled = false;
    }

    if (
        context.displayOn
        && Clock_getElapsedFastTicks(context.displayTimer) >= Config_UI_DisplayTimeoutTicks
    ) {
        context.displayOn = false;
        SSD1306_setDisplayEnabled(false);
    }

    if (
        (
            context.displayOn
            && Clock_getElapsedFastTicks(context.updateTimer) >= Config_UI_UpdateIntervalTicks
        ) || context.forceUpdate
    ) {
        context.forceUpdate = false;
        context.updateTimer = Clock_getFastTicks();
        updateScreen(false);
    }

    // Key events only propagate from the Main screen
    return context.lastKeyHandled || context.screen != UI_Screen_Main;
}

void UI_onSystemWakeUp()
{
    puts("UI:wakeUp\r\n");

    if (System_getLastWakeUpReason() == System_WakeUpReason_PowerInputChanged) {
        UI_onPowerInputChanged();
    }
}

void UI_onPowerInputChanged()
{
    puts("UI:pwrChanged\r\n");

    wakeUpDisplay();
    context.forceUpdate = true;
}