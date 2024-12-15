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

#include <stdio.h>
#include <string.h>

#include <xc.h>

static void DATAEE_WriteByte(uint8_t bAdd, uint8_t bData)
{
    uint8_t GIEBitValue = INTCONbits.GIE;

    NVMADRH = 0x70;
    NVMADRL = bAdd;
    NVMDATL = bData;
    NVMCON1bits.NVMREGS = 1;
    NVMCON1bits.WREN = 1;
    INTCONbits.GIE = 0;     // Disable interrupts
    NVMCON2 = 0x55;
    NVMCON2 = 0xAA;
    NVMCON1bits.WR = 1;
    // Wait for write to complete
    while (NVMCON1bits.WR)
    {
    }

    NVMCON1bits.WREN = 0;
    INTCONbits.GIE = GIEBitValue;   // restore interrupt enable
}

static uint8_t DATAEE_ReadByte(uint8_t bAdd)
{
    NVMADRH = 0x70;
    NVMADRL = bAdd;
    NVMCON1bits.NVMREGS = 1;
    NVMCON1bits.RD = 1;
    NOP();  // NOPs may be required for latency at high frequencies
    NOP();

    return (NVMDATL);
}

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

        for (uint8_t i = 8; i > 0; --i) {
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
#if DEBUG_ENABLE_PRINT
    puts("STNGS:load");
#endif

    // TODO potential optimization: calculate CRC8 while loading
    loadData(
        Config_Settings_DataBaseAddress,
        (uint8_t*)&Settings_data,
        sizeof(SettingsData)
    );

    uint8_t crc8 = calculateCRC8(
        (uint8_t*)&Settings_data,
        sizeof(SettingsData)
    );

    if (crc8 != 0) {
#if DEBUG_ENABLE_PRINT
        puts("STNGS:crcCheckFailed");
#endif
        Settings_loadDefaults();
    }
}

void Settings_save()
{
#if DEBUG_ENABLE_PRINT
    puts("STNGS:load");
#endif

    Settings_data.crc8 = calculateCRC8(
        (uint8_t*)&Settings_data,
        sizeof(SettingsData) - 1
    );

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

    data->display.brightness = 2;

    // Default DST settings for EU
    // Source: https://en.wikipedia.org/wiki/Daylight_saving_time_by_country
    data->dst.startOrdinal = 2;
    data->dst.startDayOfWeek = 0;
    data->dst.startMonth = 2;
    data->dst.startShiftHours = 1;
//    data->dst.startShiftHoursLocal = 0;
    data->dst.endOrdinal = 2;
    data->dst.endDayOfWeek = 0;
    data->dst.endMonth = 9;
    data->dst.endShiftHours = 1;
//    data->dst.endShiftHoursLocal = 0;
}