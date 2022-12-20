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


#include "Config.h"
#include "Settings.h"

#include "mcc_generated_files/memory.h"

#include <stdio.h>
#include <string.h>

SettingsData Settings_data;

static void saveData(
    uint8_t address,
    const uint8_t* data,
    uint8_t size
) {
    while (size--) {
        DATAEE_WriteByte(address++, *data++);
    }
}

static void loadData(
    uint8_t address,
    uint8_t* data,
    uint8_t size
) {
    while (size--) {
        *data++ = DATAEE_ReadByte(address++);
    }
}

static uint8_t calculateCRC8(const uint8_t* data, uint8_t length)
{
#define Generator   0x07u
#define Init        0u

    uint8_t crc = Init;

    while (length--) {
        crc ^= *data++;

        for (uint8_t i = 0; i < 8; ++i) {
            if (crc & 0x80) {
                crc = (uint8_t)(crc << 1) ^ Generator;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

void Settings_init()
{
    Settings_loadDefaults();
}

void Settings_loadDefaults()
{
    SettingsData_initWithDefaults(&Settings_data);
}

void Settings_load()
{
//    puts("STNGS:load");

    loadData(
        Config_Settings_DataBaseAddress,
        (uint8_t*)&Settings_data,
        sizeof(SettingsData)
    );

    uint8_t loadedChecksum = Settings_data._checksum;

    Settings_data._checksum = 0;
    uint8_t calculatedChecksum = calculateCRC8(
        (uint8_t*)&Settings_data,
        sizeof(SettingsData)
    );

    if (loadedChecksum != calculatedChecksum) {
//        puts("STNGS:checksumError");
        Settings_loadDefaults();
    }
}

void Settings_save()
{
//    puts("STNGS:load");

    Settings_data._checksum = 0;

    uint8_t checksum = calculateCRC8(
        (uint8_t*)&Settings_data,
        sizeof(SettingsData)
    );

    Settings_data._checksum = checksum;

    saveData(
        Config_Settings_DataBaseAddress,
        (uint8_t*)&Settings_data,
        sizeof(SettingsData)
    );
}

void SettingsData_initWithDefaults(SettingsData* const data)
{
    memset(data, 0, sizeof(SettingsData));

    data->output.brightness = 255;
}