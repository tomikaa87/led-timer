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

#include <stdbool.h>
#include <stdint.h>

#if !defined SSD1306_128_64 && !defined SSD1306_128_32 && !defined SSD1306_96_16
#define SSD1306_128_64
#endif

enum
{
#if defined SSD1306_128_64
    SSD1306_LCDWIDTH = 128u,
    SSD1306_LCDHEIGHT = 64u,
    SSD1306_PAGE_COUNT = 8u,
#elif defined SSD1306_128_32
    SSD1306_LCDWIDTH = 128u,
    SSD1306_LCDHEIGHT = 32u,
    SSD1306_PAGE_COUNT = 4u,
#elif defined SSD1306_96_16
    SSD1306_LCDWIDTH = 96u,
    SSD1306_LCDHEIGHT = 16u,
    SSD1306_PAGE_COUNT = 2u,
#endif
};

typedef enum
{
    SSD1306_CMD_SETCONTRAST = 0x81u,
    SSD1306_CMD_DISPLAYALLON_RESUME = 0xA4u,
    SSD1306_CMD_DISPLAYALLON = 0xA5u,
    SSD1306_CMD_NORMALDISPLAY = 0xA6u,
    SSD1306_CMD_INVERTDISPLAY = 0xA7u,
    SSD1306_CMD_DISPLAYOFF = 0xAEu,
    SSD1306_CMD_DISPLAYON = 0xAFu,
    SSD1306_CMD_SETDISPLAYOFFSET = 0xD3u,
    SSD1306_CMD_SETCOMPINS = 0xDAu,
    SSD1306_CMD_SETVCOMDETECT = 0xDBu,
    SSD1306_CMD_SETDISPLAYCLOCKDIV = 0xD5u,
    SSD1306_CMD_SETPRECHARGE = 0xD9u,
    SSD1306_CMD_SETMULTIPLEX = 0xA8u,
    SSD1306_CMD_SETLOWCOLUMN = 0x00u,
    SSD1306_CMD_SETHIGHCOLUMN = 0x10u,
    SSD1306_CMD_SETSTARTLINE = 0x40u,
    SSD1306_CMD_MEMORYMODE = 0x20u,
    SSD1306_CMD_COLUMNADDR = 0x21u,
    SSD1306_CMD_PAGEADDR = 0x22u,
    SSD1306_CMD_COMSCANINC = 0xC0u,
    SSD1306_CMD_COMSCANDEC = 0xC8u,
    SSD1306_CMD_SEGREMAP = 0xA0u,
    SSD1306_CMD_CHARGEPUMP = 0x8Du,
    SSD1306_CMD_EXTERNALVCC = 0x01u,
    SSD1306_CMD_SWITCHCAPVCC = 0x02u,
    SSD1306_CMD_ACTIVATE_SCROLL = 0x2Fu,
    SSD1306_CMD_DEACTIVATE_SCROLL = 0x2Eu,
    SSD1306_CMD_SET_VERTICAL_SCROLL_AREA = 0xA3u,
    SSD1306_CMD_RIGHT_HORIZONTAL_SCROLL = 0x26u,
    SSD1306_CMD_LEFT_HORIZONTAL_SCROLL = 0x27u,
    SSD1306_CMD_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL = 0x29u,
    SSD1306_CMD_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL = 0x2Au,
    SSD1306_CMD_PAGESTARTADDR = 0xB0u,

    SSD1306_MEM_MODE_PAGE_ADDRESSING = 0b10,
    SSD1306_MEM_MODE_HORIZONTAL_ADDRESSING = 0,
    SSD1306_MEM_MODE_VERTICAL_ADDRESSING = 0b01
} SSD1306_Command;

typedef enum {
    SSD1306_SCROLL_STOP,
    SSD1306_SCROLL_LEFT,
    SSD1306_SCROLL_RIGHT,
    SSD1306_SCROLL_DIAG_LEFT,
    SSD1306_SCROLL_DIAG_RIGHT
} SSD1306_ScrollMode;

typedef enum {
    SSD1306_CONTRAST_HIGH,
    SSD1306_CONTRAST_NORMAL,
    SSD1306_CONTRAST_LOW,
    SSD1306_CONTRAST_LOWEST,
} SSD1306_ContrastLevel;

typedef enum {
    SSD1306_COLOR_BLACK,
    SSD1306_COLOR_WHITE
} SSD1306_Color;

// High level API
void SSD1306_init();
void SSD1306_setInvertEnabled(bool enabled);
void SSD1306_scroll(SSD1306_ScrollMode scroll, uint8_t start, uint8_t stop);
void SSD1306_setContrast(uint8_t contrast);
void SSD1306_setContrastLevel(SSD1306_ContrastLevel level);
void SSD1306_clear();

void SSD1306_fillArea(uint8_t x, uint8_t startPage, uint8_t width, uint8_t pages, uint8_t color);

// Low level API
void SSD1306_sendCommand(SSD1306_Command cmd);
void SSD1306_sendData(
    const uint8_t* data,
    uint8_t length,
    uint8_t bitShift,
    bool invert
);

void SSD1306_enablePageAddressing();

void SSD1306_setColumnAddress(uint8_t start, uint8_t end);
void SSD1306_setPageAddress(uint8_t start, uint8_t end);

void SSD1306_setPage(uint8_t page);
void SSD1306_setStartColumn(uint8_t address);

void SSD1306_setDisplayEnabled(bool enabled);
bool SSD1306_isDisplayEnabled();
