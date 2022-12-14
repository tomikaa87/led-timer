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

#include "Clock.h"
#include "Graphics.h"
#include "Keypad.h"
#include "Settings.h"
#include "SettingsScreen.h"
#include "Text.h"
#include "SSD1306.h"
#include "OutputController.h"

#include <stdio.h>
#include <string.h>

static struct SettingsScreenContext {
    enum State {
        State_Navigation,
        State_Configuration
    } state;

    enum Page {
        Page_Schedule,
        Page_LEDBrightness,
        Page_Clock,
        Page_DisplayBrightness,

        Page_Last = Page_DisplayBrightness
    } page;

    uint8_t pageIndex;
    SettingsData modifiedSettings;

    struct SchedulerPageContext {
        uint8_t segmentIndex;
    } schedulerPage;

    struct ClockPageContext {
        uint8_t minutes;
        uint8_t hours;
        bool clockAdjusted;
    } clockPage;
} context = {
    .state = State_Navigation,
    .pageIndex = 0,
    .schedulerPage = {
        .segmentIndex = 0
    },
    .clockPage = {
        .minutes = 0,
        .hours = 0,
        .clockAdjusted = false
    }
};

inline static void drawKeypadHelpBar()
{
    SSD1306_fillArea(0, 0, 128, 1, SSD1306_COLOR_BLACK);

    switch (context.state) {
        case State_Navigation:
            Text_draw("EXIT", 0, 0, 0, false);
            Text_draw("SELECT", 0, 64 - (6 * 5 + 5 * 1) / 2, 0, false);
            Text_draw("NEXT", 0, 127 - (4 * 5 + 3 * 1), 0, false);
            break;

        case State_Configuration:
            Text_draw("BACK", 0, 0, 0, false);

            if (context.pageIndex == Page_Schedule) {
                Text_draw("SET", 0, 64 - (3 * 5 + 2 * 1) / 2, 0, false);
                Text_draw("CLEAR", 0, 127 - (5 * 5 + 4 * 1), 0, false);
            } else if (context.pageIndex == Page_Clock) {
                Text_draw("HOUR", 0, 64 - (4 * 5 + 3 * 1) / 2, 0, false);
                Text_draw("MINUTE", 0, 127 - (6 * 5 + 5 * 1), 0, false);
            }
            // Regular number setting
            else {
                Text_draw("+", 0, 64 - (1 * 5) / 2, 0, false);
                Text_draw("-", 0, 127 - (1 * 5), 0, false);
            }
            break;
    }
}

inline static void drawPageTitle()
{
    char s[22];

    static const char* Titles[Page_Last + 1] = {
        "Schedule",
        "LED Brightness",
        "Clock",
        "Disp. Brightness"
    };

    // If this needs to be optimized to spare program space,
    // replace it with multiple Text_draw calls
    snprintf(
        s, sizeof(s), "%u/%u: %s",
        context.pageIndex + 1,
        Page_Last + 1,
        Titles[context.pageIndex]
    );

    uint8_t x = Text_draw(s, 1, 0, 0, false);

    // Fill the rest of the line in case the previous text was longer
    if (x < 127) {
        SSD1306_fillArea(x, 1, 128 - x, 1, SSD1306_COLOR_BLACK);
    }
}

inline static void drawSchedulerPage()
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
        64 - Text_calculateWidth7Seg(s) / 2,
        false
    );

    Graphics_drawScheduleSegmentIndicator(context.schedulerPage.segmentIndex, false);
    Graphics_drawScheduleBar(context.modifiedSettings.scheduler.data, false);
}

inline static void drawBrightnessPage()
{
    char s[4];
    snprintf(s, sizeof(s), "%3u", context.modifiedSettings.output.brightness);
    Text_draw7Seg(s, 3, 64 - Text_calculateWidth7Seg(s) / 2, false);
}

inline static void drawClockPage()
{
    char s[6];
    snprintf(
        s,
        sizeof(s),
        "%2u:%02u",
        context.clockPage.hours,
        context.clockPage.minutes
    );
    Text_draw7Seg(s, 3, 64 - Text_calculateWidth7Seg(s) / 2, false);
}

inline static void drawDisplayBrightnessPage()
{
    char s[4];
    snprintf(s, sizeof(s), "%u", context.modifiedSettings.display.brightness);
    Text_draw7Seg(s, 3, 64 - Text_calculateWidth7Seg(s) / 2, false);
}

static void drawCurrentPage(const bool redraw)
{
    if (redraw) {
        drawKeypadHelpBar();
        drawPageTitle();
    }

    switch (context.pageIndex) {
        case Page_Schedule:
            drawSchedulerPage();
            break;

        case Page_LEDBrightness:
            drawBrightnessPage();
            break;

        case Page_Clock:
            drawClockPage();
            break;

        case Page_DisplayBrightness:
            drawDisplayBrightnessPage();
            break;

        default:
            break;
    }
}

void updateClockPageTime()
{
    // Update the clock
    if (context.pageIndex == Page_Clock) {
        context.clockPage.hours =
            (uint8_t)(Clock_getMinutesSinceMidnight() / 60);

        context.clockPage.minutes = (uint8_t)(
            Clock_getMinutesSinceMidnight()
            - context.clockPage.hours * 60
        );
    }
}

static void navigateToNextPage()
{
    if (++context.pageIndex > Page_Last) {
        context.pageIndex = 0;
    }

    updateClockPageTime();

    SSD1306_clear();
    drawCurrentPage(true);
}

static void adjustScheduleSegmentAndStepForward(const bool set)
{
    Types_setScheduleSegmentBit(
        context.modifiedSettings.scheduler.data,
        context.schedulerPage.segmentIndex,
        set
    );

    if (++context.schedulerPage.segmentIndex >= 48) {
        context.schedulerPage.segmentIndex = 0;
    }

    drawCurrentPage(false);
}

static void adjustOutputBrightness(const bool increase)
{
    if (increase) {
        ++context.modifiedSettings.output.brightness;
    } else {
        --context.modifiedSettings.output.brightness;
    }

    drawCurrentPage(false);
}

static void adjustClock(const bool hours)
{
    if (hours) {
        if (++context.clockPage.hours >= 24) {
            context.clockPage.hours = 0;
        }
    } else {
        if (++context.clockPage.minutes >= 60) {
            context.clockPage.minutes = 0;
        }
    }
}

static void adjustDisplayBrightness(const bool increase)
{
    if (increase) {
        if (++context.modifiedSettings.display.brightness > SSD1306_CONTRAST_HIGH) {
            context.modifiedSettings.display.brightness = SSD1306_CONTRAST_LOWEST;
        }
    } else {
        // Check if the value overflown
        if (--context.modifiedSettings.display.brightness > SSD1306_CONTRAST_HIGH) {
            context.modifiedSettings.display.brightness = SSD1306_CONTRAST_HIGH;
        };
    }

    drawCurrentPage(false);
}

void SettingsScreen_init()
{
    context.state = State_Navigation;
    context.pageIndex = Page_Schedule;

    context.schedulerPage.segmentIndex = 0;

    memcpy(&context.modifiedSettings, &Settings_data, sizeof(SettingsData));
}

void SettingsScreen_update(const bool redraw)
{
    drawCurrentPage(redraw);
}

inline bool SettingsScreen_handleKeyPress(const uint8_t keyCode, const bool hold)
{
    // Avoid jumping between states
    if (hold && context.state == State_Navigation) {
        return true;
    }

    switch (keyCode) {
        case Keypad_Key1:
            switch (context.state) {
                case State_Navigation:
                    // Save settings on exit
                    Settings_data = context.modifiedSettings;
                    Settings_save();
                    OutputController_updateState();
                    SSD1306_setContrastLevel(Settings_data.display.brightness);

                    // UI will handle this by switching to the main screen
                    return false;

                case State_Configuration:
                    context.state = State_Navigation;

                    // For convenience, reset the scheduler segment index
                    context.schedulerPage.segmentIndex = 0;

                    // Update the system clock
                    if (context.pageIndex == Page_Clock) {
                        Clock_setMinutesSinceMidnight(
                            context.clockPage.hours * 60
                            + context.clockPage.minutes
                        );
                    }

                    drawCurrentPage(true);
                    break;

                default:
                    break;
            }
            break;

        case Keypad_Key2:
            switch (context.state) {
                case State_Navigation:
                    context.state = State_Configuration;

                    updateClockPageTime();

                    drawCurrentPage(true);
                    break;

                case State_Configuration:
                    // Scheduler config: SET
                    if (context.pageIndex == Page_Schedule) {
                        adjustScheduleSegmentAndStepForward(true);
                    }
                    // Brightness config: +
                    else if (context.pageIndex == Page_LEDBrightness) {
                        adjustOutputBrightness(true);
                    }
                    // Clock: HOURS+
                    else if (context.pageIndex == Page_Clock) {
                        adjustClock(true);
                    }
                    // Display brightness: +
                    else if (context.pageIndex == Page_DisplayBrightness) {
                        adjustDisplayBrightness(true);
                    }
                    break;

                default:
                    break;
            }
            break;

        case Keypad_Key3:
            switch (context.state) {
                case State_Navigation:
                    navigateToNextPage();
                    break;

                case State_Configuration:
                    // Scheduler config: CLEAR
                    if (context.pageIndex == Page_Schedule) {
                        adjustScheduleSegmentAndStepForward(false);
                    }
                    // Brightness config: -
                    else if (context.pageIndex == Page_LEDBrightness) {
                        adjustOutputBrightness(false);
                    }
                    // Clock: MINUTES+
                    else if (context.pageIndex == Page_Clock) {
                        adjustClock(false);
                    }
                    // Display brightness: -
                    else if (context.pageIndex == Page_DisplayBrightness) {
                        adjustDisplayBrightness(false);
                    }
                    break;

                default:
                    break;
            }
            break;
    }

    return true;
}
