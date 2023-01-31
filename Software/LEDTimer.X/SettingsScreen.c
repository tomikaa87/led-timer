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

#pragma warning push
#pragma warning disable 763

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
        Page_NewScheduler,

        Page_Last = Page_NewScheduler
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

    struct NewSchedulerPageContext {
        uint8_t selection : 4;
        uint8_t schedulerTypeChanged : 1;
        uint8_t onScheduleTypeChanged : 1;
        uint8_t offScheduleTypeChanged : 1;
        uint8_t selectionChanged : 1;
    } newSchedulerPage;
} context;

inline static void drawKeypadHelpBar()
{
    SSD1306_fillArea(0, 0, 128, 1, SSD1306_COLOR_BLACK);

    switch (context.state) {
        case State_Navigation:
            LeftHelpText("EXIT");
            CenterHelpText("SELECT");
            RightHelpText("NEXT");
            break;

        case State_Configuration:
            LeftHelpText("BACK");

            if (context.pageIndex == Page_Schedule) {
                CenterHelpText("SET");
                RightHelpText("CLEAR");
            } else if (context.pageIndex == Page_Clock) {
                CenterHelpText("HOUR");
                RightHelpText("MINUTE");
            } else if (context.pageIndex == Page_NewScheduler) {
                CenterHelpText("SELECT");
                RightHelpText("ADJUST");
            }
            // Regular number setting
            else {
                CenterHelpText("+");
                RightHelpText("-");
            }
            break;
    }
}

inline static void drawPageTitle()
{
    char s[22];

    static const char* Titles[Page_Last + 1] = {
        "Sgmt. Schdeduler",
        "LED Brightness",
        "Clock",
        "Disp. Brightness",
        "Scheduler"
    };

    // If this needs to be optimized to spare program space,
    // replace it with multiple Text_draw calls
//    snprintf(
//        s, sizeof(s), "%u/%u: %s",
//        context.pageIndex + 1,
//        Page_Last + 1,
//        Titles[context.pageIndex]
//    );
    sprintf(
        s,
        "%u/%u: %s",
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
//    snprintf(s, sizeof(s), "%02u:%02u", hours, minutes);
    sprintf(s, "%02u:%02u", hours, minutes);

    Text_draw7Seg(
        s,
        2,
        64 - Text_calculateWidth7Seg(s) / 2,
        false
    );

    Graphics_drawScheduleSegmentIndicator(context.schedulerPage.segmentIndex, false);
    Graphics_drawScheduleBar(context.modifiedSettings.scheduler.segmentData, false);
}

inline static void drawBrightnessPage()
{
    char s[4];
//    snprintf(s, sizeof(s), "%3u", context.modifiedSettings.output.brightness);
    sprintf(s, "%3u", context.modifiedSettings.output.brightness);
    Text_draw7Seg(s, 3, 64 - Text_calculateWidth7Seg(s) / 2, false);
}

inline static void drawClockPage()
{
    char s[6];
//    snprintf(
//        s,
//        sizeof(s),
//        "%2u:%02u",
//        context.clockPage.hours,
//        context.clockPage.minutes
//    );
    sprintf(
        s,
        "%2u:%02u",
        context.clockPage.hours,
        context.clockPage.minutes
    );
    Text_draw7Seg(s, 3, 64 - Text_calculateWidth7Seg(s) / 2, false);
}

inline static void drawDisplayBrightnessPage()
{
    char s[4];
//    snprintf(s, sizeof(s), "%u", context.modifiedSettings.display.brightness);
    sprintf(s, "%u", context.modifiedSettings.display.brightness);
    Text_draw7Seg(s, 3, 64 - Text_calculateWidth7Seg(s) / 2, false);
}

inline static void drawNewSchedulerPage(const bool redraw)
{
    static const char* SchedulerTypes[] = { "SEGMENT", "SIMPLE", "OFF" };
    static const char* ScheduleTypes[] = { "TIME", "SUNRISE", "SUNSET" };

    #define InvertForSelectionIndex(_Index) (\
        context.state == State_Configuration \
        && context.newSchedulerPage.selection == _Index \
    )

    #define PositionAfter(_Label) (\
        CalculateTextWidth(_Label) + 5 /* space */ \
    )

    // Debug: selection index
    {
        char s[3];
        sprintf(s, "%02d", context.newSchedulerPage.selection);
        Text_draw(s, 2, 128 - CalculateTextWidth("00"), 0, false);
    }

    // Draw the fixed labels
    if (redraw) {
        context.newSchedulerPage.schedulerTypeChanged = true;
        context.newSchedulerPage.onScheduleTypeChanged = true;
        context.newSchedulerPage.offScheduleTypeChanged = true;

        SSD1306_fillArea(0, 2, 128, 6, SSD1306_COLOR_BLACK);

        Text_draw("TYPE:", 2, 0, 0, false);
    }

    if (!redraw && context.newSchedulerPage.schedulerTypeChanged) {
        // Size the background fill for the longest text (+1 because of the inverted text)
        SSD1306_fillArea(PositionAfter("TYPE:"), 2, CalculateTextWidth("SEGMENT") + 1, 1, SSD1306_COLOR_BLACK);
        // Clear the background of the contents
        SSD1306_fillArea(0, 3, 128, 6, SSD1306_COLOR_BLACK);
    }

    if (
        context.newSchedulerPage.schedulerTypeChanged
        || context.newSchedulerPage.selectionChanged
    ) {
        // Scheduler type
        Text_draw(
            SchedulerTypes[context.modifiedSettings.scheduler.type],
            2,
            PositionAfter("TYPE:"),
            0,
            InvertForSelectionIndex(0)
        );
    }

    if (context.newSchedulerPage.schedulerTypeChanged) {
        // Draw the fixed labels for the current scheduler type
        switch (context.modifiedSettings.scheduler.type) {
            case Settings_SchedulerType_Segment:
                CenterText("Change settings", 4);
                CenterText("on Sgmt. Schd. screen", 5);
                break;

            case Settings_SchedulerType_Simple:
                // Switch on schedule label
                LeftText("ON:", 4);
                // Switch off schedule label
                LeftText("OFF:", 6);

                switch (context.modifiedSettings.scheduler.simpleOnSchedule.type) {
                    case Settings_ScheduleType_Sunrise:
                    case Settings_ScheduleType_Sunset:
                        // Switch on schedule setting label
                        LeftText("OFFSET:", 5);
                        break;

                    case Settings_ScheduleType_Time:
                        // Switch on time setting label
                        LeftText("TIME:", 5);
                        // Time hour-minute separator
                        Text_draw(":", 5, PositionAfter("TIME: xx"), 0, false);
                        break;
                }

                switch (context.modifiedSettings.scheduler.simpleOffSchedule.type) {
                    case Settings_ScheduleType_Sunrise:
                    case Settings_ScheduleType_Sunset:
                        // Switch off schedule setting label
                        LeftText("OFFSET:", 7);
                        break;

                    case Settings_ScheduleType_Time:
                        // Switch off time setting label
                        LeftText("TIME:", 7);
                        // Time hour-minute separator
                        Text_draw(":", 7, PositionAfter("TIME: xx"), 0, false);
                        break;
                }
                break;

            case Settings_SchedulerType_Off:
                CenterText("The output will only", 4);
                CenterText("be switched manually", 5);
                break;
        }
    }

    switch (context.modifiedSettings.scheduler.type) {
        case Settings_SchedulerType_Segment: {
            break;
        }

        case Settings_SchedulerType_Simple: {
            // Switch on schedule type
            if (
                context.newSchedulerPage.onScheduleTypeChanged
                || context.newSchedulerPage.schedulerTypeChanged
                || context.newSchedulerPage.selectionChanged
            ) {
                uint8_t x = Text_draw(
                    ScheduleTypes[
                        context.modifiedSettings.scheduler.simpleOnSchedule.type
                    ],
                    4, PositionAfter("ON:"), 0, InvertForSelectionIndex(1)
                );
                // Clean the background after the text
                SSD1306_fillArea(x, 4, 20, 1, SSD1306_COLOR_BLACK);
            }

            // Switch off schedule type
            if (
                context.newSchedulerPage.offScheduleTypeChanged
                || context.newSchedulerPage.schedulerTypeChanged
                || context.newSchedulerPage.selectionChanged
            ) {
                uint8_t x = Text_draw(
                    ScheduleTypes[
                        context.modifiedSettings.scheduler.simpleOffSchedule.type
                    ],
                    6, PositionAfter("OFF:"), 0, InvertForSelectionIndex(4)
                );
                // Clean the background after the text
                SSD1306_fillArea(x, 6, 20, 1, SSD1306_COLOR_BLACK);
            }

#if 0
            switch (context.modifiedSettings.scheduler.simpleOnSchedule.type) {
                case Settings_ScheduleType_Sunrise:
                case Settings_ScheduleType_Sunset: {
                    // Offset value label
                    char s[10];
                    sprintf(s, "%+3.2d MIN", context.modifiedSettings.scheduler.simpleOnSchedule.sunOffset);
                    Text_draw(s, 5, PositionAfter("OFFSET:"), 0, InvertForSelectionIndex(2));
                    break;
                }

                case Settings_ScheduleType_Time: {
                    char s[3];

                    // Time hour value label
                    sprintf(
                        s, "%2d",
                        context.modifiedSettings.scheduler.simpleOnSchedule.timeHour
                    );
                    Text_draw(
                        s,
                        5,
                        PositionAfter("TIME:"),
                        0,
                        InvertForSelectionIndex(2)
                    );

                    // Time minute value label
                    sprintf(
                        s, "%02d",
                        context.modifiedSettings.scheduler.simpleOnSchedule.timeMinute
                    );
                    Text_draw(
                        s,
                        5,
                        PositionAfter("TIME: xx:"),
                        0,
                        InvertForSelectionIndex(3)
                    );
                    break;
                }
            }

            switch (context.modifiedSettings.scheduler.simpleOffSchedule.type) {
                case Settings_ScheduleType_Sunrise:
                case Settings_ScheduleType_Sunset: {
                    // Offset value label
                    char s[10];
                    sprintf(s, "%+3.2d MIN", context.modifiedSettings.scheduler.simpleOffSchedule.sunOffset);
                    Text_draw(
                        s,
                        7,
                        PositionAfter("OFFSET:"),
                        0,
                        InvertForSelectionIndex(5)
                    );
                    break;
                }

                case Settings_ScheduleType_Time: {
                    char s[3];

                    // Time hour value label
                    sprintf(
                        s, "%2d",
                        context.modifiedSettings.scheduler.simpleOnSchedule.timeHour
                    );
                    Text_draw(
                        s,
                        5,
                        PositionAfter("TIME:"),
                        0,
                        InvertForSelectionIndex(5)
                    );

                    // Time minute value label
                    sprintf(
                        s, "%02d",
                        context.modifiedSettings.scheduler.simpleOnSchedule.timeMinute
                    );
                    Text_draw(
                        s,
                        5,
                        PositionAfter("TIME: xx:"),
                        0,
                        InvertForSelectionIndex(6)
                    );
                    break;
                }
            }
#endif
            break;
        }

        case Settings_SchedulerType_Off:
            break;
    }

    context.newSchedulerPage.schedulerTypeChanged = false;
    context.newSchedulerPage.onScheduleTypeChanged = false;
    context.newSchedulerPage.offScheduleTypeChanged = false;
    context.newSchedulerPage.selectionChanged = false;
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

        case Page_NewScheduler:
            drawNewSchedulerPage(redraw);
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
        context.modifiedSettings.scheduler.segmentData,
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

static void adjustNewSchedulerSetting()
{
    #define RotateScheduleType(_Type) { \
        if ((_Type) >= Settings_ScheduleType_Sunset) {\
            (_Type) = Settings_ScheduleType_Time; \
        } \
    }

    // Scheduler type
    if (context.newSchedulerPage.selection == 0) {
        if (++context.modifiedSettings.scheduler.type > Settings_SchedulerType_Off) {
            context.modifiedSettings.scheduler.type = Settings_SchedulerType_Segment;
        }
        context.newSchedulerPage.schedulerTypeChanged = true;
    } else {
        switch (context.modifiedSettings.scheduler.type) {
            case Settings_SchedulerType_Simple:
                // On schedule type selection
                if (context.newSchedulerPage.selection == 1) {
                    RotateScheduleType(context.modifiedSettings.scheduler.simpleOnSchedule.type)
                    context.newSchedulerPage.onScheduleTypeChanged = true;
                }
                // Off schedule type selection
                else if (context.newSchedulerPage.selection == 4) {
                    RotateScheduleType(context.modifiedSettings.scheduler.simpleOffSchedule.type)
                    context.newSchedulerPage.offScheduleTypeChanged = true;
                }

                #if 0
                // On/Off schedule details adjustment
                else if (
                    context.newSchedulerPage.selection == 2
                    || context.newSchedulerPage.selection == 4
                ) {
                    struct Schedule* s =
                        context.newSchedulerPage.selection == 2
                            ? &context.modifiedSettings.scheduler.simpleOnSchedule
                            : &context.modifiedSettings.scheduler.simpleOffSchedule;

                    switch (s->type) {
                        case Settings_ScheduleType_Sunrise:
                        case Settings_ScheduleType_Sunset:
                            s->sunOffset += 15;
                            if (s->sunOffset >= 60) {
                                s->sunOffset = -60;
                            }
                            break;

                        case Settings_ScheduleType_Time:

                            break;
                    }
                }
                #endif
                break;

            default:
                break;
        }
    }

    drawCurrentPage(false);
}

void SettingsScreen_init()
{
    memset(&context, 0, sizeof(struct SettingsScreenContext));
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

                    // Reset intermediate values on leaving Configuration mode
                    context.schedulerPage.segmentIndex = 0;
                    context.newSchedulerPage.selection = 0;

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
                    // New scheduler: SELECT
                    else if (context.pageIndex == Page_NewScheduler) {
                        uint8_t limit = 6;

                        if (++context.newSchedulerPage.selection >= limit) {
                            context.newSchedulerPage.selection = 0;
                        }

                        context.newSchedulerPage.selectionChanged = true;
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
                    // New scheduler: ADJUST
                    else if (context.pageIndex == Page_NewScheduler) {
                        adjustNewSchedulerSetting();
                    }
                    break;

                default:
                    break;
            }
            break;
    }

    return true;
}

#pragma warning pop