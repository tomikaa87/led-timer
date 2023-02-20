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
    Created on 2023-02-16
*/

#include "Clock.h"
#include "Graphics.h"
#include "Keypad.h"
#include "Settings.h"
#include "SettingsScreen_Location.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static struct SettingScreen_Location_Context {
    struct Location* settings;
    uint32_t latitudeBcd;
    uint32_t longitudeBcd;
    uint8_t latitudeSign : 1;
    uint8_t longitudeSign : 1;
    uint8_t selectionIndex : 6;
} context;

void SettingsScreen_Location_init(struct Location* const settings)
{
    context.latitudeBcd = settings->latitudeBcd;
    context.longitudeBcd = settings->longitudeBcd;
    context.latitudeSign = 0;
    context.longitudeSign = 0;
    context.settings = settings;
}

static void drawBcdNumber(
    uint8_t x,
    const uint8_t line,
    const uint8_t startIndex,
    uint32_t bcdNumber,
    const uint8_t decimalPointPos
) {
    for (uint8_t i = 0; i < 8; ++i) {
        if (i == decimalPointPos) {
            x = Text_draw(".", line, x, 0, false);
        }

        char s[2] = { 0, };
        s[0] = ((bcdNumber & 0xF0000000) >> 28) + '0';

        x = Text_draw(s, line, x, 0, context.selectionIndex == i + startIndex);

        bcdNumber <<= 4;
    }
}

void SettingsScreen_Location_update(const bool redraw)
{
    if (redraw) {
        Graphics_DrawScreenTitle("Location");
        Graphics_DrawKeypadHelpBar(Graphics_ExitIcon, Graphics_ArrowRightIcon, Graphics_AdjustIcon);

        LeftText("Lat.:", 3);
        LeftText("Lon.:", 4);
    }

    uint8_t x = CalculateTextWidth("Lat.: ");

    // Latitude: -90..90, longitude: -180..180

    Text_draw(context.latitudeSign ? "-" : "+", 3, x, 0, context.selectionIndex == 0);
    x = Text_draw(context.longitudeSign ? "-" : "+", 4, x, 0, context.selectionIndex == 9);

    drawBcdNumber(x, 3, 1, context.latitudeBcd, 3);
    drawBcdNumber(x, 4, 10, context.longitudeBcd, 3);
}

static void adjustBcdNumber(uint32_t* const bcd, const uint8_t digit)
{
    uint8_t offset = (7 - digit) * 4;
    uint32_t mask = (uint32_t)0b1111u << offset;
    uint8_t value = (uint8_t)((*bcd & mask) >> offset);

    if (++value >= 10) {
        value = 0;
    };

    *bcd &= ~mask;
    *bcd |= (uint32_t)value << offset;

#if 0
    char s[30];
    sprintf(s, "o=%02d,m=%08lx", offset, mask);
    Text_draw(s, 5, 0, 0, false);
    sprintf(s, "v=%02x,b=%08lx", value, *bcd);
    Text_draw(s, 6, 0, 0, false);
#endif
}

bool SettingsScreen_Location_handleKeyPress(const uint8_t keyCode, const bool hold)
{
    switch (keyCode) {
        // Exit
        case Keypad_Key1: {
            if (hold) {
                break;
            }

            context.settings->latitudeBcd = context.latitudeBcd;
            context.settings->longitudeBcd = context.longitudeBcd;
            context.settings->latitudeSign = context.latitudeSign;
            context.settings->longitudeSign = context.longitudeSign;

            return false;
        }

        // Change selection
        case Keypad_Key2: {
            if (++context.selectionIndex > 17) {
                context.selectionIndex = 0;
            }
            if (context.selectionIndex == 10 && context.latitudeBcd > 0x9000000u) {
                // Clamp latitude to 90.00000
                context.latitudeBcd = 0x9000000u;
            } else if (context.selectionIndex == 0 && context.longitudeBcd > 0x18000000u) {
                // Clamp longitude to 180.00000
                context.longitudeBcd = 0x18000000u;
            }
            SettingsScreen_Location_update(false);
            break;
        }

        // Adjust the selected value
        case Keypad_Key3: {
            if (context.selectionIndex == 0) {
                ++context.latitudeSign;
            } else if (context.selectionIndex == 9) {
                ++context.longitudeSign;
            } else if (context.selectionIndex >= 1 && context.selectionIndex <= 8) {
                adjustBcdNumber(&context.latitudeBcd, context.selectionIndex - 1);
            } else if (context.selectionIndex >= 10 && context.selectionIndex <= 17) {
                adjustBcdNumber(&context.longitudeBcd, context.selectionIndex - 10);
            }
            SettingsScreen_Location_update(false);
            break;
        }
    }

    return true;
}
