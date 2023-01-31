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
    Created on 2022-12-03
*/

#include "Clock.h"
#include "Graphics.h"
#include "MainScreen.h"
#include "Settings.h"
#include "SSD1306.h"
#include "System.h"
#include "Text.h"
#include "Keypad.h"
#include "OutputController.h"

#include <stdio.h>

#pragma warning push
#pragma warning disable 763

static struct MainScreenContext {
    uint8_t scheduleSegmentIndex;
} context = {
    .scheduleSegmentIndex = 0
};

inline static void drawOutputToggleKeyHelp()
{
    static const uint8_t MiniBulbIcon[] = {
        0b00011110,
        0b11100001,
        0b10111001,
        0b11100001,
        0b00011110
    };

    static const uint8_t SwitchOffStateIcon[] = {
        0b00111100,
        0b01111110,
        0b11111111,
        0b11111111,
        0b11111111,
        0b11111111,
        0b11111111,
        0b11111111,
        0b10111101,
        0b10000001,
        0b10000001,
        0b10000001,
        0b10111101,
        0b10100101,
        0b10111101,
        0b10000001,
        0b10000001,
        0b10000001,
        0b01000010,
        0b00111100
    };

    static const uint8_t SwitchOnStateIcon[] = {
        0b00111100,
        0b01000010,
        0b10000001,
        0b10000001,
        0b10000001,
        0b10000001,
        0b10111101,
        0b10000001,
        0b10000001,
        0b10000001,
        0b10000001,
        0b10111101,
        0b11111111,
        0b11111111,
        0b11111111,
        0b11111111,
        0b11111111,
        0b11111111,
        0b01111110,
        0b00111100
    };

    static const uint8_t width =
        sizeof(MiniBulbIcon)
        + sizeof(SwitchOffStateIcon);

    Graphics_drawBitmap(MiniBulbIcon, 5, 64 - width / 2, 7, false);

    Graphics_drawBitmap(
        OutputController_outputEnableTargetState()
            ? SwitchOffStateIcon
            : SwitchOnStateIcon,
        sizeof(SwitchOnStateIcon),
        64 - width / 2 + 7,
        7,
        false
    );
}

inline static void drawKeypadHelpBar()
{
    SSD1306_fillArea(0, 7, 128, 1, SSD1306_COLOR_BLACK);

    Graphics_DrawKeypadHelpBarSeparators();

    static const uint8_t SettingsIcon[] = {
        0b01001001,
        0b01001001,
        0b01001001,
        0b01001001,
        0b01001001,
        0b01001001,
    };

    Graphics_drawBitmap(SettingsIcon, 6, 1, 7, false);

    drawOutputToggleKeyHelp();
}

inline static void drawClock()
{
    uint8_t hours = (uint8_t)(Clock_getMinutesSinceMidnight() / 60);
    uint8_t minutes = (uint8_t)(Clock_getMinutesSinceMidnight() - hours * 60);

    char s[6];
//    snprintf(s, sizeof(s), "%02u:%02u", hours, minutes);
    sprintf(s, "%02u:%02u", hours, minutes);

    Text_draw7Seg(s, 3, 15, false);
}

static inline void drawScheduleBar()
{
    Graphics_drawScheduleBar(0, Settings_data.scheduler.segmentData, false, true);
}

static inline void drawScheduleSegmentIndicator(const bool redraw)
{
    uint8_t segmentIndex = Types_calculateScheduleSegmentIndex(
        Clock_getMinutesSinceMidnight()
    );

    if (context.scheduleSegmentIndex != segmentIndex || redraw) {
        context.scheduleSegmentIndex = segmentIndex;
        Graphics_drawScheduleSegmentIndicator(2, segmentIndex, false, true);
    }
}

inline static void drawBulbIcon(const bool visible)
{
    static const uint8_t X = 127 - Graphics_BulbIconWidth - 15;

    if (visible) {
        Graphics_drawMultipageBitmap(
            (const uint8_t*)Graphics_BulbIcon,
            Graphics_BulbIconWidth,
            Graphics_BulbIconPages,
            X,
            3,
            false
        );
    } else {
        SSD1306_fillArea(
            X,
            3,
            Graphics_BulbIconWidth,
            Graphics_BulbIconPages,
            SSD1306_COLOR_BLACK
        );
    }
}

inline static void drawPowerIndicator()
{
    // TODO use the PowerInputChanged event in the future
    static bool lastPowerInputType = false;

    static const uint8_t BatteryIndicatorWidth =
        Graphics_BatteryIndicatorCapWidth
        + Graphics_BatteryIndicatorBodyFullWidth * 10
        + Graphics_BatteryIndicatorEndCapWidth;

    if (System_isRunningFromBackupBattery() != lastPowerInputType) {
        lastPowerInputType = System_isRunningFromBackupBattery();

        static const uint8_t ClearWidth =
            (BatteryIndicatorWidth > Graphics_ExternalPowerIndicatorWidth)
                ? BatteryIndicatorWidth
                : Graphics_ExternalPowerIndicatorWidth;

        SSD1306_fillArea(127 - ClearWidth, 7, ClearWidth, 1, SSD1306_COLOR_BLACK);
    }

    if (System_isRunningFromBackupBattery()) {
        uint8_t x = 127 - BatteryIndicatorWidth;

        Graphics_drawBitmap(
            (uint8_t*)Graphics_BatteryIndicatorCap,
            Graphics_BatteryIndicatorCapWidth,
            x,
            7,
            false
        );

        x += Graphics_BatteryIndicatorCapWidth;

        for (uint8_t i = 10 - System_getBatteryLevel(); i > 0; --i) {
            Graphics_drawBitmap(
                (uint8_t*)Graphics_BatteryIndicatorBodyEmpty,
                Graphics_BatteryIndicatorBodyEmptyWidth,
                x,
                7,
                false
            );

            ++x;
        }

        for (uint8_t i = System_getBatteryLevel(); i > 0; --i) {
            Graphics_drawBitmap(
                (uint8_t*)Graphics_BatteryIndicatorBodyFull,
                Graphics_BatteryIndicatorBodyFullWidth,
                x,
                7,
                false
            );

            ++x;
        }

        Graphics_drawBitmap(
            (uint8_t*)Graphics_BatteryIndicatorEndCap,
            Graphics_BatteryIndicatorEndCapWidth,
            x,
            7,
            false
        );
    } else {
        static const uint8_t X = 127 - Graphics_ExternalPowerIndicatorWidth;

        Graphics_drawBitmap(
            (uint8_t*)Graphics_ExternalPowerIndicator,
            Graphics_ExternalPowerIndicatorWidth,
            X,
            7,
            false
        );
    }
}

void MainScreen_update(const bool redraw)
{
    if (redraw) {
        drawKeypadHelpBar();
        drawScheduleBar();
    }

    drawBulbIcon(OutputController_isOutputEnabled());
    drawClock();
    drawScheduleSegmentIndicator(redraw);
    drawOutputToggleKeyHelp();
    drawPowerIndicator();
}

inline bool MainScreen_handleKeyPress(const uint8_t keyCode, const bool hold)
{
    switch (keyCode) {
        // Propagate to the UI to show the Settings
        case Keypad_Key1:
            break;

        case Keypad_Key2:
            if (!hold) {
                OutputController_toggle();
                return true;
            }
            break;

        // Key3 does nothing
        case Keypad_Key3:
            return true;
    }

    return false;
}

#pragma warning pop