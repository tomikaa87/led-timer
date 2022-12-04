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

void Settings_init()
{

}

inline void Settings_saveScheduleData(const ScheduleSegmentData data)
{
    saveData(
        Config_Settings_ScheduleDataBaseAddress,
        (const uint8_t*)data,
        Config_Settings_ScheduleDataSize
    );
}

inline void Settings_loadScheduleData(ScheduleSegmentData data)
{
    loadData(
        Config_Settings_ScheduleDataBaseAddress,
        (uint8_t*)data,
        Config_Settings_ScheduleDataSize
    );
}

inline void Settings_saveScheduleDataChecksum(const uint8_t checksum)
{
    saveData(
        Config_Settings_ScheduleDataCheksumAddress,
        &checksum,
        1
    );
}

inline uint8_t Settings_loadScheduleDataChecksum()
{
    uint8_t checksum = 0;

    loadData(
        Config_Settings_ScheduleDataCheksumAddress,
        &checksum,
        1
    );

    return checksum;
}