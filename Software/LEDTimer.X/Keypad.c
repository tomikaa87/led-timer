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
    Created on 2022-12-01
*/

#include "System.h"

#include "mcc_generated_files/pin_manager.h"

static void _keyPressISR()
{
    System_wakeUp(System_WakeUpReason_KeyPress);
}

void Keypad_init()
{
    IOCAF0_SetInterruptHandler(_keyPressISR);
    IOCAF1_SetInterruptHandler(_keyPressISR);
}

void Keypad_task()
{
    
}