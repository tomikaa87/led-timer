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
    Created on 2023-01-31
*/

#include "Graphics.h"
#include "Keypad.h"
#include "Settings_MenuScreen.h"
#include "SSD1306.h"
#include "Text.h"
#include "Config.h"

#include <string.h>

static const char* MenuItems[] = {
    "SCHEDULER",
    "SEGMENT SCHEDULER",
    "LED BRIGHTNES",
    "DISP. BRIGHTNESS",
    "DATE",
    "TIME",
    "TIME ZONE",
    "DST",
#if !SUNRISE_SUNSET_USE_LUT
    "LOCATION"
#endif
};

#define MenuItemCount   (sizeof(MenuItems) / sizeof(char*))

static const uint8_t PositionIndicatorEmpty[] = {
    0b10101010,
    0b01010101,
    0b10101010
};

static const uint8_t PositionIndicatorFull[] = {
    0b11111111,
    0b11111111,
    0b11111111
};

static struct Settings_MenuScreen_Context {
    uint8_t selectionIndex : 4;
    uint8_t viewPosition : 4;
} context;

static void drawMenuItems()
{
    uint8_t itemIndex = context.viewPosition;
    uint8_t line = 1;

    while (itemIndex < MenuItemCount && line != 7) {
        if (itemIndex == context.selectionIndex) {
            Graphics_DrawIcon(0, line, Graphics_ArrowRightIcon);
        } else {
            SSD1306_fillArea(0, line, sizeof(Graphics_ArrowRightIcon), 1, SSD1306_COLOR_BLACK);
        }

        uint8_t x = Text_draw(MenuItems[itemIndex], line, sizeof(Graphics_ArrowRightIcon) + 2, 0, false);
        if (x < (127 - sizeof(PositionIndicatorEmpty))) {
            SSD1306_fillArea(x, line, 127 - x - sizeof(PositionIndicatorEmpty), 1, SSD1306_COLOR_BLACK);
        }

        ++line;
        ++itemIndex;
    }
}

static void drawPositionIndicator()
{
    #define MaxPosition 4

    uint8_t position = (context.selectionIndex + 1) * MaxPosition / MenuItemCount;

    for (uint8_t i = 0; i <= MaxPosition; ++i) {
        if (i == position) {
            Graphics_DrawRightIcon(i + 1, PositionIndicatorFull);
        } else {
            Graphics_DrawRightIcon(i + 1, PositionIndicatorEmpty);
        }
    }
}

void Settings_MenuScreen_init()
{
    context.selectionIndex = 0;
    context.viewPosition = 0;
}

void Settings_MenuScreen_update(const bool redraw)
{
    if (redraw) {
        Graphics_DrawScreenTitle("SETTINGS");
        Graphics_DrawKeypadHelpBar(Graphics_ExitIcon, Graphics_ArrowDownIcon, Graphics_SelectIcon);

        drawMenuItems();
        drawPositionIndicator();
    }
}

Settings_MenuScreen_KeyPressResult Settings_MenuScreen_handleKeyPress(const uint8_t keyCode, const bool hold)
{
    switch (keyCode) {
        // Exit
        case Keypad_Key1: {
            if (hold) {
                break;
            }

            return Settings_MenuScreen_Exited;
        }

        // Next
        case Keypad_Key2: {
            if (++context.selectionIndex == MenuItemCount) {
                context.selectionIndex = 0;
                context.viewPosition = 0;
            }

            if (context.selectionIndex > 5) {
                ++context.viewPosition;
            }

            drawMenuItems();
            drawPositionIndicator();

            break;
        }

        // Select
        case Keypad_Key3: {
            if (hold) {
                break;
            }

            return Settings_MenuScreen_ItemSelected;
        }
    }

    return Settings_MenuScreen_KeyHandled;
}

uint8_t Settings_MenuScreen_lastSelectionIndex()
{
    return context.selectionIndex;
}