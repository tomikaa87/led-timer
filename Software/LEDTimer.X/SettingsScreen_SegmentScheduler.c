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

#if !NIGHTLIGHT_TIMER

#include "Clock.h"
#include "Graphics.h"
#include "Keypad.h"
#include "SettingsScreen_SegmentScheduler.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static struct SettingScreen_SegmentScheduler_Context {
    struct Scheduler* settings;
    uint8_t segmentIndex;
} context;

void SettingsScreen_SegmentScheduler_init(struct Scheduler* settings)
{
    context.settings = settings;
    context.segmentIndex = 0;
}

void SettingsScreen_SegmentScheduler_update(const bool redraw)
{
    if (redraw) {
        Graphics_DrawScreenTitle("Segment schd.");
        Graphics_DrawKeypadHelpBar(Graphics_ExitIcon, Graphics_SetIcon, Graphics_ClearIcon);
    }

    Clock_Time minutesSinceMidnightForSegment =
        (Clock_Time)context.segmentIndex * 30;
    uint8_t hours = (uint8_t)(minutesSinceMidnightForSegment / 60);
    uint8_t minutes = (uint8_t)(minutesSinceMidnightForSegment - hours * 60);

    char s[6];
    sprintf(s, "%02u:%02u", hours, minutes);

    Text_draw7Seg(
        s,
        1,
        64 - Text_calculateWidth7Seg(s) / 2,
        false
    );

    Graphics_drawScheduleSegmentIndicator(4, context.segmentIndex, 0);
    Graphics_drawScheduleBar(5, context.settings->segmentData, 0);
}

static void adjustScheduleSegmentAndStepForward(const bool set)
{
    Types_setScheduleSegmentBit(
        context.settings->segmentData,
        context.segmentIndex,
        set
    );

    if (++context.segmentIndex >= 48) {
        context.segmentIndex = 0;
    }
}

bool SettingsScreen_SegmentScheduler_handleKeyPress(const uint8_t keyCode, const bool hold)
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
            adjustScheduleSegmentAndStepForward(true);
            SettingsScreen_SegmentScheduler_update(false);
            break;
        }

        // Adjust
        case Keypad_Key3: {
            adjustScheduleSegmentAndStepForward(false);
            SettingsScreen_SegmentScheduler_update(false);
            break;
        }
    }

    return true;
}

#endif