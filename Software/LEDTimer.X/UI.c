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
#include "Keypad.h"
#include "SSD1306.h"
#include "System.h"
#include "UI.h"

#include "MainScreen.h"

#include "stdbool.h"
#include "stdio.h"

#define KeyRepeatIntervalTicks      (10)
#define DisplayTimeoutTicks         (1000)
#define UpdateIntervalTicks         (100)

typedef enum {
    UI_Screen_Main
} UI_Screen;

static struct UIContext {
    UI_Screen screen;
    bool redrawScreen;
    uint8_t lastKeyCode;
    Clock_Ticks keyRepeatTimer;
    bool displayOn;
    Clock_Ticks displayTimer;
    Clock_Ticks updateTimer;
} context = {
    .screen = UI_Screen_Main,
    .redrawScreen = true,
    .lastKeyCode = 0,
    .keyRepeatTimer = 0,
    .displayOn = true,
    .updateTimer = 0
};

static void updateScreen()
{
    switch (context.screen) {
        case UI_Screen_Main:
            MainScreen_update(context.redrawScreen);
            break;

        default:
            break;
    }

    context.redrawScreen = false;
}

static void handleKeyPress(const uint8_t keyCode)
{
    if (keyCode != 0) {
        System_onWakeUp(System_WakeUpReason_KeyPress);

        context.displayTimer = Clock_getFastTicks();

        if (!context.displayOn) {
            context.displayOn = true;
            context.redrawScreen = true;

            SSD1306_setDisplayEnabled(true);

            updateScreen();

            return;
        }
    }

    switch (keyCode) {
        case Keypad_Key1:
            printf("UI:KP:1\r\n");
            break;

        case Keypad_Key2:
            printf("UI:KP:2\r\n");
            break;

        case Keypad_Key3:
            printf("UI:KP:3\r\n");
            break;

        default:
            break;
    }
}

void UI_init()
{
    context.redrawScreen = true;
    context.displayOn = true;
    context.displayTimer = Clock_getFastTicks();
    updateScreen();
}

void UI_task(const uint8_t keyCode)
{
    if (keyCode != context.lastKeyCode) {
        context.lastKeyCode = keyCode;
        context.keyRepeatTimer = Clock_getFastTicks();

        handleKeyPress(keyCode & 0b111);
    }

    if (
        (keyCode & Keypad_Hold)
        && Clock_getElapsedFastTicks(context.keyRepeatTimer) >= KeyRepeatIntervalTicks
    ) {
        context.keyRepeatTimer = Clock_getFastTicks();
        handleKeyPress(keyCode & 0b111);
    }

    if (
        context.displayOn
        && Clock_getElapsedFastTicks(context.displayTimer) >= DisplayTimeoutTicks
    ) {
        context.displayOn = false;
        SSD1306_setDisplayEnabled(false);
    }

    if (
        context.displayOn
        && Clock_getElapsedFastTicks(context.updateTimer) >= UpdateIntervalTicks
    ) {
        context.updateTimer = Clock_getFastTicks();
        updateScreen();
    }
}