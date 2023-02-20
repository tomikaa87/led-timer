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
#include "SettingsScreen_TimeZone.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct SettingScreen_TimeZone_Context {
    struct Time* settings;
} context;

void SettingsScreen_TimeZone_init(struct Time* settings)
{
    memset(&context, 0, sizeof(struct SettingScreen_TimeZone_Context));
    context.settings = settings;
}

void SettingsScreen_TimeZone_update(const bool redraw)
{
    if (redraw) {
        Graphics_DrawScreenTitle("TIME ZONE");
        Graphics_DrawKeypadHelpBarLeftRight(Graphics_ExitIcon, Graphics_AdjustIcon);
    }

    #define CharWidth 12u
    #define CharSpacing 2u
    #define ColonWidth 4u
    #define SignWidth 7u
    #define TotalWidth (4u * CharWidth + 1u * ColonWidth + 1u * SignWidth + 5u * CharSpacing)
    #define LinePattern 0b00000110

    char s[7];
    uint8_t x = 64u - TotalWidth / 2u;
    uint8_t xPrev = x;

    // Time zone
    sprintf(
        s,
        "%c%02u:%02u",
        context.settings->timeZoneOffsetHalfHours >= 0 ? '+' : '-',
        abs(context.settings->timeZoneOffsetHalfHours / 2),
        abs(context.settings->timeZoneOffsetHalfHours % 2) * 30
    );
    xPrev = x;
    x = Text_draw7Seg(s, 2, x, false);
    SSD1306_fillAreaPattern(xPrev, 5, x - xPrev, 1, LinePattern);
}

bool SettingsScreen_TimeZone_handleKeyPress(const uint8_t keyCode, const bool hold)
{
    switch (keyCode) {
        // Exit
        case Keypad_Key1: {
            if (hold) {
                break;
            }

            return false;
        }

        // Change selection
        case Keypad_Key2: {
            break;
        }

        // Minutes
        case Keypad_Key3: {
            // -12:00 .. +14:00 in 30 minute steps
            if (++context.settings->timeZoneOffsetHalfHours > 28) {
                context.settings->timeZoneOffsetHalfHours = -24;
            }

            SettingsScreen_TimeZone_update(false);
            break;
        }
    }

    return true;
}
