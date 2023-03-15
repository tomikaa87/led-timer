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

#include "SSD1306.h"

enum
{
    SSD1306_I2C_ADDRESS = 0x3Cu,
    SSD1306_I2C_DC_FLAG = 0x40u,
    SSD1306_I2C_CO_FLAG = 0x00u
};

//#define SSD1306_DEBUG
//#define SSD1306_VERBOSE_ERRORS

#define SSD1309 1

#include <xc.h>
#include <stdio.h>

#define SSD1306_SCREEN_BUFFER_SIZE (SSD1306_LCDHEIGHT * SSD1306_LCDWIDTH / 8)

static bool SSD1306_displayOn = false;

static inline void i2cWait()
{
    while (
        SSP1STATbits.R_nW
        || (SSP1CON2 & 0b11111) // ACKEN | RCEN | PEN | RSEN | SEN
    );
}

static void i2cStart(const uint8_t command)
{
    // Start
	i2cWait();
	SSP1CON2bits.SEN = 1;

	// Send address
	i2cWait();
	SSP1BUF = SSD1306_I2C_ADDRESS << 1;

    // Send command / DC flag
    i2cWait();
    SSP1BUF = command;
}

static void i2cStop()
{
    // Stop
	i2cWait();
	SSP1CON2bits.PEN = 1;
}

void SSD1306_init()
{
    SSD1306_sendCommand(SSD1306_CMD_DISPLAYOFF);

    SSD1306_sendCommand(SSD1306_CMD_SETMULTIPLEX);
    SSD1306_sendCommand(SSD1306_LCDHEIGHT - 1);

#if SSD1309
    SSD1306_setPreChargePeriod(4, 10);
#else
    SSD1306_sendCommand(SSD1306_CMD_CHARGEPUMP);
    SSD1306_sendCommand(0x14);

    SSD1306_sendCommand(SSD1306_CMD_SETVCOMDESELECT);
    SSD1306_sendCommand(0x10);
#endif

    SSD1306_sendCommand(SSD1306_CMD_SEGREMAP | 0x1);
    SSD1306_sendCommand(SSD1306_CMD_COMSCANDEC);

    SSD1306_sendCommand(SSD1306_CMD_SETCOMPINS);
    SSD1306_sendCommand(0x12);

    SSD1306_setContrastLevel(SSD1306_CONTRAST_NORMAL);

    SSD1306_sendCommand(SSD1306_CMD_DISPLAYALLON_RESUME);
    SSD1306_sendCommand(SSD1306_CMD_NORMALDISPLAY);
    SSD1306_sendCommand(SSD1306_CMD_DEACTIVATE_SCROLL);
    SSD1306_sendCommand(SSD1306_CMD_DISPLAYON);

    SSD1306_clear();

#if defined SSD1306_DEBUG
    printf("SSD1306_init: filling buffer with test data\r\n");
    for (uint16_t i = 0; i < sizeof(SSD1306_screen_buffer); ++i)
        SSD1306_screen_buffer[i] = 0xAA;
#endif

    SSD1306_displayOn = true;
}

void SSD1306_setInvertEnabled(const bool enabled)
{
    SSD1306_sendCommand(
        enabled
            ? SSD1306_CMD_INVERTDISPLAY
            : SSD1306_CMD_NORMALDISPLAY
    );
}

void SSD1306_scroll(
    const SSD1306_ScrollMode scroll,
    const uint8_t start,
    const uint8_t stop
)
{
    switch (scroll) {
        case SSD1306_SCROLL_STOP:
            SSD1306_sendCommand(SSD1306_CMD_DEACTIVATE_SCROLL);
            return;

        case SSD1306_SCROLL_LEFT:
            SSD1306_sendCommand(SSD1306_CMD_LEFT_HORIZONTAL_SCROLL);
            SSD1306_sendCommand(0);
            SSD1306_sendCommand(start);
            SSD1306_sendCommand(0);
            SSD1306_sendCommand(stop);
            SSD1306_sendCommand(0);
            SSD1306_sendCommand(0xFF);
            SSD1306_sendCommand(SSD1306_CMD_ACTIVATE_SCROLL);
            break;

        case SSD1306_SCROLL_RIGHT:
            SSD1306_sendCommand(SSD1306_CMD_RIGHT_HORIZONTAL_SCROLL);
            SSD1306_sendCommand(0);
            SSD1306_sendCommand(start);
            SSD1306_sendCommand(0);
            SSD1306_sendCommand(stop);
            SSD1306_sendCommand(0);
            SSD1306_sendCommand(0xFF);
            SSD1306_sendCommand(SSD1306_CMD_ACTIVATE_SCROLL);
            break;

        case SSD1306_SCROLL_DIAG_LEFT:
            SSD1306_sendCommand(SSD1306_CMD_SET_VERTICAL_SCROLL_AREA);
            SSD1306_sendCommand(0);
            SSD1306_sendCommand(SSD1306_LCDHEIGHT);
            SSD1306_sendCommand(SSD1306_CMD_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL);
            SSD1306_sendCommand(0);
            SSD1306_sendCommand(start);
            SSD1306_sendCommand(0);
            SSD1306_sendCommand(stop);
            SSD1306_sendCommand(1);
            SSD1306_sendCommand(SSD1306_CMD_ACTIVATE_SCROLL);
            break;

        case SSD1306_SCROLL_DIAG_RIGHT:
            SSD1306_sendCommand(SSD1306_CMD_SET_VERTICAL_SCROLL_AREA);
            SSD1306_sendCommand(0);
            SSD1306_sendCommand(SSD1306_LCDHEIGHT);
            SSD1306_sendCommand(SSD1306_CMD_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL);
            SSD1306_sendCommand(0);
            SSD1306_sendCommand(start);
            SSD1306_sendCommand(0);
            SSD1306_sendCommand(stop);
            SSD1306_sendCommand(1);
            SSD1306_sendCommand(SSD1306_CMD_ACTIVATE_SCROLL);
            break;
    }
}

void SSD1306_setContrast(const uint8_t contrast)
{
    SSD1306_sendCommand(SSD1306_CMD_SETCONTRAST);
    SSD1306_sendCommand(contrast);
}

void SSD1306_setContrastLevel(const SSD1306_ContrastLevel level)
{
    uint8_t contrast = 0;

    switch (level) {
        case SSD1306_CONTRAST_HIGH:
            contrast = 0xCF;
            break;

        case SSD1306_CONTRAST_NORMAL:
            contrast = 0x60;
            break;

        default:
        case SSD1306_CONTRAST_LOW:
            contrast = 0x20;
            break;

        case SSD1306_CONTRAST_LOWEST:
            contrast = 0;
            break;
    }

    SSD1306_setContrast(contrast);
}

void SSD1306_clear()
{
    SSD1306_sendCommand(SSD1306_CMD_MEMORYMODE);
    SSD1306_sendCommand(SSD1306_MEM_MODE_HORIZONTAL_ADDRESSING);

    SSD1306_sendCommand(SSD1306_CMD_COLUMNADDR);
    SSD1306_sendCommand(0); // Column start address (0 = reset)
    SSD1306_sendCommand(SSD1306_LCDWIDTH - 1); // Column end address (127 = reset)

    SSD1306_sendCommand(SSD1306_CMD_PAGEADDR);
    SSD1306_sendCommand(0); // Page start address (0 = reset)

    // Page end address
#ifdef SSD1306_128_64
    SSD1306_sendCommand(7);
#elif defined SSD1306_128_32
    SSD1306_sendCommand(3);
#elif defined SSD1306_96_16
    SSD1306_sendCommand(1);
#endif

    i2cStart(SSD1306_I2C_DC_FLAG);

    for (uint16_t i = SSD1306_SCREEN_BUFFER_SIZE; i > 0; --i) {
        i2cWait();
        SSP1BUF = 0;
    }

    i2cStop();

#ifdef SSD1306_DEBUG
    printf("SSD1306_update: finished\r\n");
#endif
}

void SSD1306_sendCommand(const SSD1306_Command cmd)
{
    i2cStart(SSD1306_I2C_CO_FLAG);

    i2cWait();
    SSP1BUF = cmd;

    i2cStop();

#ifdef SSD1306_DEBUG
    printf("SSD1306_sendCommand: %02x. I2C status: %d\r\n", cmd, status);
#endif
}

static void sendCommand2(
    const SSD1306_Command cmd,
    const uint8_t arg1,
    const uint8_t arg2
)
{
    i2cStart(SSD1306_I2C_CO_FLAG);

    i2cWait();
    SSP1BUF = cmd;

    i2cWait();
    SSP1BUF = arg1;

    i2cWait();
    SSP1BUF = arg2;

    i2cStop();
}

void SSD1306_setColumnAddress(const uint8_t start, const uint8_t end)
{
    sendCommand2(SSD1306_CMD_COLUMNADDR, start & 0x7F, end & 0x7F);
}

void SSD1306_setPageAddress(const uint8_t start, const uint8_t end)
{
    sendCommand2(SSD1306_CMD_PAGEADDR, start & 0b111, end & 0b111);
}

void SSD1306_setPage(const uint8_t page)
{
    SSD1306_sendCommand(SSD1306_CMD_PAGESTARTADDR | (page & 0b111));
}

void SSD1306_setStartColumn(const uint8_t address)
{
    // Set lower nibble
    SSD1306_sendCommand(SSD1306_CMD_SETLOWCOLUMN | (address & 0xF));
    // Set upper nibble
    SSD1306_sendCommand(SSD1306_CMD_SETHIGHCOLUMN | ((address >> 4) & 0x7));
}

void SSD1306_sendData(
    const uint8_t* data,
    const uint8_t length
)
{
    SSD1306_sendData2(data, length, 0);
}

void SSD1306_sendData2(
    const uint8_t* const data,
    uint8_t length,
    const uint8_t flags
)
{
    uint8_t bitShift = (flags >> 3) & 0b111;
    uint8_t dataIndex = (flags & SSD1306_SEND_FLIPY) ? length - 1 : 0;

    i2cStart(SSD1306_I2C_DC_FLAG);

    while (length-- > 0) {
        uint8_t byte = data[dataIndex];

        if (flags & SSD1306_SEND_FLIPY) {
            --dataIndex;
        } else {
            ++dataIndex;
        }

        if (flags & SSD1306_SEND_FLIPX) {
            uint8_t reverse = 0;
            for (uint8_t i = 7; i > 0; --i) {
                reverse |= byte & 1;
                reverse <<= 1;
                byte >>= 1;
            }
            byte = reverse;
        }

        if (bitShift) {
            byte <<= bitShift;
        }

        if (flags & SSD1306_SEND_INVERT) {
            byte = ~byte;
        }

        i2cWait();
        SSP1BUF = byte;
    }

    i2cStop();
}

void SSD1306_enablePageAddressing()
{
    SSD1306_sendCommand(SSD1306_CMD_MEMORYMODE);
    SSD1306_sendCommand(SSD1306_MEM_MODE_PAGE_ADDRESSING);
}

void SSD1306_fillArea(
    const uint8_t x,
    const uint8_t startPage,
    const uint8_t width,
    const uint8_t pages,
    const uint8_t color
)
{
    SSD1306_fillAreaPattern(x, startPage, width, pages, color > 0 ? 0xFF : 0);
}

void SSD1306_fillAreaPattern(
    uint8_t x,
    const uint8_t startPage,
    const uint8_t width,
    const uint8_t pages,
    const uint8_t pattern
) {
    SSD1306_enablePageAddressing();

    for (uint8_t i = pages, page = startPage; i > 0; --i, ++page) {
        if (page >= 8) {
            return;
        }

        SSD1306_setPage(page);
        SSD1306_setStartColumn(x);

        i2cStart(SSD1306_I2C_DC_FLAG);

        for (uint8_t j = width; j > 0 && x + j < SSD1306_LCDWIDTH; --j) {
            i2cWait();
            SSP1BUF = pattern;
        }

        i2cStop();
    }
}

void SSD1306_setDisplayEnabled(const bool enabled)
{
    if (enabled) {
        SSD1306_sendCommand(SSD1306_CMD_DISPLAYON);
        SSD1306_displayOn = true;
    } else {
        SSD1306_sendCommand(SSD1306_CMD_DISPLAYOFF);
        SSD1306_displayOn = false;
    }
}

bool SSD1306_isDisplayEnabled()
{
    return SSD1306_displayOn;
}

void SSD1306_setPreChargePeriod(const uint8_t phase1, const uint8_t phase2)
{
    SSD1306_sendCommand(0xD9);
    SSD1306_sendCommand(((phase2 & 0b1111u) << 4) | (phase1 & 0b1111u));
}