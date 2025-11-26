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
#include "SettingsScreen_LEDBrightness.h"
#include "OutputController.h"

#include "mcc_generated_files/pwm5.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static struct SettingScreen_LEDBrightness_Context {
    struct Output* settings;
} context;

void SettingsScreen_LEDBrightness_init(struct Output* settings)
{
    context.settings = settings;
    OutputController_suspend(true);
    PWM5_LoadDutyValue(context.settings->brightness);
}

void SettingsScreen_LEDBrightness_close()
{
    OutputController_suspend(false);
}

void SettingsScreen_LEDBrightness_update(const bool redraw)
{
    if (redraw) {
        Graphics_DrawScreenTitle("LED BRIGHTNESS");
        Graphics_DrawKeypadHelpBar(Graphics_ExitIcon, Graphics_SetIcon, Graphics_ClearIcon);
    }

    char s[4];
    sprintf(s, "%3u", context.settings->brightness);
    Text_draw7Seg(s, 2, 64 - Text_calculateWidth7Seg(s) / 2, false);
}

bool SettingsScreen_LEDBrightness_handleKeyPress(const uint8_t keyCode, const bool hold)
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
            ++context.settings->brightness;
            SettingsScreen_LEDBrightness_update(false);
            PWM5_LoadDutyValue(context.settings->brightness);
            break;
        }

        // Adjust
        case Keypad_Key3: {
            --context.settings->brightness;
            SettingsScreen_LEDBrightness_update(false);
            PWM5_LoadDutyValue(context.settings->brightness);
            break;
        }
    }

    return true;
}
