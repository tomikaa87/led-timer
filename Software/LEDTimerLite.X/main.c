/**
  Generated Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This is the main file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  Description:
    This header file provides implementations for driver APIs for all modules selected in the GUI.
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.81.8
        Device            :  PIC16F18326
        Driver Version    :  2.00
*/

/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip software and any
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party
    license terms applicable to your use of third party software (including open source software) that
    may accompany Microchip software.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
    FOR A PARTICULAR PURPOSE.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS
    SOFTWARE.
*/

#include "Clock.h"
#include "Config.h"
#include "OutputController.h"
#include "ProgrammingInterface.h"
#include "Settings.h"
#include "System.h"
#include "UserInterface.h"

#include <stdio.h>
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
//            System_handleLDOSenseInterrupt();
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
//        Text_draw("O", 0, 0, 0, false);
        PCON0bits.STKOVF = 0;
    }

    if (PCON0bits.STKUNF) {
//        Text_draw("U", 0, 6, 0, false);
        PCON0bits.STKUNF = 0;
    }

    if (!PCON0bits.nRWDT) {
//        Text_draw("U", 0, 12, 0, false);
        PCON0bits.nRWDT = 1;
    }

    if (!PCON0bits.nRMCLR) {
//        Text_draw("M", 0, 18, 0, false);
        PCON0bits.nRMCLR = 1;
    }

    if (!PCON0bits.nRI) {
//        Text_draw("I", 0, 24, 0, false);
        PCON0bits.nRI = 1;
    }

    if (BORRDY && !PCON0bits.nBOR) {
//        Text_draw("B", 0, 30, 0, false);
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

#if 0
/*
                         Main application
 */
void main_(void)
{
    System_init();

    Settings_init();
    Settings_load();

    ProgrammingInterface_init();
    ProgrammingInterface_logEvent(PI_LOG_Startup);

    UserInterface_init();

//    showStartupScreen();

    System_onWakeUp(System_WakeUpReason_Startup);

    bool runHeavyTasks = true;

    System_handleLDOSenseInterrupt();

    System_TaskResult systemTaskResult = {
        .action = System_TaskResult_NoActionNeeded,
        .powerInputChanged = false
    };

    while (1)
    {
#if DEBUG_ENABLE
        static Clock_Ticks lastTicks = 0;
        if (Clock_getTicks() != lastTicks) {
            lastTicks = Clock_getTicks();
            UI_updateDebugDisplay();
        }
#endif

        Clock_runTasks();

        if (runHeavyTasks) {
            // Handle button press
            if (context.buttonPressed) {
                context.buttonPressed = 0;
                UserInterface_buttonPressEvent();
                System_onWakeUp(System_WakeUpReason_KeyPress);
            }

            systemTaskResult = System_task();

            if (systemTaskResult.powerInputChanged) {
                UserInterface_handleExternalEvent(UI_ExternalEvent_PowerInputChanged);
            }

            if (context.adcConversionFinished) {
                context.adcConversionFinished = false;
                UserInterface_handleExternalEvent(UI_ExternalEvent_BatteryLevelMeasurementFinished);
            }

            if (systemTaskResult.action == System_TaskResult_EnterSleepMode) {
                UserInterface_handleExternalEvent(UI_ExternalEvent_SystemGoingToSleep);
            }

            ProgrammingInterface_runTasks();

            UserInterface_runTasks();
        }

        if (OutputController_runTasks() == OutputController_TaskResult_OutputStateChanged) {
            UserInterface_handleExternalEvent(UI_ExternalEvent_OutputStateChanged);
        }

        if (systemTaskResult.action == System_TaskResult_EnterSleepMode) {
#if DEBUG_ENABLE
            if (runHeavyTasks) {
                ++_DebugState.heavyTaskUpdateValue;
            }
#endif

            ProgrammingInterface_logEvent(PI_LOG_EnterSleepMode);

            System_SleepResult sleepResult = System_sleep();

            ProgrammingInterface_logEvent(PI_LOG_LeaveSleepMode);

            runHeavyTasks =
                sleepResult == System_SleepResult_WakeUpFromExternalSource;

            if (runHeavyTasks) {
                UserInterface_handleExternalEvent(UI_ExternalEvent_SystemWakeUp);
#if DEBUG_ENABLE
                ++_DebugState.heavyTaskUpdateValue;
#endif
            } else {
                // Go back to sleep without running System_task()
                systemTaskResult.action = System_TaskResult_EnterSleepMode;
            }
        }
    }
}
#endif
/**
 End of File
*/