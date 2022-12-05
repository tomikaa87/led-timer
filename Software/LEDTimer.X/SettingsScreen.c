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
    Created on 2022-12-05
*/

#include "Keypad.h"
#include "SettingsScreen.h"
#include "Text.h"

static struct SettingsScreenContext {

} context = {

};

static void drawKeypadHelpBar()
{
    Text_draw("EXIT", 0, 0, 0, false);
}

void SettingsScreen_update(const bool redraw)
{
    if (redraw) {
        drawKeypadHelpBar();
    }
}

bool SettingsScreen_handleKeyPress(const uint8_t keyCode, const bool hold)
{
    switch (keyCode) {
        case Keypad_Key1:
            break;

        case Keypad_Key2:
        case Keypad_Key3:
            return true;
    }

    return false;
}
