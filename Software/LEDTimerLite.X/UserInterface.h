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

typedef enum
{
    UI_ExternalEvent_SystemWakeUp =                     (1 << 0),
    UI_ExternalEvent_SystemGoingToSleep =               (1 << 1),
    UI_ExternalEvent_PowerInputChanged =                (1 << 2),
    UI_ExternalEvent_BatteryLevelMeasurementFinished =  (1 << 3),
    UI_ExternalEvent_OutputStateChanged =               (1 << 4)
} UI_ExternalEvent;

void UserInterface_init(void);
void UserInterface_task(void);
void UserInterface_buttonPressEvent(void);
inline void UserInterface_setExternalEvent(UI_ExternalEvent event);



