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

typedef enum
{
    Settings_SchedulerType_Segment,
    Settings_SchedulerType_Simple,
    Settings_SchedulerType_Off
} Settings_SchedulerType;

typedef enum
{
    Settings_TriggerType_Time,
    Settings_TriggerType_Sunrise,
    Settings_TriggerType_Sunset
} Settings_TriggerType;

typedef struct
{
    struct Scheduler
    {
        Settings_SchedulerType type;

        ScheduleSegmentData segmentData;

        struct ScheduleTrigger
        {
            Settings_TriggerType type;
            int8_t sunOffset;
            uint8_t timeHour;
            uint8_t timeMinute;
        } onTrigger, offTrigger;
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

    // This must be the last member
    uint8_t crc8;
} SettingsData;

extern SettingsData Settings_data;

void Settings_init(void);
void Settings_loadDefaults(void);
void Settings_load(void);
void Settings_save(void);

void SettingsData_initWithDefaults(SettingsData* data);