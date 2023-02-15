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

#include "Clock.h"
#include "Graphics.h"
#include "Keypad.h"
#include "SettingsScreen_Date.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static struct SettingScreen_Date_Context {
    uint8_t yearFrom2022;
    uint8_t month;
    uint8_t day;
    uint8_t selectionIndex;
} context;

void SettingsScreen_Date_init()
{
    memset(&context, 0, sizeof(struct SettingScreen_Date_Context));
}

void SettingsScreen_Date_update(const bool redraw)
{
    if (redraw) {
        Graphics_DrawScreenTitle("Date");
        // TODO use -> for selection change icon
        Graphics_DrawKeypadHelpBar(Graphics_ExitIcon, Graphics_NextIcon, Graphics_AdjustIcon);
    }

    #define CharWidth 12u
    #define CharSpacing 2u
    #define TotalWidth (8u * CharWidth + 15u * CharSpacing)
    #define ExtraSpacing (CharSpacing * 4)
    #define LinePattern 0b00000110

    char s[8];
    uint8_t x = 64 - TotalWidth / 2;
    uint8_t xPrev = x;

    // Year
    sprintf(s, "%04u", (uint16_t)context.yearFrom2022 + 2022);
    x = Text_draw7Seg(s, 2, x, false);
    SSD1306_fillAreaPattern(xPrev, 5, x - xPrev, 1, context.selectionIndex == 0 ? LinePattern : 0);
    x += ExtraSpacing;

    // Month
    sprintf(s, "%02u", context.month + 1);
    xPrev = x;
    x = Text_draw7Seg(s, 2, x, false);
    SSD1306_fillAreaPattern(xPrev, 5, x - xPrev, 1, context.selectionIndex == 1 ? LinePattern : 0);
    x += ExtraSpacing - 1 /* to fit the last 2 numbers on the screen */;

    // Day
    sprintf(s, "%02u", context.day + 1);
    xPrev = x;
    x = Text_draw7Seg(s, 2, x, false);
    SSD1306_fillAreaPattern(xPrev, 5, x - xPrev, 1, context.selectionIndex == 2 ? LinePattern : 0);
}

bool SettingsScreen_Date_handleKeyPress(const uint8_t keyCode, const bool hold)
{
    switch (keyCode) {
        // Exit
        case Keypad_Key1: {
            if (hold) {
                break;
            }

            // TODO save the date

            return false;
        }

        // Change selection
        case Keypad_Key2: {
            if (++context.selectionIndex > 2) {
                context.selectionIndex = 0;
            }
            SettingsScreen_Date_update(false);
            break;
        }

        // Minutes
        case Keypad_Key3: {
            switch (context.selectionIndex) {
                case 0:
                    // Max year = 2050
                    if (++context.yearFrom2022 > 28) {
                        context.yearFrom2022 = 0;
                    }
                    break;

                case 1:
                    if (++context.month > 11) {
                        context.month = 0;
                    }
                    break;

                case 2:
                    // TODO use the correct month length
                    if (++context.day > 30) {
                        context.day = 0;
                    }
                    break;

                default:
                    break;
            }
            SettingsScreen_Date_update(false);
            break;
        }
    }

    return true;
}
