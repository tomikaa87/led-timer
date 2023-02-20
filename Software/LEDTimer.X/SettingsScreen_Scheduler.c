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
#include "SettingsScreen_Scheduler.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static struct SettingScreen_Scheduler_Context {
    struct Scheduler* settings;
    uint8_t selection : 4;
    uint8_t schedulerTypeChanged : 1;
    uint8_t onScheduleTypeChanged : 1;
    uint8_t offScheduleTypeChanged : 1;
    uint8_t selectionChanged : 1;
} context;

void SettingsScreen_Scheduler_init(struct Scheduler* settings)
{
    memset(&context, 0, sizeof(struct SettingScreen_Scheduler_Context));
    context.settings = settings;
}

void SettingsScreen_Scheduler_update(const bool redraw)
{
    if (redraw) {
        Graphics_DrawScreenTitle("SCHEDULER");
        Graphics_DrawKeypadHelpBar(Graphics_ExitIcon, Graphics_NextIcon, Graphics_AdjustIcon);
    }

    static const char* SchedulerTypes[] = { "SEGMENT", "SIMPLE", "OFF" };
    static const char* ScheduleTypes[] = { "TIME", "SUNRISE", "SUNSET" };

    #define InvertForSelectionIndex(_Index) (\
        context.selection == _Index \
    )

    #define PositionAfter(_Label) (\
        CalculateTextWidth(_Label) + 5 /* space */ \
    )

    // Debug: selection index
    {
        char s[3];
        sprintf(s, "%02u", context.selection);
        Text_draw(s, 1, 128 - CalculateTextWidth("00"), 0, false);
    }

    // Draw the fixed labels
    if (redraw) {
        context.schedulerTypeChanged = true;
        context.onScheduleTypeChanged = true;
        context.offScheduleTypeChanged = true;

        SSD1306_fillArea(0, 1, 128, 6, SSD1306_COLOR_BLACK);

        Text_draw("TYPE:", 1, 0, 0, false);
    }

    if (!redraw && context.schedulerTypeChanged) {
        // Size the background fill for the longest text (+1 because of the inverted text)
        SSD1306_fillArea(PositionAfter("TYPE:"), 1, CalculateTextWidth("SEGMENT") + 1, 1, SSD1306_COLOR_BLACK);
        // Clear the background of the contents
        SSD1306_fillArea(0, 2, 128, 5, SSD1306_COLOR_BLACK);
    }

    if (
        context.schedulerTypeChanged
        || context.selectionChanged
    ) {
        // Scheduler type
        Text_draw(
            SchedulerTypes[context.settings->type],
            1,
            PositionAfter("TYPE:"),
            0,
            InvertForSelectionIndex(0)
        );
    }

    if (context.schedulerTypeChanged) {
        // Draw the fixed labels for the current scheduler type
        switch (context.settings->type) {
            case Settings_SchedulerType_Segment:
                CenterText("CHANGE SETTINGS", 4);
                CenterText("ON SGMT. SCHD. SCREEN", 5);
                break;

            case Settings_SchedulerType_Simple:
                // Switch on schedule label
                LeftText("ON:", 3);
                // Switch off schedule label
                LeftText("OFF:", 5);

                switch (context.settings->simpleOnSchedule.type) {
                    case Settings_ScheduleType_Sunrise:
                    case Settings_ScheduleType_Sunset:
                        // Switch on schedule setting label
                        LeftText("OFFSET:", 4);
                        break;

                    case Settings_ScheduleType_Time:
                        // Switch on time setting label
                        LeftText("TIME:", 4);
                        // Time hour-minute separator
                        Text_draw(":", 4, PositionAfter("TIME: xx"), 0, false);
                        break;
                }

                switch (context.settings->simpleOffSchedule.type) {
                    case Settings_ScheduleType_Sunrise:
                    case Settings_ScheduleType_Sunset:
                        // Switch off schedule setting label
                        LeftText("OFFSET:", 6);
                        break;

                    case Settings_ScheduleType_Time:
                        // Switch off time setting label
                        LeftText("TIME:", 6);
                        // Time hour-minute separator
                        Text_draw(":", 6, PositionAfter("TIME: xx"), 0, false);
                        break;
                }
                break;

            case Settings_SchedulerType_Off:
                CenterText("The output will only", 3);
                CenterText("be switched manually", 4);
                break;
        }
    }

    switch (context.settings->type) {
        case Settings_SchedulerType_Segment: {
            break;
        }

        case Settings_SchedulerType_Simple: {
            // Switch on schedule type
            if (
                context.onScheduleTypeChanged
                || context.schedulerTypeChanged
                || context.selectionChanged
            ) {
                uint8_t x = Text_draw(
                    ScheduleTypes[
                        context.settings->simpleOnSchedule.type
                    ],
                    3, PositionAfter("ON:"), 0, InvertForSelectionIndex(1)
                );
                // Clean the background after the text
                SSD1306_fillArea(x, 3, 20, 1, SSD1306_COLOR_BLACK);
            }

            // Switch off schedule type
            if (
                context.offScheduleTypeChanged
                || context.schedulerTypeChanged
                || context.selectionChanged
            ) {
                uint8_t x = Text_draw(
                    ScheduleTypes[
                        context.settings->simpleOffSchedule.type
                    ],
                    5, PositionAfter("OFF:"), 0, InvertForSelectionIndex(4)
                );
                // Clean the background after the text
                SSD1306_fillArea(x, 5, 20, 1, SSD1306_COLOR_BLACK);
            }

#if 0
            switch (context.settings->simpleOnSchedule.type) {
                case Settings_ScheduleType_Sunrise:
                case Settings_ScheduleType_Sunset: {
                    // Offset value label
                    char s[10];
                    sprintf(s, "%+3.2d MIN", context.settings->simpleOnSchedule.sunOffset);
                    Text_draw(s, 5, PositionAfter("OFFSET:"), 0, InvertForSelectionIndex(2));
                    break;
                }

                case Settings_ScheduleType_Time: {
                    char s[3];

                    // Time hour value label
                    sprintf(
                        s, "%2d",
                        context.settings->simpleOnSchedule.timeHour
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
                        context.settings->simpleOnSchedule.timeMinute
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

            switch (context.settings->simpleOffSchedule.type) {
                case Settings_ScheduleType_Sunrise:
                case Settings_ScheduleType_Sunset: {
                    // Offset value label
                    char s[10];
                    sprintf(s, "%+3.2d MIN", context.settings->simpleOffSchedule.sunOffset);
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
                        context.settings->simpleOnSchedule.timeHour
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
                        context.settings->simpleOnSchedule.timeMinute
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

    context.schedulerTypeChanged = false;
    context.onScheduleTypeChanged = false;
    context.offScheduleTypeChanged = false;
    context.selectionChanged = false;
}

bool SettingsScreen_Scheduler_handleKeyPress(const uint8_t keyCode, const bool hold)
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
            uint8_t limit = 6;

            if (++context.selection >= limit) {
                context.selection = 0;
            }

            context.selectionChanged = true;
            break;
        }

        // Adjust
        case Keypad_Key3: {
            #define RotateScheduleType(_Type) { \
                if ((_Type) >= Settings_ScheduleType_Sunset) {\
                    (_Type) = Settings_ScheduleType_Time; \
                } \
            }

            // Scheduler type
            if (context.selection == 0) {
                if (++(context.settings->type) > Settings_SchedulerType_Off) {
                    context.settings->type = Settings_SchedulerType_Segment;
                }
                context.schedulerTypeChanged = true;
            } else {
                switch (context.settings->type) {
                    case Settings_SchedulerType_Simple:
                        // On schedule type selection
                        if (context.selection == 1) {
                            RotateScheduleType(context.settings->simpleOnSchedule.type)
                            context.onScheduleTypeChanged = true;
                        }
                        // Off schedule type selection
                        else if (context.selection == 4) {
                            RotateScheduleType(context.settings->simpleOffSchedule.type)
                            context.offScheduleTypeChanged = true;
                        }

                        #if 0
                        // On/Off schedule details adjustment
                        else if (
                            context.selection == 2
                            || context.selection == 4
                        ) {
                            struct Schedule* s =
                                context.selection == 2
                                    ? &context.settings->simpleOnSchedule
                                    : &context.settings->simpleOffSchedule;

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

            SettingsScreen_Scheduler_update(false);

            break;
        }
    }

    return true;
}
