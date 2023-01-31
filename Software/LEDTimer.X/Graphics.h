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

#define Graphics_NextIconWidth                      7
extern const uint8_t Graphics_NextIcon[Graphics_NextIconWidth];

#define Graphics_SelectIconWidth                    8
extern const uint8_t Graphics_SelectIcon[Graphics_SelectIconWidth];

#define Graphics_HeaderLeftCapIconWidth             7
extern const uint8_t Graphics_HeaderLeftCapIcon[Graphics_HeaderLeftCapIconWidth];

#define Graphics_HeaderRightCapIconWidth            7
extern const uint8_t Graphics_HeaderRightCapIcon[Graphics_HeaderRightCapIconWidth];

void Graphics_drawBitmap(
    const uint8_t* bitmap,
    uint8_t width,
    uint8_t x,
    uint8_t page,
    bool invert
);

void Graphics_drawMultipageBitmap(
    const uint8_t* bitmap,
    uint8_t width,
    uint8_t pageCount,
    uint8_t x,
    uint8_t startPage,
    bool invert
);

void Graphics_drawScheduleBar(
    uint8_t line,
    const ScheduleSegmentData segmentData,
    bool invert,
    bool flip
);

void Graphics_drawScheduleSegmentIndicator(
    uint8_t line,
    uint8_t segmentIndex,
    bool invert,
    bool flip
);

#define Graphics_DrawIcon(_Column, _Line, _Icon) \
    SSD1306_enablePageAddressing(); \
    SSD1306_setStartColumn((_Column)); \
    SSD1306_setPage((_Line)); \
    SSD1306_sendData((_Icon), sizeof((_Icon)), 0, false)

#define Graphics_DrawCenterIcon(_Line, _Icon) \
    Graphics_DrawIcon(64 - sizeof((_Icon)) / 2, _Line, _Icon)

#define Graphics_DrawRightIcon(_Line, _Icon) \
    Graphics_DrawIcon(128 - sizeof((_Icon)), _Line, _Icon)

#define Graphics_DrawKeypadHelpBar(_Icon1, _Icon2, _Icon3) \
    SSD1306_enablePageAddressing(); \
    SSD1306_setPage(7); \
    SSD1306_setStartColumn(0); \
    SSD1306_sendData((_Icon1), sizeof((_Icon1)), 0, false); \
    SSD1306_setStartColumn(32 - sizeof(Graphics_KeypadHelpBarSeparator) / 2); \
    SSD1306_sendData(Graphics_KeypadHelpBarSeparator, sizeof(Graphics_KeypadHelpBarSeparator), 0, false); \
    SSD1306_setStartColumn(64 - sizeof((_Icon2)) / 2); \
    SSD1306_sendData((_Icon2), sizeof((_Icon2)), 0, false); \
    SSD1306_setStartColumn(96 - sizeof(Graphics_KeypadHelpBarSeparator) / 2); \
    SSD1306_sendData(Graphics_KeypadHelpBarSeparator, sizeof(Graphics_KeypadHelpBarSeparator), 0, false); \
    SSD1306_setStartColumn(128 - sizeof((_Icon3))); \
    SSD1306_sendData((_Icon3), sizeof((_Icon3)), 0, false)

#define Graphics_DrawKeypadHelpBarSeparators() \
    SSD1306_enablePageAddressing(); \
    SSD1306_setPage(7); \
    SSD1306_setStartColumn(32 - sizeof(Graphics_KeypadHelpBarSeparator) / 2); \
    SSD1306_sendData(Graphics_KeypadHelpBarSeparator, sizeof(Graphics_KeypadHelpBarSeparator), 0, false); \
    SSD1306_setStartColumn(96 - sizeof(Graphics_KeypadHelpBarSeparator) / 2); \
    SSD1306_sendData(Graphics_KeypadHelpBarSeparator, sizeof(Graphics_KeypadHelpBarSeparator), 0, false)

#define Graphics_DrawScreenTitle(_Text) {\
    SSD1306_enablePageAddressing(); \
    SSD1306_setPage(0); \
    uint8_t _Pos = 64 - CalculateTextWidth(_Text) / 2 - sizeof(Graphics_HeaderLeftCapIcon) - 2; \
    SSD1306_setStartColumn(_Pos); \
    SSD1306_sendData(Graphics_HeaderLeftCapIcon, sizeof(Graphics_HeaderLeftCapIcon), 0, false); \
    _Pos = Text_draw(_Text, 0, _Pos + sizeof(Graphics_HeaderLeftCapIcon) + 2, 0, false); \
    SSD1306_setStartColumn(_Pos + 1); \
    SSD1306_sendData(Graphics_HeaderRightCapIcon, sizeof(Graphics_HeaderRightCapIcon), 0, false); \
}