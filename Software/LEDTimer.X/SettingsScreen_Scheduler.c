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
    uint8_t selection : 3;
    uint8_t schedulerTypeChanged : 1;
    uint8_t onTriggerChanged : 1;
    uint8_t offTriggerChanged : 1;
    uint8_t selectionChanged : 1;
    uint8_t reserved : 1;
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
    static const char* TriggerTypes[] = { "TIME", "SUNRISE", "SUNSET" };

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
        Text_draw(s, 1, 127 - CalculateTextWidth("00"), 0, false);
    }

    // Draw the fixed labels
    if (redraw) {
        SSD1306_fillArea(0, 1, 127, 6, SSD1306_COLOR_BLACK);

        Text_draw("TYPE:", 1, 0, 0, false);
    }

    if (redraw || context.schedulerTypeChanged) {
        // Size the background fill for the longest text (+1 because of the inverted text)
        SSD1306_fillArea(PositionAfter("TYPE:"), 1, CalculateTextWidth("SEGMENT") + 1, 1, SSD1306_COLOR_BLACK);
        // Clear the background of the contents
        SSD1306_fillArea(0, 2, 127, 5, SSD1306_COLOR_BLACK);
    }

    if (
        redraw
        || context.schedulerTypeChanged
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

    if (
        redraw
        || context.schedulerTypeChanged
        || context.onTriggerChanged
        || context.offTriggerChanged
    ) {
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

                switch (context.settings->onTrigger.type) {
                    case Settings_TriggerType_Sunrise:
                    case Settings_TriggerType_Sunset: {
                        // Switch on schedule setting label
                        uint8_t x = LeftText("OFFSET:", 4);
                        SSD1306_fillArea(x, 4, 127 - x, 1, SSD1306_COLOR_BLACK);
                        break;
                    }

                    case Settings_TriggerType_Time: {
                        // Switch on time setting label
                        uint8_t x = LeftText("TIME:", 4);
                        SSD1306_fillArea(x, 4, 127 - x, 1, SSD1306_COLOR_BLACK);
                        // Time hour-minute separator
                        Text_draw(":", 4, PositionAfter("TIME: xx"), 0, false);
                        break;
                    }
                }

                switch (context.settings->offTrigger.type) {
                    case Settings_TriggerType_Sunrise:
                    case Settings_TriggerType_Sunset: {
                        // Switch off schedule setting label
                        uint8_t x = LeftText("OFFSET:", 6);
                        SSD1306_fillArea(x, 6, 127 - x, 1, SSD1306_COLOR_BLACK);
                        break;
                    }

                    case Settings_TriggerType_Time: {
                        // Switch off time setting label
                        uint8_t x = LeftText("TIME:", 6);
                        SSD1306_fillArea(x, 6, 127 - x, 1, SSD1306_COLOR_BLACK);
                        // Time hour-minute separator
                        Text_draw(":", 6, PositionAfter("TIME: xx"), 0, false);
                        break;
                    }
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
                redraw
                || context.onTriggerChanged
                || context.schedulerTypeChanged
                || context.selectionChanged
            ) {
                uint8_t x = Text_draw(
                    TriggerTypes[context.settings->onTrigger.type],
                    3, PositionAfter("ON:"), 0, InvertForSelectionIndex(1)
                );
                // Clean the background after the text
                SSD1306_fillArea(x, 3, 127 - x, 1, SSD1306_COLOR_BLACK);
            }

            // Switch off schedule type
            if (
                redraw
                || context.offTriggerChanged
                || context.schedulerTypeChanged
                || context.selectionChanged
            ) {
                uint8_t x = Text_draw(
                    TriggerTypes[context.settings->offTrigger.type],
                    5, PositionAfter("OFF:"), 0, InvertForSelectionIndex(4)
                );
                // Clean the background after the text
                SSD1306_fillArea(x, 5, 127 - x, 1, SSD1306_COLOR_BLACK);
            }

#if 0
            switch (context.settings->onTrigger.type) {
                case Settings_TriggerType_Sunrise:
                case Settings_TriggerType_Sunset: {
                    // Offset value label
                    char s[10];
                    sprintf(s, "%+3.2d MIN", context.settings->onTrigger.sunOffset);
                    Text_draw(s, 5, PositionAfter("OFFSET:"), 0, InvertForSelectionIndex(2));
                    break;
                }

                case Settings_TriggerType_Time: {
                    char s[3];

                    // Time hour value label
                    sprintf(
                        s, "%2d",
                        context.settings->onTrigger.timeHour
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
                        context.settings->onTrigger.timeMinute
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

            switch (context.settings->offTrigger.type) {
                case Settings_TriggerType_Sunrise:
                case Settings_TriggerType_Sunset: {
                    // Offset value label
                    char s[10];
                    sprintf(s, "%+3.2d MIN", context.settings->offTrigger.sunOffset);
                    Text_draw(
                        s,
                        7,
                        PositionAfter("OFFSET:"),
                        0,
                        InvertForSelectionIndex(5)
                    );
                    break;
                }

                case Settings_TriggerType_Time: {
                    char s[3];

                    // Time hour value label
                    sprintf(
                        s, "%2d",
                        context.settings->onTrigger.timeHour
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
                        context.settings->onTrigger.timeMinute
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
    context.onTriggerChanged = false;
    context.offTriggerChanged = false;
    context.selectionChanged = false;
}

/*
    Selection index table:

        Index | 0        | 1       | 2           | 3      | 4        | 5            | 6
    Mode      |          |         |             |        |          |              |
    --------------------------------------------------------------------------------------------
    Time/Time | Type sel | ON trig | ON hour     | ON min | OFF trig | OFF hour     | OFF min
    Sun /Time | Type sel | ON trig | ON Sun offs | -      | OFF trig | OFF Sun offs | -
    Time/Sun  | Type sel | ON trig | ON hour     | ON min | OFF trig | OFF hour     | OFF min
    Sun /Sun  | Type sel | ON trig | ON Sun offs | -      | OFF trig | OFF Sun offs | -
*/

static void selectNextItem()
{
    if (context.settings->type != Settings_SchedulerType_Simple) {
        context.selection = 0;
        return;
    }

    ++context.selection;
    context.selectionChanged = true;

    if (context.selection == 3 && context.settings->onTrigger.type != Settings_TriggerType_Time) {
        ++context.selection;
    }

    if (context.selection == 6 && context.settings->offTrigger.type != Settings_TriggerType_Time) {
        ++context.selection;
    }

    if (context.selection > 6) {
        context.selection = 0;
    }
}

static void adjustSelectedItem()
{
    #define RotateTriggerType(_Type) { \
        if (++(_Type) > Settings_TriggerType_Sunset) {\
            (_Type) = Settings_TriggerType_Time; \
        } \
    }

    #define RotateHour(_Value) { \
        if (++(_Value) >= 24) { \
            (_Value) = 0; \
        } \
    }

    #define RotateMinute(_Value) { \
        if (++(_Value) >= 60) { \
            (_Value) = 0; \
        } \
    }

    #define RotateSunOffset(_Value) { \
        (_Value) += 15; \
        if ((_Value) >= 60) { \
            (_Value) = -60; \
        } \
    }

    switch (context.selection) {
        case 0: {
            if (++(context.settings->type) > Settings_SchedulerType_Off) {
                context.settings->type = Settings_SchedulerType_Segment;
            }
            context.schedulerTypeChanged = true;
            break;
        }

        case 1: {
            RotateTriggerType(context.settings->onTrigger.type)
            context.onTriggerChanged = true;
            break;
        }

        case 2: {
            switch (context.settings->onTrigger.type) {
                case Settings_TriggerType_Time: {
                    RotateHour(context.settings->onTrigger.timeHour);
                    break;
                }

                case Settings_TriggerType_Sunrise:
                case Settings_TriggerType_Sunset: {
                    RotateSunOffset(context.settings->onTrigger.sunOffset);
                    break;
                }

                default:
                    break;
            }
            break;
        }

        case 3: {
            if (context.settings->onTrigger.type == Settings_TriggerType_Time) {
                RotateMinute(context.settings->onTrigger.timeMinute);
            }
            break;
        }

        case 4: {
            RotateTriggerType(context.settings->offTrigger.type)
            context.offTriggerChanged = true;
            break;
        }

        case 5: {
            switch (context.settings->offTrigger.type) {
                case Settings_TriggerType_Time: {
                    RotateHour(context.settings->offTrigger.timeHour);
                    break;
                }

                case Settings_TriggerType_Sunrise:
                case Settings_TriggerType_Sunset: {
                    RotateSunOffset(context.settings->offTrigger.sunOffset);
                    break;
                }

                default:
                    break;
            }
            break;
        }

        case 6: {
            if (context.settings->offTrigger.type == Settings_TriggerType_Time) {
                RotateMinute(context.settings->offTrigger.timeMinute);
            }
            break;
        }

        default:
            break;
    }
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
            selectNextItem();
            SettingsScreen_Scheduler_update(false);
            break;
        }

        // Adjust
        case Keypad_Key3: {
            adjustSelectedItem();
            SettingsScreen_Scheduler_update(false);
            break;
        }
    }

    return true;
}
