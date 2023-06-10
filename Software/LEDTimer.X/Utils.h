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

#pragma once

#include <stdbool.h>
#include <stdint.h>

extern const char* Date_DayShortNames[7];
extern const char* Date_MonthShortNames[12];

typedef uint8_t YearsFrom2023;

typedef struct {
    // Byte 0
    uint8_t startOrdinal : 2;       // 00: 1st, 01: 2nd, 11: last
    uint8_t endOrdinal : 2;
    uint8_t startShiftHours : 2;    // 0..3
    uint8_t endShiftHours : 2;
    // Byte 1
    uint8_t startMonth : 4;         // 0..11
    uint8_t endMonth : 4;           // 0..11
    // Byte 2
    uint8_t startDayOfWeek : 3;     // 0..6
    uint8_t endDayOfWeek : 3;
    uint8_t reserved : 2;
    // Byte 3
    uint8_t startHour : 4;
    uint8_t endHour : 4;
} Date_DstData;

bool Date_isLeapYear(YearsFrom2023 year);

uint8_t Date_lastDayOfMonth(uint8_t month, bool leapYear);

bool Date_dayOfMonth(
    uint8_t targetDayOfWeek,
    uint8_t currentDayOfWeek,
    int8_t ordinal,
    uint8_t daysInMonth,
    uint8_t currentDate
);

bool Date_isDst(
    Date_DstData data,
    uint8_t month,
    uint8_t daysInMonth,
    uint8_t date,
    uint8_t dayOfWeek,
    uint8_t hour
);