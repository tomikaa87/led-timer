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

/**
 * Draws a text with the default font.
 * @param s Input string
 * @param line Position of the text from the top, measured in pages (8 pixels)
 * @param x Position of the text from the left
 * @param yOffset If > 0, offsets the text in the current line
 * @param invert Set to True to draw the text inverted
 * @return Position of the end of the text from the left
 */
uint8_t Text_draw(
    const char* s,
    uint8_t line,
    uint8_t x,
    uint8_t yOffset,
    bool invert
);

/**
 * Draws a text with large 7-Segment LCD font
 * @param number String with the following characters allowed: 0-9, -, <space>, :
 * @param line Position of the text from the top, measured in pages (8 pixels)
 * @param x Position of the text from the left
 * @param invert Set to True to draw the text inverted
 */
uint8_t Text_draw7Seg(
    const char* number,
    uint8_t line,
    uint8_t x,
    bool invert
);

uint8_t Text_calculateWidth(const char* s);

uint8_t Text_calculateWidth7Seg(const char* s);

#define CalculateTextWidth(_Text) ( \
    (sizeof((_Text)) - 1) * 5 \
    + ((sizeof((_Text)) >= 2) ? (sizeof((_Text)) - 2) : 0) * 1 \
)

#define LeftText(_Text, _Line) \
    Text_draw((_Text), (_Line), 0, 0, false)

#define CenterText(_Text, _Line) \
    Text_draw((_Text), (_Line), 64 - CalculateTextWidth((_Text)) / 2, 0, false)

#define RightText(_Text, _Line) \
    Text_draw((_Text), (_Line), 127 - CalculateTextWidth((_Text)), 0, false)

#define LeftHelpText(_Text) LeftText((_Text), 0)

#define CenterHelpText(_Text) CenterText((_Text), 0)

#define RightHelpText(_Text) RightText((_Text), 0)