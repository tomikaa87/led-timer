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
#include "SettingsScreen_DisplayBrightness.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static struct SettingScreen_DisplayBrightness_Context {
    struct Display* settings;
} context;

void SettingsScreen_DisplayBrightness_init(struct Display* settings)
{
    context.settings = settings;
}

void SettingsScreen_DisplayBrightness_update(const bool redraw)
{
    if (redraw) {
        Graphics_DrawScreenTitle("DISP. BRIGHTNESS");
        Graphics_DrawKeypadHelpBar(Graphics_ExitIcon, Graphics_SetIcon, Graphics_ClearIcon);
    }

    char s[6];
//    sprintf(s, "%u", context.settings->brightness);
    uint16ToString(context.settings->brightness, s, '0');
    Text_draw7Seg(s + 4, 2, 64 - Text_calculateWidth7Seg(s) / 2, false);
}

bool SettingsScreen_DisplayBrightness_handleKeyPress(const uint8_t keyCode, const bool hold)
{
    switch (keyCode) {
        // Exit
        case Keypad_Key1: {
            if (hold) {
                break;
            }

            return false;
        }

        // Set
        case Keypad_Key2: {
            if (++context.settings->brightness > SSD1306_CONTRAST_HIGH) {
                context.settings->brightness = SSD1306_CONTRAST_LOWEST;
            }
            SettingsScreen_DisplayBrightness_update(false);
            break;
        }

        // Adjust
        case Keypad_Key3: {
            // Check if the value overflown
            if (--context.settings->brightness > SSD1306_CONTRAST_HIGH) {
                context.settings->brightness = SSD1306_CONTRAST_HIGH;
            };
            SettingsScreen_DisplayBrightness_update(false);
            break;
        }
    }

    return true;
}
