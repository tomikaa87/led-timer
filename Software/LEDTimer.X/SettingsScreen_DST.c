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
    Created on 2023-04-06
*/

#include "Graphics.h"
#include "Keypad.h"
#include "SettingsScreen_DST.h"
#include "SSD1306.h"
#include "Text.h"

#include <stdio.h>

static struct SettingsScreen_DST_Context {
    Date_DstData* settings;
    uint8_t selectionIndex;
} context;

void SettingsScreen_DST_init(Date_DstData* settings)
{
    context.settings = settings;
    context.selectionIndex = 0;
}

static void drawDstSettingsLine(const bool start)
{
    static const char* OrdinalNames[4] = {
        "1ST",
        "2ND",
        "LAST"
    };

    char buf[5];
    uint8_t x = 5;
    uint8_t line = start ? 3 : 5;
    uint8_t itemIndex = start ? 0 : 4;

    sprintf(
        buf,
        "%4s",
        OrdinalNames[
            start
                ? context.settings->startOrdinal
                : context.settings->endOrdinal
        ]
    );
    x = Text_draw(buf, line, x, 0, context.selectionIndex == itemIndex);
    x += 5;
    ++itemIndex;

    sprintf(
        buf,
        "%3s",
        Date_DayShortNames[
            start
                ? context.settings->startDayOfWeek
                : context.settings->endDayOfWeek
        ]
    );
    x = Text_draw(buf, line, x, 0, context.selectionIndex == itemIndex);
    x += 5;
    ++itemIndex;

    x = Text_draw("OF", line, x, 0, false);
    x += 5;

    sprintf(
        buf,
        "%3s",
        Date_MonthShortNames[
            start
                ? context.settings->startMonth
                : context.settings->endMonth
        ]
    );
    x = Text_draw(buf, line, x, 0, context.selectionIndex == itemIndex);
    x += 5;
    ++itemIndex;

    sprintf(
        buf,
        "%d:00",
        start
            ? context.settings->startShiftHours
            : context.settings->endShiftHours
    );
    Text_draw(buf, line, x, 0, context.selectionIndex == itemIndex);
}

void SettingsScreen_DST_update(const bool redraw)
{
    if (redraw) {
        Graphics_DrawScreenTitle("DST");
        Graphics_DrawKeypadHelpBar(
            Graphics_ExitIcon,
            Graphics_ArrowDownIcon,
            Graphics_AdjustIcon
        );
    }

    /*
START                           @2
     1ST SUN OF MAR AT 2:00     @3  selection: 0..3
     2ND SUN OF MAR AT 2:00
    LAST SUN OF MAR AT 2:00
END                             @4
     1ST SUN OF NOV AT 2:00     @5  selection: 4..7
     2ND SUN OF NOV AT 2:00
    LAST SUN OF NOV AT 2:00
    */

    if (redraw) {
        SSD1306_fillArea(0, 2, 128, 4, SSD1306_COLOR_BLACK);
        Text_draw("START", 2, 0, 0, false);
        Text_draw("END", 4, 0, 0, false);
    }

    drawDstSettingsLine(true);
    drawDstSettingsLine(false);
}

static void adjustSelectedItem()
{
    switch (context.selectionIndex) {
        // Start ordinal
        case 0:
            if (++context.settings->startOrdinal > 2) {
                context.settings->startOrdinal = 0;
            }
            break;

        // Start day-of-week
        case 1:
            if (++context.settings->startDayOfWeek > 6) {
                context.settings->startDayOfWeek = 0;
            }
            break;

        // Start month
        case 2:
            if (++context.settings->startMonth > 11) {
                context.settings->startMonth = 0;
            }
            break;

        // Start shift hours
        case 3:
            ++context.settings->startShiftHours;
            break;

        // End ordinal
        case 4:
            if (++context.settings->endOrdinal > 2) {
                context.settings->endOrdinal = 0;
            }
            break;

        // End day-of-week
        case 5:
            if (++context.settings->endDayOfWeek > 6) {
                context.settings->endDayOfWeek = 0;
            }
            break;

        // End month
        case 6:
            if (++context.settings->endMonth > 11) {
                context.settings->endMonth = 0;
            }
            break;

        // End shift hours
        case 7:
            ++context.settings->endShiftHours;
            break;

        default:
            break;
    }
}

bool SettingsScreen_DST_handleKeyPress(const uint8_t keyCode, const bool hold)
{
    switch (keyCode) {
        // Exit
        case Keypad_Key1: {
            if (hold) {
                break;
            }

            return false;
        }

        // Select
        case Keypad_Key2: {
            if (++context.selectionIndex == 8) {
                context.selectionIndex = 0;
            }
            SettingsScreen_DST_update(false);
            break;
        }

        // Adjust
        case Keypad_Key3: {
            adjustSelectedItem();
            SettingsScreen_DST_update(false);
            break;
        }
    }

    return true;
}
