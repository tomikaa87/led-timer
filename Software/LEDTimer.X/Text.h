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

void Text_draw7Seg(
    const char* number,
    uint8_t line,
    uint8_t x,
    bool invert
);