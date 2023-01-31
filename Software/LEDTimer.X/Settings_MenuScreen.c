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

#include "Keypad.h"
#include "Settings_MenuScreen.h"
#include "SSD1306.h"
#include "Text.h"

#include <string.h>

#define MenuItemCount   7

static const char* MenuItems[MenuItemCount] = {
    "Scheduler",
    "Segment sch.",
    "LED brightness",
    "Disp. brightness",
    "Date",
    "Time",
    "Location"
};

static const uint8_t HeaderLeftCap[] = {
    0b01000000,
    0b00100000,
    0b01010000,
    0b00101000,
    0b01010100,
    0b00101010,
    0b01010101
};

static const uint8_t HeaderRightCap[] = {
    0b01010101,
    0b00101010,
    0b00010101,
    0b00001010,
    0b00000101,
    0b00000010,
    0b00000001
};

static const uint8_t ExitIcon[] = {
    0b11111111,
    0b00000001,
    0b00000001,
    0b00010001,
    0b11111111,
    0b00000000,
    0b00010000,
    0b00111000,
    0b01111100,
    0b00010000,
    0b00010000,
    0b00010000
};

static const uint8_t NextIcon[] = {
    0b00010000,
    0b00110000,
    0b01110000,
    0b11111111,
    0b01110000,
    0b00110000,
    0b00010000
};

static const uint8_t SelectIcon[] = {
    0b00010000,
    0b00111000,
    0b01111100,
    0b11111110,
    0b00010000,
    0b00010000,
    0b00010000,
    0b00011111
};

static const uint8_t SelectedItemIcon[] = {
    0b00001000,
    0b00001000,
    0b00001000,
    0b01111111,
    0b00111110,
    0b00011100,
    0b00001000
};

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

#define DrawIcon(_Column, _Line, _Icon) \
    SSD1306_enablePageAddressing(); \
    SSD1306_setStartColumn((_Column)); \
    SSD1306_setPage((_Line)); \
    SSD1306_sendData((_Icon), sizeof((_Icon)), 0, false)

#define DrawCenterIcon(_Line, _Icon) \
    DrawIcon(64 - sizeof((_Icon)) / 2, _Line, _Icon)

#define DrawRightIcon(_Line, _Icon) \
    DrawIcon(128 - sizeof((_Icon)), _Line, _Icon)

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
            DrawIcon(0, line, SelectedItemIcon);
        } else {
            SSD1306_fillArea(0, line, sizeof(SelectedItemIcon), 1, SSD1306_COLOR_BLACK);
        }

        uint8_t x = Text_draw(MenuItems[itemIndex], line, sizeof(SelectedItemIcon) + 2, 0, false);
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
            DrawRightIcon(i + 1, PositionIndicatorFull);
        } else {
            DrawRightIcon(i + 1, PositionIndicatorEmpty);
        }
    }
}

void Settings_MenuScreen_init()
{
    memset(&context, 0, sizeof(struct Settings_MenuScreen_Context));
}

void Settings_MenuScreen_update(const bool redraw)
{
    if (redraw) {
        // Header
        const uint8_t tw = CalculateTextWidth("Settings");
        uint8_t x = 64 - tw / 2 - sizeof(HeaderLeftCap) - 1;
        DrawIcon(x, 0, HeaderLeftCap);
        x = Text_draw("Settings", 0, x + sizeof(HeaderLeftCap) + 1, 0, false);
        DrawIcon(x, 0, HeaderRightCap);

        // Bottom help bar
        DrawIcon(0, 7, ExitIcon);
        DrawIcon(32 - sizeof(HeaderLeftCap), 7, HeaderLeftCap);
        DrawIcon(32 - 1, 7, HeaderRightCap);
        DrawCenterIcon(7, NextIcon);
        DrawIcon(96 - sizeof(HeaderLeftCap), 7, HeaderLeftCap);
        DrawIcon(96 - 1, 7, HeaderRightCap);
        DrawRightIcon(7, SelectIcon);

        drawMenuItems();
        drawPositionIndicator();
    }
}

bool Settings_MenuScreen_handleKeyPress(const uint8_t keyCode, const bool hold)
{
    switch (keyCode) {
        // Exit
        case Keypad_Key1: {
            // Save settings
            break;
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

            return true;
        }

        // Select
        case Keypad_Key3: {

            break;
        }
    }

    return false;
}