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
    Created on 2024-12-16
*/

#pragma once

typedef enum {
    PI_LOG_Startup,
    PI_LOG_LDOPowerUp,
    PI_LOG_LDOPowerDown,
    PI_LOG_ButtonPress,
    PI_LOG_EnterSleepMode,
    PI_LOG_LeaveSleepMode
} PI_LogEvent;

void ProgrammingInterface_init(void);
void ProgrammingInterface_task(void);
void ProgrammingInterface_logEvent(PI_LogEvent event);
void ProgrammingInterface_processInputChar(char c);