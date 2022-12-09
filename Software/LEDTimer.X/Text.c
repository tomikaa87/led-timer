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

#define ASCIIReduced_CharCount              58
#define ASCIIReduced_SpaceWidth             5
#define ASCIIReduced_CharSpacing            1
#define ASCIIReduced_CharWidth              5
static const uint8_t ASCIIReduced[ASCIIReduced_CharCount][ASCIIReduced_CharWidth] = {
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

static const uint8_t ASCIIReducedPlaceholder[ASCIIReduced_CharWidth] = {
    0b01111111,
    0b01010101,
    0b01001001,
    0b01010101,
    0b01111111
};

#define Numbers7Seg_CharCount               10
#define Numbers7Seg_CharWidth               12
#define Numbers7Seg_Pages                   3
static const uint8_t Numbers7Seg[Numbers7Seg_CharCount][Numbers7Seg_Pages][Numbers7Seg_CharWidth] = {
    {
        // 0, Page 0
        {
            0b11111110,
            0b11111101,
            0b11111011,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b11111011,
            0b11111101,
            0b11111110
        },
        // 0, Page 1
        {
            0b11111111,
            0b11110111,
            0b11100011,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b11100011,
            0b11110111,
            0b11111111
        },
        // 0, Page 2
        {
            0b00111111,
            0b01011111,
            0b01101111,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01101111,
            0b01011111,
            0b00111111
        }
    },
    {
        // 1, Page 0
        {
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b11111000,
            0b11111100,
            0b11111110
        },
        // 1, Page 1
        {
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b11100011,
            0b11110111,
            0b11111111
        },
        // 1, Page 2
        {
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00001111,
            0b00011111,
            0b00111111
        }
    },
    {
        // 2, Page 0
        {
            0b00000000,
            0b00000001,
            0b00000011,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b11111011,
            0b11111101,
            0b11111110
        },
        // 2, Page 1
        {
            0b11111000,
            0b11110100,
            0b11101100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011011,
            0b00010111,
            0b00001111
        },
        // 2, Page 2
        {
            0b00111111,
            0b01011111,
            0b01101111,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01100000,
            0b01000000,
            0b00000000
        }
    },
    {
        // 3, Page 0
        {
            0b00000000,
            0b00000001,
            0b00000011,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b11111011,
            0b11111101,
            0b11111110
        },
        // 3, Page 1
        {
            0b00000000,
            0b00000000,
            0b00001000,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b11101011,
            0b11110111,
            0b11111111
        },
        // 3, Page 2
        {
            0b00000000,
            0b01000000,
            0b01100000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01101111,
            0b01011111,
            0b00111111
        }
    },
    {
        // 4, Page 0
        {
            0b11111110,
            0b11111100,
            0b11111000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b11111000,
            0b11111100,
            0b11111110
        },
        // 4, Page 1
        {
            0b00001111,
            0b00010111,
            0b00011011,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b11101011,
            0b11110111,
            0b11111111
        },
        // 4, Page 2
        {
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00001111,
            0b00011111,
            0b00111111
        }
    },
    {
        // 5, Page 0
        {
            0b11111110,
            0b11111101,
            0b11111011,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000011,
            0b00000001,
            0b00000000
        },
        // 5, Page 1
        {
            0b00001111,
            0b00010111,
            0b00011011,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b11101100,
            0b11110100,
            0b11111000
        },
        // 5, Page 2
        {
            0b00000000,
            0b01000000,
            0b01100000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01101111,
            0b01011111,
            0b00111111
        }
    },
    {
        // 6, Page 0
        {
            0b11111110,
            0b11111101,
            0b11111011,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000011,
            0b00000001,
            0b00000000
        },
        // 6, Page 1
        {
            0b11111111,
            0b11110111,
            0b11101011,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b11101100,
            0b11110100,
            0b11111000
        },
        // 6, Page 2
        {
            0b00111111,
            0b01011111,
            0b01101111,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01101111,
            0b01011111,
            0b00111111
        }
    },
    {
        // 7, Page 0
        {
            0b00000000,
            0b00000001,
            0b00000011,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b11111011,
            0b11111101,
            0b11111110
        },
        // 7, Page 1
        {
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b11100011,
            0b11110111,
            0b11111111
        },
        // 7, Page 2
        {
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00001111,
            0b00011111,
            0b00111111
        }
    },
    {
        // 8, Page 0
        {
            0b11111110,
            0b11111101,
            0b11111011,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b11111011,
            0b11111101,
            0b11111110
        },
        // 8, Page 1
        {
            0b11111111,
            0b11110111,
            0b11101011,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b11101011,
            0b11110111,
            0b11111111
        },
        // 8, Page 2
        {
            0b00111111,
            0b01011111,
            0b01101111,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01101111,
            0b01011111,
            0b00111111
        }
    },
    {
        // 9, Page 0
        {
            0b11111110,
            0b11111101,
            0b11111011,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b00000111,
            0b11111011,
            0b11111101,
            0b11111110
        },
        // 9, Page 1
        {
            0b00001111,
            0b00010111,
            0b00011011,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b00011100,
            0b11101011,
            0b11110111,
            0b11111111
        },
        // 9, Page 2
        {
            0b00000000,
            0b01000000,
            0b01100000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01110000,
            0b01101111,
            0b01011111,
            0b00111111
        }
    }
};

#define Colon7Seg_CharWidth                 4
#define Colon7Seg_Pages                     3
static const uint8_t Colon7Seg[Colon7Seg_Pages][Colon7Seg_CharWidth] = {
    {
        0b10000000,
        0b11000000,
        0b11000000,
        0b11000000
    },
    {
        0b10000011,
        0b11000011,
        0b11000011,
        0b11000001
    },
    {
        0b00000011,
        0b00000011,
        0b00000011,
        0b00000001
    }
};

uint8_t Text_draw(
    const char* const s,
    const uint8_t line,
    uint8_t x,
    const uint8_t yOffset,
    const bool invert
)
{
    uint8_t length = (uint8_t)strlen(s);

    if (length == 0 || line > 7 || x > 127 || yOffset > 7) {
        return x;
    }

    SSD1306_enablePageAddressing();
    SSD1306_setPage(line);

    for (uint8_t i = 0; i < length; ++i) {
        // Stop if the next character won't fit
        if (x > SSD1306_LCDWIDTH - 1) {
            return x;
        }

        SSD1306_setStartColumn(x);

        char c = (char)toupper(s[i]);

        if (c == ' ') {
            for (uint8_t i = 0; i < ASCIIReduced_SpaceWidth; ++i) {
                const uint8_t data = 0;
                SSD1306_sendData(&data, 1, yOffset, invert);
            }
        } else {
            const uint8_t* charData;

            // If character is not supported, draw placeholder
            // Since space is dynamically generated, the first character is '!'
            if ((c - '!') >= ASCIIReduced_CharCount) {
                charData = ASCIIReducedPlaceholder;
            } else {
                // Get data for the next character
                charData = ASCIIReduced[c - '!'];
            }

            // Send data to the display
            SSD1306_sendData(charData, ASCIIReduced_CharWidth, yOffset, invert);
        }

        x += ASCIIReduced_CharWidth;

        // Clear the pixels between the characters
        for (uint8_t j = 0; j < ASCIIReduced_CharSpacing; ++j) {
            static const uint8_t data = 0;
            SSD1306_sendData(&data, 1, yOffset, invert);
            ++x;
        }
    }

    return x;
}

void Text_draw7Seg(
    const char* const number,
    const uint8_t line,
    uint8_t x,
    const bool invert
) {
    uint8_t length = (uint8_t)strlen(number);

    if (length == 0 || line > (7 - Numbers7Seg_Pages) || x > 127) {
        return;
    }

    SSD1306_enablePageAddressing();

    for (uint8_t i = 0; i < length; ++i) {
        // Stop if the next character won't fit
        if (x + Numbers7Seg_CharWidth + 1 > SSD1306_LCDWIDTH - 1) {
            return;
        }

        char c = number[i];

        if (c != ' ') {
            if (c == '-') {
                SSD1306_setStartColumn(x);
                SSD1306_setPage(line + 1);

                // Cost-efficient dash symbol
                uint8_t charData = 0b00011100;
                for (uint8_t j = 2; j < Numbers7Seg_CharWidth - 2; ++j) {
                    SSD1306_sendData(&charData, 1, 0, invert);
                }
            } else {
                uint8_t width = Numbers7Seg_CharWidth;

                // Draw the pages of the character
                for (uint8_t page = 0; page < Numbers7Seg_Pages; ++page) {
                    SSD1306_setPage(line + page);
                    SSD1306_setStartColumn(x);

                    const uint8_t* charData = 0;

                    if (c >= '0' && c <= '9') {
                        charData = Numbers7Seg[c - '0'][page];
                    } else if (c == ':') {
                        width = Colon7Seg_CharWidth;
                        charData = Colon7Seg[page];
                    }

                    SSD1306_sendData(charData, width, 0, invert);
                }

                x += width + 2;
            }
        } else {
            for (uint8_t page = 0; page < Numbers7Seg_Pages; ++page) {
                SSD1306_setPage(line + page);
                SSD1306_setStartColumn(x);

                uint8_t charData[Numbers7Seg_CharWidth] = { 0 };
                SSD1306_sendData(charData, sizeof(charData), 0, invert);
            }

            x += Numbers7Seg_CharWidth + 2;
        }
    }
}