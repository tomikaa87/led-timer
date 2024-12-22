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
    Created on 2024-12-22
*/

#include "Clock.h"
#include "Config.h"
#include "OutputController.h"
#include "ProgrammingInterface.h"
#include "Settings.h"
#include "System.h"
#include "UserInterface.h"

#include <stdbool.h>

#include <xc.h>

static struct MainContext
{
    volatile uint8_t adcConversionFinished : 1;
    volatile uint8_t buttonPressed : 1;
    volatile uint8_t ldoSenseChanged : 1;
} context = {
    .adcConversionFinished = 0,
    .buttonPressed = 0,
    .ldoSenseChanged = 0
};

void __interrupt() isr(void)
{
    // Timer1 (RTC)
    if (TMR1IE && TMR1IF) {
        TMR1IF = 0;
        Clock_handleRTCTimerInterrupt();
    }

    // Timer4 (FastTick)
    if (TMR4IE && TMR4IF) {
        TMR4IF = 0;
        Clock_handleFastTimerInterrupt();
    }

    // ADC
    if (ADIE && ADIF) {
        ADIF = 0;
        System_handleADCInterrupt(((uint16_t)ADRESH) << 8 | ADRESL);
        context.adcConversionFinished = 1;
    }

    // UART RC (Programming Interface)
    if (RCIE && RCIF) {
        char c = RCREG;
        ProgrammingInterface_processInputChar(c);
    }

    // GPIO Interrupt-on-change
    if (IOCIE && IOCIF) {
        // RC3 IOC - SW
        if (IOCCF3) {
            IOCCF3 = 0;
            context.buttonPressed = 1;
//            System_handleExternalWakeUp();
            ProgrammingInterface_logEvent(PI_LOG_ButtonPress);
        }

        // RA2 IOC - LDO_SENSE
        if (IOCAF2) {
            IOCAF2 = 0;
            context.ldoSenseChanged = 1;
            ProgrammingInterface_logEvent(
                RA2 == 1 ? PI_LOG_LDOPowerDown : PI_LOG_LDOPowerUp
            );
        }
    }
}

inline static void showStartupScreen()
{
    if (PCON0bits.STKOVF) {
        PCON0bits.STKOVF = 0;
    }

    if (PCON0bits.STKUNF) {
        PCON0bits.STKUNF = 0;
    }

    if (!PCON0bits.nRWDT) {
        PCON0bits.nRWDT = 1;
    }

    if (!PCON0bits.nRMCLR) {
        PCON0bits.nRMCLR = 1;
    }

    if (!PCON0bits.nRI) {
        PCON0bits.nRI = 1;
    }

    if (BORRDY && !PCON0bits.nBOR) {
        PCON0bits.nBOR = 1;
    }
}

void main(void)
{
    System_init();

    Settings_init();
    Settings_load();

    ProgrammingInterface_init();
    ProgrammingInterface_logEvent(PI_LOG_Startup);

    UserInterface_init();

    while (true) {
        if (System_isRunningFromBackupBattery()) {
            // Do only minimal number of tasks to save power

            // Block sleep if UI has pending lightweight tasks
            if (!UserInterface_hasPendingLightweightTasks()) {
                System_prepareForSleepMode();
                SLEEP();
                System_runTasksAfterWakeUp();
            }
        } else {
            // We have external power, do every task

            Clock_runTasks();

            if (OutputController_runTasks() == OutputController_TaskResult_OutputStateChanged) {
                UserInterface_handleExternalEvent(UI_ExternalEvent_OutputStateChanged);
            }

            UserInterface_runTasks();

            ProgrammingInterface_runTasks();
        }

        // Handle button press
        if (context.buttonPressed) {
            context.buttonPressed = 0;
            UserInterface_buttonPressEvent();
        }

        // Handle LDO sense changes
        if (context.ldoSenseChanged) {
            context.ldoSenseChanged = 0;
            UserInterface_handleExternalEvent(UI_ExternalEvent_PowerInputChanged);
        }

        UserInterface_runLightweightTasks();
    }
}
