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
    Created on 2022-12-05
*/

#include "Graphics.h"
#include "Keypad.h"
#include "Settings.h"
#include "SettingsScreen.h"
#include "Text.h"
#include "SSD1306.h"

#include <stdio.h>
#include <string.h>

static struct SettingsScreenContext {
    enum State {
        State_Navigation,
        State_Configuration
    } state;

    uint8_t pageIndex;
    const uint8_t pageCount;
    SettingsData modifiedSettings;

    struct SchedulerPageContext {
        uint8_t segmentIndex;
    } schedulerPage;
} context = {
    .state = State_Navigation,
    .pageIndex = 0,
    .pageCount = 2,
    .schedulerPage = {
        .segmentIndex = 0
    }
};

static void drawKeypadHelpBar()
{
    SSD1306_fillArea(0, 1, 128, 1, SSD1306_COLOR_BLACK);

    switch (context.state) {
        case State_Navigation:
            Text_draw("EXIT", 0, 0, 0, false);
            Text_draw("SELECT", 0, 64 - (6 * 5 + 5 * 1) / 2, 0, false);
            Text_draw("NEXT", 0, 127 - (4 * 5 + 3 * 1) / 2, 0, false);
            break;

        case State_Configuration:
            Text_draw("BACK", 0, 0, 0, false);

            // Scheduler
            if (context.pageIndex == 0) {
                Text_draw("SET", 0, 64 - (3 * 5 + 2 * 1) / 2, 0, false);
                Text_draw("CLEAR", 0, 127 - (5 * 5 + 4 * 1) / 2, 0, false);
            }
            // Regular number setting
            else {
                Text_draw("+", 0, 64 - (1 * 5) / 2, 0, false);
                Text_draw("-", 0, 127 - (1 * 5) / 2, 0, false);
            }
            break;
    }
}

static void drawPageTitle()
{
    char s[22];

    static const char* Titles[2] = {
        "Schedule",
        "LED Brightness"
    };

    // If this needs to be optimized to spare program space,
    // replace it with multiple Text_draw calls
    snprintf(
        s, sizeof(s), "%u/%u: %s",
        context.pageIndex + 1,
        context.pageCount,
        Titles[context.pageIndex]
    );

    uint8_t x = Text_draw(s, 1, 0, 0, false);

    // Fill the rest of the line in case the previous text was longer
    if (x < 127) {
        SSD1306_fillArea(x, 1, 128 - x, 1, SSD1306_COLOR_BLACK);
    }
}

static void drawSchedulerPage()
{
    uint16_t minutesSinceMidnightForSegment =
        (uint16_t)context.schedulerPage.segmentIndex * 30;
    uint8_t hours = (uint8_t)(minutesSinceMidnightForSegment / 60);
    uint8_t minutes = (uint8_t)(minutesSinceMidnightForSegment - hours * 60);

    char s[6];
    snprintf(s, sizeof(s), "%02u:%02u", hours, minutes);

    Text_draw7Seg(
        s,
        2,
        64 - (
            4 * 12      /* number width */
            + 4         /* colon width */
            + 4 * 2     /* spacing */
        ) / 2,
        false
    );

    Graphics_drawScheduleSegmentIndicator(context.schedulerPage.segmentIndex, false);
    Graphics_drawScheduleBar(context.modifiedSettings.scheduler.data, false);
}

static void drawBrightnessPage()
{

}

static void drawCurrentPage(const bool redraw)
{
    if (redraw) {
        drawKeypadHelpBar();
        drawPageTitle();
    }

    switch (context.pageIndex) {
        case 0:
            drawSchedulerPage();
            break;

        case 1:
            drawBrightnessPage();
            break;

        default:
            break;
    }
}

static void navigateToNextPage()
{
    if (++context.pageIndex >= context.pageCount) {
        context.pageIndex = 0;
    }

    SSD1306_clear();
    drawCurrentPage(true);
}

void SettingsScreen_init()
{
    context.state = State_Navigation;
    context.pageIndex = 0;

    context.schedulerPage.segmentIndex = 0;

    memcpy(&context.modifiedSettings, &Settings_data, sizeof(SettingsData));

    drawCurrentPage(true);
}

void SettingsScreen_update(const bool redraw)
{
}

bool SettingsScreen_handleKeyPress(const uint8_t keyCode, const bool hold)
{
    switch (keyCode) {
        case Keypad_Key1:
            if (context.state == State_Configuration) {
                context.state = State_Navigation;
                return true;
            }
            break;

        case Keypad_Key2:
        case Keypad_Key3:
            if (context.state == State_Navigation) {
                navigateToNextPage();
            }
            return true;
    }

    return false;
}
