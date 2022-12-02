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

#include "Text.h"
#include "SSD1306.h"
#include "Graphics.h"

#include <ctype.h>
#include <string.h>

#define ASCII_REDUCED_CHARSET_CHAR_COUNT 58
#define ASCII_REDUCED_CHARSET_SPACE_WIDTH 5
#define ASCII_REDUCED_CHARSET_WIDTH 5
static const uint8_t asciiReducedCharset[ASCII_REDUCED_CHARSET_CHAR_COUNT][ASCII_REDUCED_CHARSET_WIDTH] = {
    // !
    {
        0b00000000,
        0b00000000,
        0b01001111,
        0b00000000,
        0b00000000
    },
    // "
    {
        0b00000000,
        0b00000111,
        0b00000000,
        0b00000111,
        0b00000000
    },
    // #
    {
        0b00010100,
        0b01111111,
        0b00010100,
        0b01111111,
        0b00010100
    },
    // $
    {
        0b00100100,
        0b00101010,
        0b01111111,
        0b00101010,
        0b00010010
    },
    // %
    {
        0b00100011,
        0b00010011,
        0b00001000,
        0b01100100,
        0b01100010
    },
    // &
    {
        0b00110110,
        0b01001001,
        0b01010101,
        0b00100010,
        0b01010000
    },
    // '
    {
        0b00000000,
        0b00000101,
        0b00000011,
        0b00000000,
        0b00000000
    },
    // (
    {
        0b00000000,
        0b00011100,
        0b00100010,
        0b01000001,
        0b00000000
    },
    // )
    {
        0b00000000,
        0b01000001,
        0b00100010,
        0b00011100,
        0b00000000
    },
    // *
    {
        0b00010100,
        0b00001000,
        0b00111110,
        0b00001000,
        0b00010100
    },
    // +
    {
        0b00001000,
        0b00001000,
        0b00111110,
        0b00001000,
        0b00001000,
    },
    // ,
    {
        0b00000000,
        0b01010000,
        0b00110000,
        0b00000000,
        0b00000000
    },
    // -
    {
        0b00001000,
        0b00001000,
        0b00001000,
        0b00001000,
        0b00001000
    },
    // .
    {
        0b00000000,
        0b01100000,
        0b01100000,
        0b00000000,
        0b00000000
    },
    // /
    {
        0b00100000,
        0b00010000,
        0b00001000,
        0b00000100,
        0b00000010
    },
    // 0
    {
        0b00111110,
        0b01010001,
        0b01001001,
        0b01000101,
        0b00111110
    },
    // 1
    {
        0b00000000,
        0b01000010,
        0b01111111,
        0b01000000,
        0b00000000
    },
    // 2
    {
        0b01000010,
        0b01100001,
        0b01010001,
        0b01001001,
        0b01000110
    },
    // 3
    {
        0b00100001,
        0b01000001,
        0b01000101,
        0b01001011,
        0b00110001
    },
    // 4
    {
        0b00011000,
        0b00010100,
        0b00010010,
        0b01111111,
        0b00010000
    },
    // 5
    {
        0b00100111,
        0b01000101,
        0b01000101,
        0b01000101,
        0b00111001
    },
    // 6
    {
        0b00111100,
        0b01001010,
        0b01001001,
        0b01001001,
        0b00110000
    },
    // 7
    {
        0b00000001,
        0b01110001,
        0b00001001,
        0b00000101,
        0b00000011
    },
    // 8
    {
        0b00110110,
        0b01001001,
        0b01001001,
        0b01001001,
        0b00110110
    },
    // 9
    {
        0b00000110,
        0b01001001,
        0b01001001,
        0b00101001,
        0b00011110
    },
    // :
    {
        0b00000000,
        0b00110110,
        0b00110110,
        0b00000000,
        0b00000000
    },
    // ;
    {
        0b00000000,
        0b01010110,
        0b00110110,
        0b00000000,
        0b00000000
    },
    // <
    {
        0b00001000,
        0b00010100,
        0b00100010,
        0b01000001,
        0b00000000
    },
    // =
    {
        0b00010100,
        0b00010100,
        0b00010100,
        0b00010100,
        0b00010100
    },
    // >
    {
        0b00000000,
        0b01000001,
        0b00100010,
        0b00010100,
        0b00001000
    },
    // ?
    {
        0b00000010,
        0b00000001,
        0b01010001,
        0b00001001,
        0b00000110
    },
    // @
    {
        0b00110010,
        0b01001001,
        0b01111001,
        0b01000001,
        0b00111110
    },
    // A
    {
        0b01111110,
        0b00010001,
        0b00010001,
        0b00010001,
        0b01111110
    },
    // B
    {
        0b01111111,
        0b01001001,
        0b01001001,
        0b01001001,
        0b00110110
    },
    // C
    {
        0b00111110,
        0b01000001,
        0b01000001,
        0b01000001,
        0b00100010
    },
    // D
    {
        0b01111111,
        0b01000001,
        0b01000001,
        0b00100010,
        0b00011100
    },
    // E
    {
        0b01111111,
        0b01001001,
        0b01001001,
        0b01001001,
        0b01000001
    },
    // F
    {
        0b01111111,
        0b00001001,
        0b00001001,
        0b00001001,
        0b00000001
    },
    // G
    {
        0b00111110,
        0b01000001,
        0b01001001,
        0b01001001,
        0b01111010
    },
    // H
    {
        0b01111111,
        0b00001000,
        0b00001000,
        0b00001000,
        0b01111111
    },
    // I
    {
        0b00000000,
        0b01000001,
        0b01111111,
        0b01000001,
        0b00000000
    },
    // J
    {
        0b00100000,
        0b01000000,
        0b01000001,
        0b00111111,
        0b00000001
    },
    // K
    {
        0b01111111,
        0b00001000,
        0b00010100,
        0b00100010,
        0b01000001
    },
    // L
    {
        0b01111111,
        0b01000000,
        0b01000000,
        0b01000000,
        0b01000000
    },
    // M
    {
        0b01111111,
        0b00000010,
        0b00001100,
        0b00000010,
        0b01111111
    },
    // N
    {
        0b01111111,
        0b00000100,
        0b00001000,
        0b00010000,
        0b01111111
    },
    // O
    {
        0b00111110,
        0b01000001,
        0b01000001,
        0b01000001,
        0b00111110
    },
    // P
    {
        0b01111111,
        0b00001001,
        0b00001001,
        0b00001001,
        0b00000110
    },
    // Q
    {
        0b00111110,
        0b01000001,
        0b01010001,
        0b00100001,
        0b01011110
    },
    // R
    {
        0b01111111,
        0b00001001,
        0b00011001,
        0b00101001,
        0b01000110
    },
    // S
    {
        0b01000110,
        0b01001001,
        0b01001001,
        0b01001001,
        0b00110001
    },
    // T
    {
        0b00000001,
        0b00000001,
        0b01111111,
        0b00000001,
        0b00000001
    },
    // U
    {
        0b00111111,
        0b01000000,
        0b01000000,
        0b01000000,
        0b00111111
    },
    // V
    {
        0b00011111,
        0b00100000,
        0b01000000,
        0b00100000,
        0b00011111
    },
    // W
    {
        0b00111111,
        0b01000000,
        0b00111000,
        0b01000000,
        0b00111111
    },
    // X
    {
        0b01100011,
        0b00010100,
        0b00001000,
        0b00010100,
        0b01100011
    },
    // Y
    {
        0b00000111,
        0b00001000,
        0b01110000,
        0b00001000,
        0b00000111
    },
    // Z
    {
        0b01100001,
        0b01010001,
        0b01001001,
        0b01000101,
        0b01000011
    },
};

static const uint8_t asciiReducedPlaceholder[ASCII_REDUCED_CHARSET_WIDTH] = {
    0b01111111,
    0b01010101,
    0b01001001,
    0b01010101,
    0b01111111
};

void Text_draw(
    const char* const s,
    const uint8_t line,
    uint8_t x,
    const uint8_t yOffset
)
{
    uint8_t length = (uint8_t)strlen(s);

    if (length == 0 || line > 7 || x > 127 || yOffset > 7) {
        return;
    }

    SSD1306_enablePageAddressing();
    SSD1306_setPage(line);

    for (uint8_t i = 0; i < length; ++i) {
        SSD1306_setStartColumn(x);
        x += ASCII_REDUCED_CHARSET_WIDTH + 1;

        // Stop if the next character won't fit
        if (x > SSD1306_LCDWIDTH - 1) {
            return;
        }

        char c = (char)toupper(s[i]);
        
        if (c == ' ') {
            for (uint8_t i = 0; i < ASCII_REDUCED_CHARSET_SPACE_WIDTH; ++i) {
                const uint8_t data = 0;
                SSD1306_sendData(&data, 1, yOffset);
            }
            
            continue;
        }
        
        const uint8_t* charData;

        // If character is not supported, draw placeholder
        // Since space is dynamically generated, the first character is '!'
        if ((c - '!') >= ASCII_REDUCED_CHARSET_CHAR_COUNT) {
            charData = asciiReducedPlaceholder;
        } else {
            // Get data for the next character
            charData = asciiReducedCharset[c - '!'];
        }

        // Send data to the display
        SSD1306_sendData(charData, ASCII_REDUCED_CHARSET_WIDTH, yOffset);
    }
}
