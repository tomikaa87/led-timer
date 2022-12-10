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
 *
 *      Settings screen (configuration state)
 *
 *      [K 1]   [K 2]   [K 3]
 *     [123456789012345678901]
 *
 *      BACK  |  UP   |  DOWN
 *      SETTING NAME      n/m
 *      [ setting details   ]
 *      [ setting details   ]
 *      [ setting details   ]
 *      [ setting details   ]
 *      [ setting details   ]
 *      [ setting details   ]
 *
 */

typedef enum {
    UI_Screen_Main,
    UI_Screen_Settings
} UI_Screen;

static struct UIContext {
    UI_Screen screen;
    bool displayOn;
    Clock_Ticks displayTimer;
    Clock_Ticks updateTimer;
    bool forceUpdate;
    volatile uint8_t externalEvents;
} context = {
    .screen = UI_Screen_Main,
    .displayOn = true,
    .updateTimer = 0,
    .forceUpdate = true,
    .externalEvents = 0
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
        puts("UI:wakeUp");

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

void UI_init()
{
    context.displayOn = true;
    context.displayTimer = Clock_getFastTicks();
    updateScreen(true);
}

void UI_task()
{
    // Check external events
    if (context.externalEvents & UI_ExternalEvent_SystemWakeUp) {
        // TODO
    }
    if (context.externalEvents & UI_ExternalEvent_PowerInputChanged) {
        wakeUpDisplay();
        context.forceUpdate = true;
    }
    if (context.externalEvents & UI_ExternalEvent_BatteryLevelMeasurementFinished) {
        context.forceUpdate = true;
    }
    context.externalEvents = 0;

    if (context.displayOn) {
        // Update the contents of the current screen periodically
        if (
            context.forceUpdate
            || (Clock_getElapsedFastTicks(context.updateTimer) >= Config_UI_UpdateIntervalTicks)
        ) {
            puts(context.forceUpdate ? "UI:forcedUpdate" : "UI:update");

            context.forceUpdate = false;
            context.updateTimer = Clock_getFastTicks();
            updateScreen(false);
        }

        // Turn off the display after the specified time
        if (
            Clock_getElapsedFastTicks(context.displayTimer)
                >= Config_UI_DisplayTimeoutTicks
        ) {
            puts("UI:displayOff");

            context.displayOn = false;
            SSD1306_setDisplayEnabled(false);
        }
    }
}

void UI_keyEvent(uint8_t keyCode)
{
    // Stop propagating to avoid executing more code in main()
    if (keyCode == 0) {
        return;
    }

    bool hold = !!(keyCode & Keypad_Hold);
    keyCode = keyCode & (Keypad_Key1 | Keypad_Key2 | Keypad_Key3);

#if 0
    printf("UI:keyEvent:%u %u\r\n", keyCode, hold);
#endif

    System_onWakeUp(System_WakeUpReason_KeyPress);

    if (wakeUpDisplay()) {
        return;
    }

    context.forceUpdate = true;

    switch (context.screen) {
        case UI_Screen_Main:
            if (!MainScreen_handleKeyPress(keyCode, hold)) {
                // Key1 -> Show Settings
                if (keyCode == Keypad_Key1) {
                    puts("UI:ShowSettings");
                    SettingsScreen_init();
                    switchToScreen(UI_Screen_Settings);
                }
            }
            break;

        case UI_Screen_Settings:
            if (!SettingsScreen_handleKeyPress(keyCode, hold)) {
                // Key1 -> Back to Main
                if (keyCode == Keypad_Key1) {
                    puts("UI:BackToMain");
                    switchToScreen(UI_Screen_Main);
                }
            }
            break;

        default:
            break;
    }
}

inline void UI_setExternalEvent(const UI_ExternalEvent event)
{
    context.externalEvents |= event;
}