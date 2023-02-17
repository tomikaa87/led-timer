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
 * General
 */
#define Config_FirmwareVersion                              "2.0.0"

/**
 * Settings
 */
#define Config_Settings_DataBaseAddress                     (0)

/**
 * System
 */
#define Config_System_StartupAwakeLengthTicks               (6)
#define Config_System_KeyPressWakeUpLengthTicks             (6)
#define Config_System_PowerInputChangeWakeUpLengthTicks     (6)
#define Config_System_MonitoringUpdateIntervalTicks         (2)

#define Config_System_VDDCalMilliVolts                      (3140ul)
#define Config_System_VDDCalADCValue                        (332ul)

#define Config_System_VBatDiodeDropMilliVolts               (270)

#define Config_System_VBatMinMilliVolts                     (2500u)
#define Config_System_VBatMaxMilliVolts                     (3000u)

/**
 * UI
 */
#define Config_UI_KeyRepeatIntervalTicks                    (10)
#define Config_UI_DisplayTimeoutTicks                       (1000)
#define Config_UI_UpdateIntervalTicks                       (100)

/**
 * Keypad
 */
#define Config_Keypad_ScanSampleCount                       (3)
#define Config_Keypad_ScanSamplingDelayUs                   (1)
#define Config_Keypad_DelayAfterKeysChangedTicks            (10)
#define Config_Keypad_RepeatTimeoutTicks                    (50)
#define Config_Keypad_RepeatIntervalTicks                   (10)
#define Config_Keypad_DeBounceCoolDownTicks                 (5)