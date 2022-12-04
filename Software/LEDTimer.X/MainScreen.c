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
#include "SSD1306.h"
#include "System.h"
#include "Text.h"

#include <stdio.h>

static struct MainScreenContext {
    uint8_t scheduleSegmentIndex;
    bool onBatteryPower;
} context = {
    .scheduleSegmentIndex = 0,
    .onBatteryPower = false
};

static void drawClock()
{
    uint8_t hours = (uint8_t)(Clock_getMinutesSinceMidnight() / 60);
    uint8_t minutes = (uint8_t)(Clock_getMinutesSinceMidnight() - hours * 60);

    char s[6];
    snprintf(s, sizeof(s), "%02u:%02u", hours, minutes);

    Text_draw7Seg(s, 2, 15, false);
}

static void drawScheduleBar()
{
    static const ScheduleSegmentData ssd = {
        0, 0, 0, 0, 0, 0
    };

    Graphics_drawScheduleBar(ssd, false);
}

static void drawScheduleSegmentIndicator(const bool redraw)
{
    uint8_t segmentIndex = (uint8_t)(Clock_getMinutesSinceMidnight() / 30);

    if (context.scheduleSegmentIndex != segmentIndex || redraw) {
        context.scheduleSegmentIndex = segmentIndex;
        Graphics_drawScheduleSegmentIndicator(segmentIndex, false);
    }
}

static void drawBulbIcon(const bool visible)
{
    static const uint8_t X = 127 - Graphics_BulbIconWidth - 15;

    if (visible) {
        Graphics_drawMultipageBitmap(
            (const uint8_t*)Graphics_BulbIcon,
            Graphics_BulbIconWidth,
            Graphics_BulbIconPages,
            X,
            2,
            false
        );
    } else {
        SSD1306_fillArea(
            X,
            2,
            Graphics_BulbIconWidth,
            Graphics_BulbIconPages,
            SSD1306_COLOR_BLACK
        );
    }
}

static void drawPowerIndicator()
{
    if (context.onBatteryPower) {
        static const uint8_t Width =
            Graphics_BatteryIndicatorCapWidth
            + Graphics_BatteryIndicatorBodyFullWidth * 10
            + Graphics_BatteryIndicatorEndCapWidth;

        uint8_t x = 127 - Width;

        SSD1306_fillArea(x, 0, Width, 1, SSD1306_COLOR_BLACK);

        Graphics_drawBitmap(
            (uint8_t*)Graphics_BatteryIndicatorCap,
            Graphics_BatteryIndicatorCapWidth,
            x,
            0,
            false
        );

        x += Graphics_BatteryIndicatorCapWidth;

        for (uint8_t i = 10 - system.monitoring.batteryLevel; i > 0; --i) {
            Graphics_drawBitmap(
                (uint8_t*)Graphics_BatteryIndicatorBodyEmpty,
                Graphics_BatteryIndicatorBodyEmptyWidth,
                x,
                0,
                false
            );

            ++x;
        }

        for (uint8_t i = system.monitoring.batteryLevel; i > 0; --i) {
            Graphics_drawBitmap(
                (uint8_t*)Graphics_BatteryIndicatorBodyFull,
                Graphics_BatteryIndicatorBodyFullWidth,
                x,
                0,
                false
            );

            ++x;
        }

        Graphics_drawBitmap(
            (uint8_t*)Graphics_BatteryIndicatorEndCap,
            Graphics_BatteryIndicatorEndCapWidth,
            x,
            0,
            false
        );
    } else {
        static const uint8_t X = 127 - Graphics_ExternalPowerIndicatorWidth;

        SSD1306_fillArea(X, 0, Graphics_ExternalPowerIndicatorWidth, 1, SSD1306_COLOR_BLACK);

        Graphics_drawBitmap(
            (uint8_t*)Graphics_ExternalPowerIndicator,
            Graphics_ExternalPowerIndicatorWidth,
            X,
            0,
            false
        );
    }
}

void MainScreen_update(const bool redraw)
{
    drawClock();
    drawScheduleSegmentIndicator(redraw);

    if (redraw) {
        drawBulbIcon(true);
        drawScheduleBar();
    }

    if (
        redraw
        || (context.onBatteryPower != system.monitoring.onBatteryPower)
    ) {
        context.onBatteryPower = system.monitoring.onBatteryPower;
        drawPowerIndicator();
    }
}

void MainScreen_handleKeyPress(const uint8_t keyCode)
{

}