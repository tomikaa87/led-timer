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

#include "Config.h"
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
    uint8_t onSwitchChanged : 1;
    uint8_t offSwitchChanged : 1;
    uint8_t selectionChanged : 1;
    uint8_t intervalIndexChanged : 1;
    uint8_t activeStateChanged : 1;
    uint8_t intervalIndex : 3;
    uint8_t reserved : 3;
} context;

void SettingsScreen_Scheduler_init(struct Scheduler* settings)
{
    context.settings = settings;
    context.selection = 0;
    context.schedulerTypeChanged = 0;
    context.onSwitchChanged = 0;
    context.offSwitchChanged = 0;
    context.selectionChanged = 0;
    context.intervalIndexChanged = 0;
    context.activeStateChanged = 0;
    context.intervalIndex = 0;
}

void SettingsScreen_Scheduler_update(const bool redraw)
{
    if (redraw) {
        Graphics_DrawScreenTitle("SCHEDULER");
        Graphics_DrawKeypadHelpBar(Graphics_ExitIcon, Graphics_ArrowDownIcon, Graphics_AdjustIcon);
    }

    static const char* SchedulerTypes[] = { "INTERVAL", "SEGMENT", "OFF" };
    static const char* SwitchTypes[] = { "TIME", "SUNRISE", "SUNSET" };

    #define InvertForSelectionIndex(_Index) (\
        context.selection == _Index \
    )

    #define PositionAfter(_Label) (\
        CalculateTextWidth(_Label) + 5 /* space */ \
    )

#if 0
    // Debug: selection index
    {
        char s[3];
        sprintf(s, "%02u", context.selection);
        Text_draw(s, 1, 128 - CalculateTextWidth("00"), 0, false);
    }
#endif

    // Draw the fixed labels
    if (redraw) {
        SSD1306_fillArea(0, 1, 128, 6, SSD1306_COLOR_BLACK);

        Text_draw("TYPE:", 1, 0, 0, false);

        // Trigger update of all fields
        context.schedulerTypeChanged = true;
        context.onSwitchChanged = true;
        context.offSwitchChanged = true;
        context.selectionChanged = true;
        context.intervalIndexChanged = true;
        context.activeStateChanged = true;
    }

    if (context.schedulerTypeChanged) {
        // Size the background fill for the longest text (+1 because of the inverted text)
        SSD1306_fillArea(PositionAfter("TYPE:"), 1, CalculateTextWidth("INTERVAL") + 1, 1, SSD1306_COLOR_BLACK);
        // Clear the background of the contents
        SSD1306_fillArea(0, 2, 128, 5, SSD1306_COLOR_BLACK);
    }

    if (context.schedulerTypeChanged || context.selectionChanged) {
        // Scheduler type
        Text_draw(
            SchedulerTypes[context.settings->type],
            1,
            PositionAfter("TYPE:"),
            0,
            InvertForSelectionIndex(0)
        );
    }

    if (context.settings->type == Settings_SchedulerType_Interval) {
        if (context.schedulerTypeChanged) {
            // Interval scheduler program index title
            LeftText("SCHEDULE:", 2);
            // Interval active state title
            Text_draw("ACTIVE:", 2, PositionAfter("SCHEDULE: x"), 0, false);
        }

        if (context.schedulerTypeChanged || context.intervalIndexChanged || context.selectionChanged) {
            // Interval scheduler program index
            char s[2];
            sprintf(s, "%u", context.intervalIndex + 1);
            Text_draw(s, 2, PositionAfter("SCHEDULE:"), 0, InvertForSelectionIndex(1));
        }

        if (context.schedulerTypeChanged || context.activeStateChanged || context.selectionChanged) {
            // Interval active state value
            Text_draw(
                context.settings->intervals[context.intervalIndex].active ? "1" : "0",
                2,
                PositionAfter("SCHEDULE: x ACTIVE:"),
                false,
                InvertForSelectionIndex(2)
            );
        }
    }

    if (
        context.schedulerTypeChanged
        || context.onSwitchChanged
        || context.offSwitchChanged
        || context.intervalIndexChanged
    ) {
        // Draw the fixed labels for the current scheduler type
        switch (context.settings->type) {
            case Settings_SchedulerType_Segment:
                CenterText("CHANGE SETTINGS", 3);
                CenterText("ON SGMT. SCHD. SCREEN", 4);
                break;

            case Settings_SchedulerType_Interval:
                // Switch on schedule label
                LeftText("ON:", 3);
                // Switch off schedule label
                LeftText("OFF:", 5);

                switch (context.settings->intervals[context.intervalIndex].onSwitch.type) {
                    case Settings_IntervalSwitchType_Sunrise:
                    case Settings_IntervaSwitchType_Sunset: {
                        // Switch on schedule setting label
                        uint8_t x = LeftText("OFFSET:", 4);
                        SSD1306_fillArea(x, 4, 128 - x, 1, SSD1306_COLOR_BLACK);
                        // Unit label
                        Text_draw("MIN", 4, PositionAfter("OFFSET: xxx"), 0, false);
                        break;
                    }

                    case Settings_IntervalSwitchType_Time: {
                        // Switch on time setting label
                        uint8_t x = LeftText("TIME:", 4);
                        SSD1306_fillArea(x, 4, 128 - x, 1, SSD1306_COLOR_BLACK);
                        // Time hour-minute separator
                        Text_draw(":", 4, CalculateTextWidth("TIME: xx"), 0, false);
                        break;
                    }
                }

                switch (context.settings->intervals[context.intervalIndex].offSwitch.type) {
                    case Settings_IntervalSwitchType_Sunrise:
                    case Settings_IntervaSwitchType_Sunset: {
                        // Switch off schedule setting label
                        uint8_t x = LeftText("OFFSET:", 6);
                        SSD1306_fillArea(x, 6, 128 - x, 1, SSD1306_COLOR_BLACK);
                        // Unit label
                        Text_draw("MIN", 6, PositionAfter("OFFSET: xxx"), 0, false);
                        break;
                    }

                    case Settings_IntervalSwitchType_Time: {
                        // Switch off time setting label
                        uint8_t x = LeftText("TIME:", 6);
                        SSD1306_fillArea(x, 6, 128 - x, 1, SSD1306_COLOR_BLACK);
                        // Time hour-minute separator
                        Text_draw(":", 6, CalculateTextWidth("TIME: xx"), 0, false);
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

        case Settings_SchedulerType_Interval: {
            // Switch on schedule type
            if (
                context.onSwitchChanged
                || context.schedulerTypeChanged
                || context.selectionChanged
                || context.intervalIndexChanged
            ) {
                uint8_t x = Text_draw(
                    SwitchTypes[context.settings->intervals[context.intervalIndex].onSwitch.type],
                    3, PositionAfter("ON:"), 0, InvertForSelectionIndex(3)
                );
                // Clean the background after the text
                SSD1306_fillArea(x, 3, 128 - x, 1, SSD1306_COLOR_BLACK);
            }

            // Switch off schedule type
            if (
                context.offSwitchChanged
                || context.schedulerTypeChanged
                || context.selectionChanged
                || context.intervalIndexChanged
            ) {
                uint8_t x = Text_draw(
                    SwitchTypes[context.settings->intervals[context.intervalIndex].offSwitch.type],
                    5, PositionAfter("OFF:"), 0, InvertForSelectionIndex(6)
                );
                // Clean the background after the text
                SSD1306_fillArea(x, 5, 128 - x, 1, SSD1306_COLOR_BLACK);
            }

            switch (context.settings->intervals[context.intervalIndex].onSwitch.type) {
                case Settings_IntervalSwitchType_Sunrise:
                case Settings_IntervaSwitchType_Sunset: {
                    // Offset value label
                    char s[4];
                    sprintf(s, "%+3.2d", context.settings->intervals[context.intervalIndex].onSwitch.sunOffset);
                    Text_draw(s, 4, PositionAfter("OFFSET:"), 0, InvertForSelectionIndex(4));
                    break;
                }

                case Settings_IntervalSwitchType_Time: {
                    char s[3];

                    // Time hour value label
                    sprintf(s, "%2d", context.settings->intervals[context.intervalIndex].onSwitch.timeHour);
                    Text_draw(s, 4, PositionAfter("TIME:"), 0, InvertForSelectionIndex(4));

                    // Time minute value label
                    sprintf(s, "%02d", context.settings->intervals[context.intervalIndex].onSwitch.timeMinute);
                    Text_draw(s, 4, CalculateTextWidth("TIME: xx:"), 0, InvertForSelectionIndex(5));
                    break;
                }
            }

            switch (context.settings->intervals[context.intervalIndex].offSwitch.type) {
                case Settings_IntervalSwitchType_Sunrise:
                case Settings_IntervaSwitchType_Sunset: {
                    // Offset value label
                    char s[4];
                    sprintf(s, "%+3.2d", context.settings->intervals[context.intervalIndex].offSwitch.sunOffset);
                    Text_draw(s, 6, PositionAfter("OFFSET:"), 0, InvertForSelectionIndex(7));
                    break;
                }

                case Settings_IntervalSwitchType_Time: {
                    char s[3];

                    // Time hour value label
                    sprintf(s, "%2d", context.settings->intervals[context.intervalIndex].offSwitch.timeHour);
                    Text_draw(s, 6, PositionAfter("TIME:"), 0, InvertForSelectionIndex(7));

                    // Time minute value label
                    sprintf(s, "%02d", context.settings->intervals[context.intervalIndex].offSwitch.timeMinute);
                    Text_draw(s, 6, CalculateTextWidth("TIME: xx:"), 0, InvertForSelectionIndex(8));
                    break;
                }
            }
            break;
        }

        case Settings_SchedulerType_Off:
            break;
    }

    context.schedulerTypeChanged = false;
    context.onSwitchChanged = false;
    context.offSwitchChanged = false;
    context.selectionChanged = false;
    context.intervalIndexChanged = false;
}

/*
    Selection index table:

        Index | 0        | 1      | 2      | 3       | 4           | 5      | 6        | 7            | 8
    Mode      |          |        |        |         |             |        |          |              |
    ----------|----------|--------|--------|---------|-------------|--------|----------|--------------|--------
    Time/Time | Type sel | PgmIdx | Active | ON trig | ON hour     | ON min | OFF trig | OFF hour     | OFF min
    Sun /Time | Type sel | PgmIdx | Active | ON trig | ON Sun offs | -      | OFF trig | OFF Sun offs | -
    Time/Sun  | Type sel | PgmIdx | Active | ON trig | ON hour     | ON min | OFF trig | OFF hour     | OFF min
    Sun /Sun  | Type sel | PgmIdx | Active | ON trig | ON Sun offs | -      | OFF trig | OFF Sun offs | -
*/

static void selectNextItem()
{
    if (context.settings->type != Settings_SchedulerType_Interval) {
        context.selection = 0;
        return;
    }

    ++context.selection;
    context.selectionChanged = true;

    if (context.selection == 5 && context.settings->intervals[context.intervalIndex].onSwitch.type != Settings_IntervalSwitchType_Time) {
        ++context.selection;
    }

    if (context.selection == 8 && context.settings->intervals[context.intervalIndex].offSwitch.type != Settings_IntervalSwitchType_Time) {
        ++context.selection;
    }

    if (context.selection > 8) {
        context.selection = 0;
    }
}

static void adjustSelectedItem()
{
    #define RotateSwitchType(_Type) { \
        if (++(_Type) > Settings_IntervaSwitchType_Sunset) {\
            (_Type) = Settings_IntervalSwitchType_Time; \
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
        if ((_Value) > 60) { \
            (_Value) = -60; \
        } \
    }

    switch (context.selection) {
        case 0: {
            if (++(context.settings->type) > Settings_SchedulerType_Off) {
                context.settings->type = Settings_SchedulerType_Interval;
            }
            context.schedulerTypeChanged = true;
            break;
        }

        case 1: {
            if (++context.intervalIndex >= Config_Settings_IntervalScheduleCount) {
                context.intervalIndex = 0;
            }
            context.intervalIndexChanged = true;
            break;
        }

        case 2: {
            ++context.settings->intervals[context.intervalIndex].active;
            context.activeStateChanged = true;
            break;
        }

        case 3: {
            RotateSwitchType(context.settings->intervals[context.intervalIndex].onSwitch.type);
            context.onSwitchChanged = true;
            break;
        }

        case 4: {
            switch (context.settings->intervals[context.intervalIndex].onSwitch.type) {
                case Settings_IntervalSwitchType_Time: {
                    RotateHour(context.settings->intervals[context.intervalIndex].onSwitch.timeHour);
                    break;
                }

                case Settings_IntervalSwitchType_Sunrise:
                case Settings_IntervaSwitchType_Sunset: {
                    RotateSunOffset(context.settings->intervals[context.intervalIndex].onSwitch.sunOffset);
                    break;
                }

                default:
                    break;
            }
            break;
        }

        case 5: {
            if (context.settings->intervals[context.intervalIndex].onSwitch.type == Settings_IntervalSwitchType_Time) {
                RotateMinute(context.settings->intervals[context.intervalIndex].onSwitch.timeMinute);
            }
            break;
        }

        case 6: {
            RotateSwitchType(context.settings->intervals[context.intervalIndex].offSwitch.type);
            context.offSwitchChanged = true;
            break;
        }

        case 7: {
            switch (context.settings->intervals[context.intervalIndex].offSwitch.type) {
                case Settings_IntervalSwitchType_Time: {
                    RotateHour(context.settings->intervals[context.intervalIndex].offSwitch.timeHour);
                    break;
                }

                case Settings_IntervalSwitchType_Sunrise:
                case Settings_IntervaSwitchType_Sunset: {
                    RotateSunOffset(context.settings->intervals[context.intervalIndex].offSwitch.sunOffset);
                    break;
                }

                default:
                    break;
            }
            break;
        }

        case 8: {
            if (context.settings->intervals[context.intervalIndex].offSwitch.type == Settings_IntervalSwitchType_Time) {
                RotateMinute(context.settings->intervals[context.intervalIndex].offSwitch.timeMinute);
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
