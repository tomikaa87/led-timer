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
    Created on 2023-04-06
*/

#include "Utils.h"

bool Date_isLeapYear(const YearsFrom2023 year)
{
    uint16_t y = year + 2023;
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

uint8_t Date_lastDayOfMonth(const uint8_t month, const bool leapYear)
{
    if (month == 2) {
        if (leapYear) {
            return 29;
        }

        return 28;
    }

    if (month == 4 || month == 6 || month == 9 || month == 11) {
        return 30;
    }

    return 31;
}