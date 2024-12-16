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
#include "Settings.h"
#include "System.h"

// FIXME for testing
#include "SunsetSunrise.h"

#include <stdio.h>
#include <stdbool.h>

#include <xc.h>

static struct MainContext
{
    volatile bool adcConversionFinished;
} context = {
    .adcConversionFinished = false
};

void __interrupt() isr(void)
{
    if (IOCIE && IOCIF) {
        // RA0 IOC - SW1
        if (IOCAF0) {
            IOCAF0 = 0;
            System_handleExternalWakeUp();
        }

        // RA1 IOC - SW2
        if (IOCAF1) {
            IOCAF1 = 0;
            System_handleExternalWakeUp();
        }

        // RC5 IOC - SW3
        if (IOCCF5) {
            IOCCF5 = 0;
            System_handleExternalWakeUp();
        }

        // RA2 IOC - LDO_SENSE
        if (IOCAF2) {
            IOCAF2 = 0;
            System_handleLDOSenseInterrupt();
        }
    }

    if (PEIE) {
        if (ADIE && ADIF) {
            ADIF = 0;
            System_handleADCInterrupt(((uint16_t)ADRESH) << 8 | ADRESL);
            context.adcConversionFinished = true;
        }

        if (TMR4IE & TMR4IF) {
            TMR4IF = 0;
            Clock_handleFastTimerInterrupt();
        }

        if (TMR1IE & TMR1IF) {
            TMR1IF = 0;
            Clock_handleRTCTimerInterrupt();
        }

        if (RCIE && RCIF) {
            char c = RCREG;
            (void)c;
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

/*
                         Main application
 */
void main(void)
{
    System_init();
    Settings_init();
    Settings_load();

    showStartupScreen();

    // Sunrise/Sunset calculation test code

#if 0
    SunriseSunsetData data;

    SunriseSunset_setPosition(
        &data,
        Types_bcdToDouble(
            Settings_data.location.latitudeBcd,
            Settings_data.location.latitudeSign
        ),
        Types_bcdToDouble(
            Settings_data.location.longitudeBcd,
            Settings_data.location.longitudeSign
        )
//        19.046867,
//        47.467442
    );
    SunriseSunset_setTimeZone(&data, 1, false);

    SunriseSunset_Time sunrise = SunriseSunset_calculate(&data, false, 28);
    SunriseSunset_Time sunset = SunriseSunset_calculate(&data, true, 28);

    char s[20];
    sprintf(s, "%2d:%02d, %2d:%02d",
        sunrise.hour, sunrise.minute,
        sunset.hour, sunset.minute
    );
    Text_draw(s, 7, 0, 0, false);
#endif

    System_onWakeUp(System_WakeUpReason_Startup);

    bool runHeavyTasks = true;

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

        Clock_task();

        if (runHeavyTasks) {
            systemTaskResult = System_task();

            if (systemTaskResult.powerInputChanged) {
            }

            if (context.adcConversionFinished) {
                context.adcConversionFinished = false;
            }

            if (systemTaskResult.action == System_TaskResult_EnterSleepMode) {
            }
        }

        if (OutputController_task() == OutputController_TaskResult_OutputStateChanged) {
        }

        if (systemTaskResult.action == System_TaskResult_EnterSleepMode) {
#if DEBUG_ENABLE
            if (runHeavyTasks) {
                ++_DebugState.heavyTaskUpdateValue;
            }
#endif

            System_SleepResult sleepResult = System_sleep();

            runHeavyTasks =
                sleepResult == System_SleepResult_WakeUpFromExternalSource;

            if (runHeavyTasks) {
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
/**
 End of File
*/