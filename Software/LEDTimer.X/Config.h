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

#include "Types.h"

/**
 * Settings
 */
#define Config_Settings_ScheduleDataBaseAddress     (0)
#define Config_Settings_ScheduleDataSize            (sizeof(ScheduleSegmentData))
#define Config_Settings_ScheduleDataCheksumAddress  (Config_Settings_ScheduleDataBaseAddress + Config_Settings_ScheduleDataSize)
