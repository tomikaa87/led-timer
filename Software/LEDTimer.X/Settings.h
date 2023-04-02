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
    Created on 2022-12-04
*/

#pragma once

#include "Config.h"
#include "Types.h"

#include <stdbool.h>
#include <stdint.h>

// Max 3 bits (0..7)
typedef enum
{
    Settings_SchedulerType_Interval,
    Settings_SchedulerType_Segment,
    Settings_SchedulerType_Off
} Settings_SchedulerType;

// Max 3 bits (0..7)
typedef enum
{
    Settings_IntervalSwitchType_Time,
    Settings_IntervalSwitchType_Sunrise,
    Settings_IntervaSwitchType_Sunset
} Settings_IntervalSwitchType;

typedef struct
{
    struct Scheduler
    {
        uint8_t type : 3;               // Settings_SchedulerType
        uint8_t reserved : 5;

        ScheduleSegmentData segmentData;

        struct IntervalScheduler
        {
            uint8_t active : 1;
            uint8_t reserved : 7;

            struct IntervalSwitch
            {
                uint8_t type : 3;       // Settings_IntervalSwitchType
                uint8_t reserved : 5;

                int8_t sunOffset;

                uint8_t timeHour;
                uint8_t timeMinute;
            } onSwitch, offSwitch;
        } intervals[Config_Settings_IntervalScheduleCount];
    } scheduler;

    struct Output
    {
        uint8_t brightness;
    } output;

    struct Display
    {
        uint8_t brightness;
    } display;

#if !SUNRISE_SUNSET_USE_LUT
    struct Location
    {
        uint32_t latitudeBcd;
        uint32_t longitudeBcd;
        uint8_t latitudeSign : 1;
        uint8_t longitudeSign : 1;
    } location;
#endif

    struct Time
    {
        int8_t timeZoneOffsetHalfHours;
    } time;

    // This must be the last field
    uint8_t crc8;
} SettingsData;

extern SettingsData Settings_data;

void Settings_init(void);
void Settings_loadDefaults(void);
void Settings_load(void);
void Settings_save(void);

void SettingsData_initWithDefaults(SettingsData* data);