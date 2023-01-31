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
    uint8_t _reserved : 2;
} context;

void SettingsScreen_Time_init()
{
    memset(&context, 0, sizeof(struct SettingScreen_Time_Context));
    context.hours = (uint8_t)(Clock_getMinutesSinceMidnight() / 60);
    context.minutes = (uint8_t)(Clock_getMinutesSinceMidnight() - context.hours * 60);
}

void SettingsScreen_Time_update(const bool redraw)
{
    if (redraw) {
        Graphics_DrawScreenTitle("Time");
        Graphics_DrawKeypadHelpBar(Graphics_ExitIcon, Graphics_SetIcon, Graphics_ClearIcon);
    }

    char s[6];

    sprintf(
        s,
        "%2u:%02u",
        context.hours,
        context.minutes
    );

    Text_draw7Seg(s, 2, 64 - Text_calculateWidth7Seg(s) / 2, false);
}

bool SettingsScreen_Time_handleKeyPress(const uint8_t keyCode, const bool hold)
{
    switch (keyCode) {
        // Exit
        case Keypad_Key1: {
            if (hold) {
                break;
            }

            Clock_setMinutesSinceMidnight(context.hours * 60 + context.minutes);

            return false;
        }

        // Hours
        case Keypad_Key2: {
            if (++context.hours >= 24) {
                context.hours = 0;
            }
            context.clockAdjusted = true;
            SettingsScreen_Time_update(false);
            break;
        }

        // Minutes
        case Keypad_Key3: {
            if (++context.minutes >= 60) {
                context.minutes = 0;
            }
            context.clockAdjusted = true;
            SettingsScreen_Time_update(false);
            break;
        }
    }

    return true;
}
