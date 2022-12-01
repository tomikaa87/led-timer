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
    SSD1306_I2C_CO_FLAG = 0x80u
};

//#define SSD1306_DEBUG
//#define SSD1306_VERBOSE_ERRORS

#include <xc.h>
#include <stdio.h>

#define SSD1306_SCREEN_BUFFER_SIZE (SSD1306_LCDHEIGHT * SSD1306_LCDWIDTH / 8)

static bool SSD1306_displayOn = false;

static inline void i2cWait()
{
    while (
        SSP1STATbits.R_nW
        || SSP1CON2bits.SEN
        || SSP1CON2bits.RSEN
        || SSP1CON2bits.PEN
        || SSP1CON2bits.RCEN
        || SSP1CON2bits.ACKEN
    );
}

void i2cTransmit(const uint8_t* data, uint8_t length)
{
	// Start
	i2cWait();
	SSP1CON2bits.SEN = 1;
	
	// Send address
	i2cWait();
	SSP1BUF = SSD1306_I2C_ADDRESS << 1;
	
	// Send data
	while (length-- > 0) {
		i2cWait();
		SSP1BUF = *data++;
	}
	
	// Stop
	i2cWait();
	SSP1CON2bits.PEN = 1;
}

void SSD1306_init()
{
    SSD1306_sendCommand(SSD1306_CMD_DISPLAYOFF);

    SSD1306_sendCommand(SSD1306_CMD_SETDISPLAYCLOCKDIV);
    SSD1306_sendCommand(0x80);

    SSD1306_sendCommand(SSD1306_CMD_SETMULTIPLEX);
    SSD1306_sendCommand(SSD1306_LCDHEIGHT - 1);

    SSD1306_sendCommand(SSD1306_CMD_SETDISPLAYOFFSET);
    SSD1306_sendCommand(0x0);

    SSD1306_sendCommand(SSD1306_CMD_SETSTARTLINE | 0x0);

    SSD1306_sendCommand(SSD1306_CMD_CHARGEPUMP);
    SSD1306_sendCommand(0x14);

    SSD1306_sendCommand(SSD1306_CMD_MEMORYMODE);
    SSD1306_sendCommand(SSD1306_MEM_MODE_HORIZONTAL_ADDRESSING);

    SSD1306_sendCommand(SSD1306_CMD_SEGREMAP | 0x1);
    SSD1306_sendCommand(SSD1306_CMD_COMSCANDEC);

    SSD1306_sendCommand(SSD1306_CMD_SETCOMPINS);
    SSD1306_sendCommand(0x12);

    SSD1306_setContrastLevel(SSD1306_CONTRAST_NORMAL);

    SSD1306_sendCommand(SSD1306_CMD_SETVCOMDETECT);
    SSD1306_sendCommand(0x10);

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
        default:
        case SSD1306_CONTRAST_HIGH:
            contrast = 0xCF;
            break;

        case SSD1306_CONTRAST_NORMAL:
            contrast = 0x60;
            break;

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

    for (uint16_t i = 0; i < SSD1306_SCREEN_BUFFER_SIZE; i += 16) {
#ifdef SSD1306_DEBUG
        printf("SSD1306_clear: sending data chunk. i = %u, i, buffer);
#endif

        uint8_t data[17];
        data[0] = SSD1306_I2C_DC_FLAG;

        for (uint8_t j = 1; j < 17; ++j)
            data[j] = 0;
        
        i2cTransmit(data, sizeof(data));

#ifdef SSD1306_DEBUG
        printf("SSD1306_update: data chunk written\r\n");
#endif
    }

#ifdef SSD1306_DEBUG
    printf("SSD1306_update: finished\r\n");
#endif
}

void SSD1306_sendCommand(const SSD1306_Command cmd)
{
    uint8_t data[2] = {0};
    data[1] = (uint8_t) cmd;

    i2cTransmit(data, sizeof(data));

#ifdef SSD1306_DEBUG
    printf("SSD1306_sendCommand: %02x. I2C status: %d\r\n", cmd, status);
#endif
}

void SSD1306_setColumnAddress(const uint8_t start, const uint8_t end)
{
    if (start > 127 || end > 127) {
#ifdef SSD1306_VERBOSE_ERRORS
        printf("SSD1306_setColumnAddress: invalid range: %u-%u\r\n", start, end);
#endif
        return;
    }

    uint8_t data[3];
    data[0] = SSD1306_CMD_COLUMNADDR;
    data[1] = start;
    data[2] = end;

    i2cTransmit(data, sizeof(data));
}

void SSD1306_setPageAddress(const uint8_t start, const uint8_t end)
{
    if (start > 7 || end > 7) {
#ifdef SSD1306_VERBOSE_ERRORS
        printf("SSD1306_setPageAddress: invalid range: %u-%u\r\n", start, end);
#endif
        return;
    }

    uint8_t data[3];
    data[0] = SSD1306_CMD_PAGEADDR;
    data[1] = start;
    data[2] = end;

    i2cTransmit(data, sizeof(data));
}

void SSD1306_setPage(uint8_t page)
{
    if (page > 7) {
#ifdef SSD1306_VERBOSE_ERRORS
        printf("SSD1306_setPage: invalid page: %u\r\n", page);
#endif
        return;
    }

    SSD1306_sendCommand(SSD1306_CMD_PAGESTARTADDR | page);
}

void SSD1306_setStartColumn(const uint8_t address)
{
    if (address > 127) {
#ifdef SSD1306_VERBOSE_ERRORS
        printf("SSD1306_setStartColumn: invalid address: %u\r\n", address);
#endif
        return;
    }

    // Set lower nibble
    SSD1306_sendCommand(SSD1306_CMD_SETLOWCOLUMN | (address & 0xF));
    // Set upper nibble
    SSD1306_sendCommand(SSD1306_CMD_SETHIGHCOLUMN | ((address >> 4) & 0xF));
}

void SSD1306_sendData(
    const uint8_t* const data,
    const uint8_t length,
    const uint8_t bitShift
)
{
    if (bitShift > 7)
        return;
    
    uint8_t buffer[17];
    uint8_t bytesRemaining = length;
    uint8_t dataIndex = 0;
    static const uint8_t chunk_size = 16;

    while (bytesRemaining > 0) {
        uint8_t count = bytesRemaining >= chunk_size ? chunk_size : bytesRemaining;
        bytesRemaining -= count;

        buffer[0] = SSD1306_I2C_DC_FLAG;
        for (uint8_t i = 1; i <= count; ++i) {
            buffer[i] = (uint8_t)(data[dataIndex++] << bitShift);
        }
        
        i2cTransmit(buffer, count + 1);
    }
}

void SSD1306_setPageAddressing()
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
    if (width == 0 || x >= SSD1306_LCDWIDTH) {
        return;
    }
    
    SSD1306_setPageAddressing();
    
    uint8_t data[2];
    data[0] = SSD1306_I2C_DC_FLAG;
    data[1] = color > 0 ? 0xFF : 0;
    
    for (uint8_t i = 0; i < pages; ++i) {
        uint8_t page = startPage + i;
        if (page >= 8) {
            return;
        }
        
        SSD1306_setPage(page);
        SSD1306_setStartColumn(x);
        
        for (uint8_t j = 0; j < width && x + j < SSD1306_LCDWIDTH; ++j) {
            i2cTransmit(data, sizeof(data));
        }
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