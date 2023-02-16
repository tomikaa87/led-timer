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
#include "Settings_MenuScreen.h"
#include "SettingsScreen_Date.h"
#include "SettingsScreen_DisplayBrightness.h"
#include "SettingsScreen_LEDBrightness.h"
#include "SettingsScreen_Location.h"
#include "SettingsScreen_Scheduler.h"
#include "SettingsScreen_SegmentScheduler.h"
#include "SettingsScreen_Time.h"

#include "stdbool.h"
#include "stdio.h"
#include "string.h"

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
 *
 *      New scheduling settings
 *
 *      [K 1]   [K 2]   [K 3]
 *     [123456789012345678901]
 *
 *      BACK  | SELECT |  ADJ
 *      SETTING NAME      n/m
 *      TYPE: [ SIMPLE ]        [ SIMPLE ] [ SEGMENT ] [ OFF ]
 *
 *      ON: [ SUNSET ]          [ TIME ] [ SUNSET ] [ SUNRISE ]
 *      <SUNSET>: +/- XX MIN    [ max. +/- 60 min, 15 min step ]
 *      OFF: [ TIME ]           [ TIME ] [ SUNSET ] [ SUNRISE ]
 *      <TIME>: XX:XX
 *
 *
 *
 *
 *
 */

typedef enum {
    UI_Screen_Main,
    UI_Screen_Settings,
    UI_Screen_Settings_Scheduler,
    UI_Screen_Settings_SegmentScheduler,
    UI_Screen_Settings_LEDBrightness,
    UI_Screen_Settings_DisplayBrightness,
    UI_Screen_Settings_Date,
    UI_Screen_Settings_Time,
    UI_Screen_Settings_Location
} UI_Screen;

static struct UIContext {
    UI_Screen screen;
    Clock_Ticks displayTimer;
    Clock_Ticks updateTimer;
    volatile uint8_t externalEvents;
    uint8_t displayOn : 1;
    uint8_t forceUpdate : 1;
    uint8_t _reserved : 5;
    SettingsData modifiedSettings;
} context = {
    .screen = UI_Screen_Main,
    .updateTimer = 0,
    .externalEvents = 0,
    .displayOn = true,
    .forceUpdate = true
};

#if DEBUG_ENABLE
DebugState _DebugState;

void UI_updateDebugDisplay()
{
    static const uint8_t Page = 1;

    SSD1306_enablePageAddressing();
    SSD1306_setStartColumn(0);
    SSD1306_setPage(Page);

    // First column pixels:
    //  0: sleeping (white = true)

    uint8_t testData[2] = { 0 };

    if (++_DebugState.updateValue >= 4) {
        _DebugState.updateValue = 0;
    }

    if (_DebugState.heavyTaskUpdateValue >= 4) {
        _DebugState.heavyTaskUpdateValue = 0;
    }

    testData[0] =
        (_DebugState.sleeping            ? (1 << 0) : 0)
        | (_DebugState.externalWakeUp    ? (1 << 1) : 0)
        | (_DebugState.ldoSenseValue     ? (1 << 2) : 0)
        | (uint8_t)((_DebugState.heavyTaskUpdateValue & 0b11) << 4)
        | (uint8_t)((_DebugState.updateValue & 0b11) << 6)
        ;

    testData[1] =
        (_DebugState.oc_stateFromSchedule           ? (1 << 0) : 0)
        | (_DebugState.oc_prevStateFromSchedule     ? (1 << 1) : 0)
        | (_DebugState.oc_outputOverride            ? (1 << 2) : 0)
        | (_DebugState.oc_outputState               ? (1 << 3) : 0)
        | (_DebugState.oc_forceUpdate               ? (1 << 4) : 0)
        ;

    for (uint8_t i = 0; i < sizeof(testData) * 2; ++i) {
        // Every other column is a separator
        if ((i & 1) != 0) {
            const uint8_t FF = 0xff;
            SSD1306_sendData(&FF, 1, 0, false);
        } else {
            SSD1306_sendData(testData + (i >> 1), 1, 0, false);
        }
    }
}
#endif

static void updateScreen(const bool redraw)
{
    switch (context.screen) {
        case UI_Screen_Main:
            MainScreen_update(redraw);
            break;

        case UI_Screen_Settings:
            // SettingsScreen_update(redraw);
            Settings_MenuScreen_update(redraw);
            break;

        case UI_Screen_Settings_Scheduler:
            SettingsScreen_Scheduler_update(redraw);
            break;

        case UI_Screen_Settings_SegmentScheduler:
            SettingsScreen_SegmentScheduler_update(redraw);
            break;

        case UI_Screen_Settings_LEDBrightness:
            SettingsScreen_LEDBrightness_update(redraw);
            break;

        case UI_Screen_Settings_DisplayBrightness:
            SettingsScreen_DisplayBrightness_update(redraw);
            break;

        case UI_Screen_Settings_Date:
            SettingsScreen_Date_update(redraw);
            break;

        case UI_Screen_Settings_Time:
            SettingsScreen_Time_update(redraw);
            break;

        case UI_Screen_Settings_Location:
            SettingsScreen_Location_update(redraw);
            break;

        default:
            break;
    }

#if DEBUG_ENABLE
    UI_updateDebugDisplay();
#endif
}

static bool wakeUpDisplay()
{
    context.displayTimer = Clock_getFastTicks();

    if (!context.displayOn) {
#if DEBUG_ENABLE_PRINT
        puts("UI:wakeUp");
#endif

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

#if DEBUG_ENABLE
    memset((void *)&_DebugState, 0, sizeof(DebugState));
#endif
}

void UI_task()
{
    // Check external events
    if (context.externalEvents & UI_ExternalEvent_SystemWakeUp) {
        // TODO handle wake up events
    }
    if (context.externalEvents & UI_ExternalEvent_PowerInputChanged) {
        wakeUpDisplay();
        context.forceUpdate = true;
    }
    if (context.externalEvents & UI_ExternalEvent_BatteryLevelMeasurementFinished) {
        context.forceUpdate = true;
    }
    if (context.externalEvents & UI_ExternalEvent_OutputStateChanged) {
        context.forceUpdate = true;
    }
    if (context.externalEvents & UI_ExternalEvent_SystemGoingToSleep) {
        // Turn off the display immediately to conserve power
        if (context.displayOn) {
#if DEBUG_ENABLE_PRINT
            puts("UI:displayOff(->sleep)");
#endif

            context.displayOn = false;
#if !DEBUG_ENABLE
            SSD1306_setDisplayEnabled(false);
#endif
        }

        context.externalEvents = 0;

        return;
    }
    context.externalEvents = 0;

    if (context.displayOn) {
        // Update the contents of the current screen periodically
        if (
            context.forceUpdate
            || (Clock_getElapsedFastTicks(context.updateTimer) >= Config_UI_UpdateIntervalTicks)
        ) {
#if DEBUG_ENABLE_PRINT
            puts(context.forceUpdate ? "UI:forcedUpdate" : "UI:update");
#endif

            context.forceUpdate = false;
            context.updateTimer = Clock_getFastTicks();
            updateScreen(false);
        }

        // Turn off the display after the specified time
        if (
            Clock_getElapsedFastTicks(context.displayTimer)
                >= Config_UI_DisplayTimeoutTicks
        ) {
#if DEBUG_ENABLE_PRINT
            puts("UI:displayOff");
#endif

            context.displayOn = false;
#if !DEBUG_ENABLE
            SSD1306_setDisplayEnabled(false);
#endif
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

#if DEBUG_ENABLE_PRINT
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
#if DEBUG_ENABLE_PRINT
                    puts("UI:ShowSettings");
#endif
                    // SettingsScreen_init();
                    Settings_MenuScreen_init();
                    memcpy(&context.modifiedSettings, &Settings_data, sizeof(SettingsData));
                    switchToScreen(UI_Screen_Settings);
                }
            }
            break;

        case UI_Screen_Settings: {
            switch (Settings_MenuScreen_handleKeyPress(keyCode, hold)) {
                case Settings_MenuScreen_KeyHandled:
                    break;

                case Settings_MenuScreen_Exited:
                    // Save settings
                    memcpy(&Settings_data, &context.modifiedSettings, sizeof(SettingsData));
                    SSD1306_setContrastLevel(Settings_data.display.brightness);
                    Settings_save();

#if DEBUG_ENABLE_PRINT
                    puts("UI:BackToMain");
#endif
                    switchToScreen(UI_Screen_Main);
                    break;

                case Settings_MenuScreen_ItemSelected:
                    // Switch to the selected settings screen
                    switch (Settings_MenuScreen_lastSelectionIndex()) {
                        case 0:
                            SettingsScreen_Scheduler_init(&context.modifiedSettings.scheduler);
                            switchToScreen(UI_Screen_Settings_Scheduler);
                            break;
                        case 1:
                            SettingsScreen_SegmentScheduler_init(&context.modifiedSettings.scheduler);
                            switchToScreen(UI_Screen_Settings_SegmentScheduler);
                            break;
                        case 2:
                            SettingsScreen_LEDBrightness_init(&context.modifiedSettings.output);
                            switchToScreen(UI_Screen_Settings_LEDBrightness);
                            break;
                        case 3:
                            SettingsScreen_DisplayBrightness_init(&context.modifiedSettings.display);
                            switchToScreen(UI_Screen_Settings_DisplayBrightness);
                            break;
                        case 4:
                            SettingsScreen_Date_init();
                            switchToScreen(UI_Screen_Settings_Date);
                            break;
                        case 5:
                            SettingsScreen_Time_init();
                            switchToScreen(UI_Screen_Settings_Time);
                            break;
                        case 6:
                            SettingsScreen_Location_init(&context.modifiedSettings.location);
                            switchToScreen(UI_Screen_Settings_Location);
                            break;
                        default:
                            break;
                    }
                    break;
            }
            break;
        }

        case UI_Screen_Settings_Scheduler:
            if (!SettingsScreen_Scheduler_handleKeyPress(keyCode, hold)) {
                switchToScreen(UI_Screen_Settings);
            }
            break;

        case UI_Screen_Settings_SegmentScheduler:
            if (!SettingsScreen_SegmentScheduler_handleKeyPress(keyCode, hold)) {
                switchToScreen(UI_Screen_Settings);
            }
            break;

        case UI_Screen_Settings_LEDBrightness:
            if (!SettingsScreen_LEDBrightness_handleKeyPress(keyCode, hold)) {
                switchToScreen(UI_Screen_Settings);
            }
            break;

        case UI_Screen_Settings_DisplayBrightness:
            if (!SettingsScreen_DisplayBrightness_handleKeyPress(keyCode, hold)) {
                switchToScreen(UI_Screen_Settings);
            }
            break;

        case UI_Screen_Settings_Date:
            if (!SettingsScreen_Date_handleKeyPress(keyCode, hold)) {
                switchToScreen(UI_Screen_Settings);
            }
            break;

        case UI_Screen_Settings_Time:
            if (!SettingsScreen_Time_handleKeyPress(keyCode, hold)) {
                switchToScreen(UI_Screen_Settings);
            }
            break;

        case UI_Screen_Settings_Location:
            if (!SettingsScreen_Location_handleKeyPress(keyCode, hold)) {
                switchToScreen(UI_Screen_Settings);
            }
            break;
#if 0
        case UI_Screen_Settings_:
            break;
        case UI_Screen_Settings_:
            break;
        case UI_Screen_Settings_:
            break;
        case UI_Screen_Settings_:
            break;
        case UI_Screen_Settings_:
            break;
        case UI_Screen_Settings_:
            break;
#endif

        default:
            break;
    }
}

inline void UI_setExternalEvent(const UI_ExternalEvent event)
{
    context.externalEvents |= event;
}