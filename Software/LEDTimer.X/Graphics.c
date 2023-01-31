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

const uint8_t Graphics_NextIcon[Graphics_NextIconWidth] = {
    0b00010000,
    0b00110000,
    0b01110000,
    0b11111111,
    0b01110000,
    0b00110000,
    0b00010000
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

const uint8_t Graphics_HeaderRightCapIcon[Graphics_HeaderRightCapIconWidth] = {
    0b01010101,
    0b00101010,
    0b00010101,
    0b00001010,
    0b00000101,
    0b00000010,
    0b00000001
};

void Graphics_drawBitmap(
    const uint8_t* const bitmap,
    const uint8_t width,
    const uint8_t x,
    const uint8_t page,
    const bool invert
)
{
    if (page > SSD1306_PAGE_COUNT || width == 0 || x + width >= SSD1306_LCDWIDTH) {
        return;
    }

    SSD1306_enablePageAddressing();
    SSD1306_setPage(page);
    SSD1306_setStartColumn(x);
    SSD1306_sendData(bitmap, width, 0, invert);
}

void Graphics_drawMultipageBitmap(
    const uint8_t* bitmap,
    const uint8_t width,
    const uint8_t pageCount,
    const uint8_t x,
    const uint8_t startPage,
    const bool invert
)
{
    if (startPage + pageCount > SSD1306_PAGE_COUNT) {
        return;
    }

    for (uint8_t page = startPage; page < startPage + pageCount; ++page) {
        Graphics_drawBitmap(bitmap, width, x, page, invert);
        bitmap += width;
    }
}

void Graphics_drawScheduleBar(
    const uint8_t line,
    const ScheduleSegmentData segmentData,
    const bool invert,
    const bool flip
)
{
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

	for (uint8_t x = 0; x < 121; ++x) {
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

		SSD1306_sendData(&bitmap, 1, 0, invert);

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

	Text_draw("0", textLine, 1, 1, invert);
	Text_draw("6", textLine, 31, 1, invert);
	Text_draw("12", textLine, 58, 1, invert);
	Text_draw("18", textLine, 88, 1, invert);
	Text_draw("24", textLine, 115, 1, invert);
}

void Graphics_drawScheduleSegmentIndicator(
    const uint8_t line,
    const uint8_t segmentIndex,
    const bool invert,
    const bool flip
)
{
	static const uint8_t SegmentIndicatorBitmap[] = {
		0b00010000,
		0b00100000,
		0b01111100,
		0b00100000,
		0b00010000
	};

    static const uint8_t SegmentIndicatorBitmapFlipped[] = {
		0b00001000,
		0b00000100,
		0b00111110,
		0b00000100,
		0b00001000
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
        flip ? SegmentIndicatorBitmapFlipped : SegmentIndicatorBitmap,
        sizeof(SegmentIndicatorBitmap),
        x,
        line,
        invert
    );
}