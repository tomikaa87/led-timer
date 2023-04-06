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
    Created on 2022-11-29
*/

#pragma once

#include "SSD1306.h"
#include "Text.h"
#include "Types.h"

#include <stdbool.h>
#include <stdint.h>

#define Graphics_BulbIconWidth                      15
#define Graphics_BulbIconPages                      3
extern const uint8_t Graphics_BulbIcon[Graphics_BulbIconPages][Graphics_BulbIconWidth];

#define Graphics_BatteryIndicatorCapWidth           3
extern const uint8_t Graphics_BatteryIndicatorCap[Graphics_BatteryIndicatorCapWidth];

#define Graphics_BatteryIndicatorBodyFullWidth      1
extern const uint8_t Graphics_BatteryIndicatorBodyFull[Graphics_BatteryIndicatorBodyFullWidth];

#define Graphics_BatteryIndicatorBodyEmptyWidth     1
extern const uint8_t Graphics_BatteryIndicatorBodyEmpty[Graphics_BatteryIndicatorBodyEmptyWidth];

#define Graphics_BatteryIndicatorEndCapWidth        2
extern const uint8_t Graphics_BatteryIndicatorEndCap[Graphics_BatteryIndicatorEndCapWidth];

#define Graphics_ExternalPowerIndicatorWidth        15
extern const uint8_t Graphics_ExternalPowerIndicator[Graphics_ExternalPowerIndicatorWidth];

#define Graphics_KeypadHelpBarSeparatorWidth        12
extern const uint8_t Graphics_KeypadHelpBarSeparator[Graphics_KeypadHelpBarSeparatorWidth];

#define Graphics_ExitIconWidth                      12
extern const uint8_t Graphics_ExitIcon[Graphics_ExitIconWidth];

#define Graphics_ArrowDownIconWidth                 7
extern const uint8_t Graphics_ArrowDownIcon[Graphics_ArrowDownIconWidth];

#define Graphics_ArrowRightIconWidth                7
extern const uint8_t Graphics_ArrowRightIcon[Graphics_ArrowRightIconWidth];

#define Graphics_SelectIconWidth                    8
extern const uint8_t Graphics_SelectIcon[Graphics_SelectIconWidth];

#define Graphics_HeaderLeftCapIconWidth             7
extern const uint8_t Graphics_HeaderLeftCapIcon[Graphics_HeaderLeftCapIconWidth];

#define Graphics_AdjustIconWidth                    12
extern const uint8_t Graphics_AdjustIcon[Graphics_AdjustIconWidth];

#define Graphics_SetIconWidth                       8
extern const uint8_t Graphics_SetIcon[Graphics_SetIconWidth];

#define Graphics_ClearIconWidth                     8
extern const uint8_t Graphics_ClearIcon[Graphics_ClearIconWidth];

#define Graphics_SunOnTheHorizonIconWidth           11
extern const uint8_t Graphics_SunOnTheHorizonIcon[Graphics_SunOnTheHorizonIconWidth];

#define Graphics_MiniBulIconWidth                   5
extern const uint8_t Graphics_MiniBulbIcon[Graphics_MiniBulIconWidth];

enum {
    GRAPHICS_DRAW_INVERT =  SSD1306_SEND_INVERT,
    GRAPHICS_DRAW_FLIPX =   SSD1306_SEND_FLIPX,
    GRAPHICS_DRAW_FLIPY =   SSD1306_SEND_FLIPY
};

enum {
    GRAPHICS_DRAW_SCHEDULE_BAR_INVERT = (1 << 0),
    GRAPHICS_DRAW_SCHEDULE_BAR_FLIP =   (1 << 1)
};

void Graphics_drawBitmap(
    const uint8_t* bitmap,
    uint8_t width,
    uint8_t x,
    uint8_t page,
    uint8_t flags
);

void Graphics_drawMultipageBitmap(
    const uint8_t* bitmap,
    uint8_t width,
    uint8_t pageCount,
    uint8_t x,
    uint8_t startPage,
    uint8_t flags
);

void Graphics_drawScheduleBar(
    uint8_t line,
    const ScheduleSegmentData segmentData,
    uint8_t flags
);

void Graphics_drawScheduleSegmentIndicator(
    uint8_t line,
    uint8_t segmentIndex,
    uint8_t flags
);

#define Graphics_DrawIcon(_Column, _Line, _Icon) \
    Graphics_drawBitmap((_Icon), sizeof((_Icon)), (_Column), (_Line), 0)

#define Graphics_DrawCenterIcon(_Line, _Icon) \
    Graphics_DrawIcon(64 - sizeof((_Icon)) / 2, (_Line), (_Icon))

#define Graphics_DrawRightIcon(_Line, _Icon) \
    Graphics_DrawIcon(128 - sizeof((_Icon)), (_Line), (_Icon))

#define Graphics_DrawKeypadHelpBar(_Icon1, _Icon2, _Icon3) \
    Graphics_drawKeypadHelpBarSeparators(); \
    SSD1306_setStartColumn(0); \
    SSD1306_sendData((_Icon1), sizeof((_Icon1))); \
    SSD1306_setStartColumn(64 - sizeof((_Icon2)) / 2); \
    SSD1306_sendData((_Icon2), sizeof((_Icon2))); \
    SSD1306_setStartColumn(128 - sizeof((_Icon3))); \
    SSD1306_sendData((_Icon3), sizeof((_Icon3)))

#define Graphics_DrawKeypadHelpBarLeftRight(_Icon1, _Icon3) \
    Graphics_drawKeypadHelpBarSeparators(); \
    SSD1306_setStartColumn(0); \
    SSD1306_sendData((_Icon1), sizeof((_Icon1))); \
    SSD1306_setStartColumn(128 - sizeof((_Icon3))); \
    SSD1306_sendData((_Icon3), sizeof((_Icon3)))

#define Graphics_DrawScreenTitle(_Text) { \
    Graphics_drawScreenTitleHelper( \
        (_Text), \
        64 - CalculateTextWidth((_Text)) / 2 - sizeof(Graphics_HeaderLeftCapIcon) - 2 \
    ); \
}

void Graphics_drawScreenTitleHelper(const char* text, uint8_t pos);
void Graphics_drawKeypadHelpBarSeparators();
void Graphics_drawVerticalLine(uint8_t x, uint8_t line);