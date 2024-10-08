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

#include "Graphics.h"
#include "SSD1306.h"
#include "Text.h"

const uint8_t Graphics_BulbIcon[Graphics_BulbIconPages][Graphics_BulbIconWidth] = {
    {
        0b11000000,
        0b00110000,
        0b00001000,
        0b10000100,
        0b01000010,
        0b00100010,
        0b01000001,
        0b10000001,
        0b01000001,
        0b00100010,
        0b01000010,
        0b10000100,
        0b00001000,
        0b00110000,
        0b11000000
    },
    {
        0b00000011,
        0b00001100,
        0b00010000,
        0b11100000,
        0b10000000,
        0b10000000,
        0b01111110,
        0b10000000,
        0b01111110,
        0b10000000,
        0b10000000,
        0b11100000,
        0b00010000,
        0b00001100,
        0b00000011
    },
    {
        0b00000000,
        0b00000000,
        0b00000000,
        0b00001110,
        0b00010010,
        0b01110010,
        0b10000010,
        0b10000010,
        0b10000010,
        0b01110010,
        0b00010010,
        0b00001110,
        0b00000000,
        0b00000000,
        0b00000000
    }
};

const uint8_t Graphics_BatteryIndicatorCap[Graphics_BatteryIndicatorCapWidth] = {
    0b00111100,
    0b01111110,
    0b10000001
};

const uint8_t Graphics_BatteryIndicatorBodyFull[Graphics_BatteryIndicatorBodyFullWidth] = {
    0b10111101
};

const uint8_t Graphics_BatteryIndicatorBodyEmpty[Graphics_BatteryIndicatorBodyEmptyWidth] = {
    0b10000001
};

const uint8_t Graphics_BatteryIndicatorEndCap[Graphics_BatteryIndicatorEndCapWidth] = {
    0b10000001,
    0b01111110
};

const uint8_t Graphics_ExternalPowerIndicator[Graphics_ExternalPowerIndicatorWidth] = {
    0b00100100,
    0b00100100,
    0b11111111,
    0b10000001,
    0b10000001,
    0b10000001,
    0b01000010,
    0b00111100,
    0b00100100,
    0b00011000,
    0b00001000,
    0b00001000,
    0b00110000,
    0b01000000,
    0b01000000
};

const uint8_t Graphics_KeypadHelpBarSeparator[Graphics_KeypadHelpBarSeparatorWidth] = {
    0b10000000,
    0b01000000,
    0b10100000,
    0b01010000,
    0b10101000,
    0b01010100,
    0b00101010,
    0b00010101,
    0b00001010,
    0b00000101,
    0b00000010,
    0b00000001
};

const uint8_t Graphics_ExitIcon[Graphics_ExitIconWidth] = {
    0b11111111,
    0b00000001,
    0b00000001,
    0b00010001,
    0b11111111,
    0b00000000,
    0b00010000,
    0b00111000,
    0b01111100,
    0b00010000,
    0b00010000,
    0b00010000
};

const uint8_t Graphics_ArrowDownIcon[Graphics_ArrowDownIconWidth] = {
    0b00010000,
    0b00110000,
    0b01110000,
    0b11111111,
    0b01110000,
    0b00110000,
    0b00010000
};

const uint8_t Graphics_ArrowRightIcon[Graphics_ArrowRightIconWidth] = {
    0b00001000,
    0b00001000,
    0b00001000,
    0b01111111,
    0b00111110,
    0b00011100,
    0b00001000
};

const uint8_t Graphics_SelectIcon[Graphics_SelectIconWidth] = {
    0b00010000,
    0b00111000,
    0b01111100,
    0b11111110,
    0b00010000,
    0b00010000,
    0b00010000,
    0b00011111
};

const uint8_t Graphics_HeaderLeftCapIcon[Graphics_HeaderLeftCapIconWidth] = {
    0b01000000,
    0b00100000,
    0b01010000,
    0b00101000,
    0b01010100,
    0b00101010,
    0b01010101
};

const uint8_t Graphics_AdjustIcon[Graphics_AdjustIconWidth] = {
    0b00111000,
    0b01000100,
    0b01000100,
    0b01011111,
    0b01001110,
    0b00000100,
    0b00100000,
    0b01110010,
    0b11111010,
    0b00100010,
    0b00100010,
    0b00011100
};

const uint8_t Graphics_SetIcon[Graphics_SetIconWidth] = {
    0b11111111,
    0b10000001,
    0b10111101,
    0b10111101,
    0b10111101,
    0b10111101,
    0b10000001,
    0b11111111
};

const uint8_t Graphics_ClearIcon[Graphics_ClearIconWidth] = {
    0b11111111,
    0b10000001,
    0b10000001,
    0b10000001,
    0b10000001,
    0b10000001,
    0b10000001,
    0b11111111
};

const uint8_t Graphics_SunOnTheHorizonIcon[Graphics_SunOnTheHorizonIconWidth] = {
    0b10100000,
    0b10100100,
    0b10001000,
    0b11100000,
    0b10010000,
    0b10010110,
    0b10010000,
    0b11100000,
    0b10001000,
    0b10100100,
    0b10100000
};

const uint8_t Graphics_MiniBulbIcon[Graphics_MiniBulIconWidth] = {
    0b00011110,
    0b11100001,
    0b10111001,
    0b11100001,
    0b00011110
};

void Graphics_drawBitmap(
    const uint8_t* const bitmap,
    const uint8_t width,
    const uint8_t x,
    const uint8_t page,
    const uint8_t flags
)
{
    if (page > SSD1306_PAGE_COUNT || width == 0 || x + width > SSD1306_LCDWIDTH) {
        return;
    }

    SSD1306_enablePageAddressing();
    SSD1306_setPage(page);
    SSD1306_setStartColumn(x);
    SSD1306_sendData2(bitmap, width, flags);
}

void Graphics_drawMultipageBitmap(
    const uint8_t* bitmap,
    const uint8_t width,
    const uint8_t pageCount,
    const uint8_t x,
    uint8_t startPage,
    const uint8_t flags
)
{
    if (startPage + pageCount > SSD1306_PAGE_COUNT) {
        return;
    }

    for (uint8_t i = pageCount; i > 0; --i) {
        Graphics_drawBitmap(bitmap, width, x, startPage++, flags);
        bitmap += width;
    }
}

void Graphics_drawScheduleBar(
    const uint8_t line,
    const ScheduleSegmentData segmentData,
    const uint8_t flags
)
{
    uint8_t flip = flags & GRAPHICS_DRAW_SCHEDULE_BAR_FLIP;
	uint8_t longTick = flip ? 0b00001111 : 0b11110000;
	uint8_t shortTick = flip ? 0b00001110 : 0b01110000;
	uint8_t segmentActiveIndicator = flip ? 0b11101000 : 0b00010111;
	uint8_t segmentInactiveIndicator = flip ? 0b00001000 : 0b00010000;

	SSD1306_enablePageAddressing();
	SSD1306_setPage(flip ? line + 1 : line);
	SSD1306_setStartColumn(3);

	uint8_t tickCounter = 0;
	uint8_t longTickCounter = 0;
	uint8_t dataByteIdx = 0;
	uint8_t dataBitIdx = 255; // Let it overflow in the first iteration
	uint8_t indicatorCounter = 0;
	uint8_t segmentActive = 0;

	for (uint8_t i = 121; i > 0; --i) {
		uint8_t bitmap;

		if (tickCounter == 0) {
			// Draw ticks
			if (longTickCounter == 0)
				bitmap = longTick;
			else
				bitmap = shortTick;
		} else {
			// Draw rest of the bar with or without the indicators
			if (indicatorCounter < 2 && segmentActive)
				bitmap = segmentActiveIndicator;
			else
				bitmap = segmentInactiveIndicator;
		}

		SSD1306_sendData2(&bitmap, 1, flags & GRAPHICS_DRAW_SCHEDULE_BAR_INVERT);

		if (++tickCounter == 5) {
			tickCounter = 0;
			if (++longTickCounter == 6)
				longTickCounter = 0;
		}

		++indicatorCounter;
		if (tickCounter == 1 || tickCounter == 3) {
			indicatorCounter = 0;
			if (++dataBitIdx == 8) {
				++dataByteIdx;
				dataBitIdx = 0;
			}

			segmentActive = (segmentData[dataByteIdx] >> dataBitIdx) & 1;
		}
	}

    uint8_t textLine = flip ? line : line + 1;
    uint8_t yOffset = flip ? 1 : 0;

	Text_draw("0", textLine, 1, yOffset, flags & GRAPHICS_DRAW_SCHEDULE_BAR_INVERT);
	Text_draw("6", textLine, 31, yOffset, flags & GRAPHICS_DRAW_SCHEDULE_BAR_INVERT);
	Text_draw("12", textLine, 58, yOffset, flags & GRAPHICS_DRAW_SCHEDULE_BAR_INVERT);
	Text_draw("18", textLine, 88, yOffset, flags & GRAPHICS_DRAW_SCHEDULE_BAR_INVERT);
	Text_draw("24", textLine, 115, yOffset, flags & GRAPHICS_DRAW_SCHEDULE_BAR_INVERT);
}

void Graphics_drawScheduleSegmentIndicator(
    const uint8_t line,
    const uint8_t segmentIndex,
    const uint8_t flags
)
{
	static const uint8_t SegmentIndicatorBitmap[] = {
		0b00010000,
		0b00100000,
		0b01111100,
		0b00100000,
		0b00010000
	};

	uint8_t x = 2; // initial offset from left
	x += segmentIndex << 1;	// for every "tick"
	x += segmentIndex >> 1;	// for every padding between "ticks"

	/*
	 0:     v
	 1:     . v
	 2:     . .  v
	 3:     . .  . v
	 4:     . .  . .  v
	 5:     . .  . .  . v
	        |||| |||| ||||
	        01234567890123

		0 -> 0
		1 -> 2
		2 -> 5
		3 -> 7
		4 -> 10
		5 -> 12
	 */

    // Clear the background
	SSD1306_fillArea(0, line, 128, 1, 0);

    Graphics_drawBitmap(
        SegmentIndicatorBitmap,
        sizeof(SegmentIndicatorBitmap),
        x,
        line,
        flags
    );
}

void Graphics_drawScreenTitleHelper(const char* text, uint8_t pos)
{
    SSD1306_enablePageAddressing();
    SSD1306_setPage(0);
    SSD1306_setStartColumn(pos);
    SSD1306_sendData(Graphics_HeaderLeftCapIcon, sizeof(Graphics_HeaderLeftCapIcon));
    pos = Text_draw(text, 0, pos + sizeof(Graphics_HeaderLeftCapIcon) + 2, 0, false);
    SSD1306_setStartColumn(pos + 1);
    SSD1306_sendData2(
        Graphics_HeaderLeftCapIcon,
        sizeof(Graphics_HeaderLeftCapIcon),
        SSD1306_SEND_FLIPX
        | SSD1306_SEND_FLIPY
        | SSD1306_SEND_BITSHIFT(1)
        | SSD1306_SEND_BITSHIFT_REVERSE
    );
}

void Graphics_drawKeypadHelpBarSeparators()
{
    SSD1306_enablePageAddressing();
    SSD1306_setPage(7);
    SSD1306_setStartColumn(32 - sizeof(Graphics_KeypadHelpBarSeparator) / 2);
    SSD1306_sendData(Graphics_KeypadHelpBarSeparator, sizeof(Graphics_KeypadHelpBarSeparator));
    SSD1306_setStartColumn(96 - sizeof(Graphics_KeypadHelpBarSeparator) / 2);
    SSD1306_sendData(Graphics_KeypadHelpBarSeparator, sizeof(Graphics_KeypadHelpBarSeparator));
}

void Graphics_drawVerticalLine(const uint8_t x, const uint8_t line)
{
    static const uint8_t Pattern = 0xFF;

    SSD1306_setPage(line);
    SSD1306_setStartColumn(x);
    SSD1306_sendData(&Pattern, sizeof(Pattern));
}