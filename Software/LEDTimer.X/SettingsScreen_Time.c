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
#include "SettingsScreen_Time.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static struct SettingScreen_Time_Context {
    uint8_t minutes;
    uint8_t hours : 5;
    uint8_t clockAdjusted : 1;
    uint8_t selectionIndex : 1;
} context;

void SettingsScreen_Time_init()
{
    context.hours = Clock_getHour();
    context.minutes = Clock_getMinute();
    context.clockAdjusted = 0;
    context.selectionIndex = 0;
}

void SettingsScreen_Time_update(const bool redraw)
{
    if (redraw) {
        Graphics_DrawScreenTitle("TIME");
        Graphics_DrawKeypadHelpBar(Graphics_ExitIcon, Graphics_ArrowRightIcon, Graphics_AdjustIcon);
    }

    #define CharWidth 12u
    #define CharSpacing 2u
    #define ColonWidth 4u
    #define TotalWidth (4u * CharWidth + 1u * ColonWidth + 4u * CharSpacing)
    #define LinePattern 0b00000110

    char s[6];
    uint8_t x = 64u - TotalWidth / 2u;
    uint8_t xPrev = x;

    // Hour
    sprintf(s, "%02u", context.hours);
    x = Text_draw7Seg(s, 2, x, false);
    SSD1306_fillAreaPattern(xPrev, 5, x - xPrev, 1, context.selectionIndex == 0 ? LinePattern : 0);
    x += CharSpacing;

    // Separator
    x = Text_draw7Seg(":", 2, x, false);
    x += CharSpacing;

    // Minute
    sprintf(s, "%02u", context.minutes);
    xPrev = x;
    x = Text_draw7Seg(s, 2, x, false);
    SSD1306_fillAreaPattern(xPrev, 5, x - xPrev, 1, context.selectionIndex == 1 ? LinePattern : 0);
}

bool SettingsScreen_Time_handleKeyPress(const uint8_t keyCode, const bool hold)
{
    switch (keyCode) {
        // Exit
        case Keypad_Key1: {
            if (hold) {
                break;
            }

            // Apply changes immediately to keep the time accurate
            Clock_setTime(context.hours, context.minutes);

            return false;
        }

        // Change selection
        case Keypad_Key2: {
            ++context.selectionIndex;
            SettingsScreen_Time_update(false);
            break;
        }

        // Minutes
        case Keypad_Key3: {
            switch (context.selectionIndex) {
                case 0:
                    if (++context.hours >= 24) {
                        context.hours = 0;
                    }
                    break;

                case 1:
                    if (++context.minutes >= 60) {
                        context.minutes = 0;
                    }
                    break;

                default:
                    break;
            }
            context.clockAdjusted = true;
            SettingsScreen_Time_update(false);
            break;
        }
    }

    return true;
}
