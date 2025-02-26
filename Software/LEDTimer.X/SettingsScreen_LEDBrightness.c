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
#include "OutputController.h"
#include "SettingsScreen_LEDBrightness.h"

#include "mcc_generated_files/pwm5.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static struct SettingScreen_LEDBrightness_Context {
    struct Output* settings;
} context;

void SettingsScreen_LEDBrightness_init(struct Output* settings)
{
    context.settings = settings;
}

void SettingsScreen_LEDBrightness_update(const bool redraw)
{
    if (redraw) {
        Graphics_DrawScreenTitle("LED BRIGHTNESS");
        Graphics_DrawKeypadHelpBar(Graphics_ExitIcon, Graphics_SetIcon, Graphics_ClearIcon);
    }

    char s[6];
//    sprintf(s, "%3lu", context.settings->brightness);
    uint16ToString(context.settings->brightness, s, ' ');
    Text_draw7Seg(s + 2, 2, 64 - Text_calculateWidth7Seg(s) / 2, false);

    OutputController_deactivate();
    PWM5_LoadDutyValue(context.settings->brightness);
}

bool SettingsScreen_LEDBrightness_handleKeyPress(const uint8_t keyCode, const bool hold)
{
    switch (keyCode) {
        // Exit
        case Keypad_Key1: {
            if (hold) {
                break;
            }

            // Restore the original state
            OutputController_activate();
            OutputController_updateState();

            return false;
        }

        // Set
        case Keypad_Key2: {
            if (++context.settings->brightness >= 512) {
                context.settings->brightness = 0;
            }
            SettingsScreen_LEDBrightness_update(false);
            break;
        }

        // Adjust
        case Keypad_Key3: {
            if (--context.settings->brightness >= 512) {
                context.settings->brightness = 511;
            }
            SettingsScreen_LEDBrightness_update(false);
            break;
        }
    }

    return true;
}
