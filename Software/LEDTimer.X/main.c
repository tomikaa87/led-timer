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

#include "mcc_generated_files/mcc.h"

#include "Clock.h"
#include "Config.h"
#include "Graphics.h"
#include "Keypad.h"
#include "OutputController.h"
#include "Settings.h"
#include "SSD1306.h"
#include "System.h"
#include "Text.h"
#include "UI.h"

#include <stdio.h>
#include <stdbool.h>

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
            System_handleExternalWakeUp();
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
    }
}

inline static void setupI2C()
{
    SSP1CON1bits.SSPEN = 1;
}

inline static void showStartupScreen()
{
    SSD1306_clear();

    static const uint8_t MiniBulbIcon[] = {
        0b00011110,
        0b11100001,
        0b10111001,
        0b11100001,
        0b00011110
    };

    static const char* Title = "LED TIMER";
    static const char* GitHub = "GITHUB.COM/TOMIKAA87";

    uint8_t titleWidth = Text_calculateWidth(Title);
    uint8_t iconPos = 64 - (titleWidth + 5 + 5) / 2;

    Graphics_drawBitmap(MiniBulbIcon, 5, iconPos, 1, false);
    Text_draw(Title, 1, iconPos + 10, 0, false);

    const char* firmwareVersion = "v" Config_FirmwareVersion;

    Text_draw(
        firmwareVersion,
        3,
        64 - Text_calculateWidth(firmwareVersion) / 2,
        0,
        false
    );

    Text_draw(GitHub, 6, 64 - Text_calculateWidth(GitHub) / 2, 0, false);

    if (PCON0bits.STKOVF) {
        Text_draw("O", 0, 0, 0, false);
        PCON0bits.STKOVF = 0;
    }

    if (PCON0bits.STKUNF) {
        Text_draw("U", 0, 6, 0, false);
        PCON0bits.STKUNF = 0;
    }

    if (!PCON0bits.nRWDT) {
        Text_draw("U", 0, 12, 0, false);
        PCON0bits.nRWDT = 1;
    }

    if (!PCON0bits.nRMCLR) {
        Text_draw("M", 0, 18, 0, false);
        PCON0bits.nRMCLR = 1;
    }

    if (!PCON0bits.nRI) {
        Text_draw("I", 0, 24, 0, false);
        PCON0bits.nRI = 1;
    }

    if (!PCON0bits.nBOR) {
        Text_draw("B", 0, 30, 0, false);
        PCON0bits.nBOR = 1;
    }
}

/*
                         Main application
 */
void main(void)
{
    // initialize the device
    SYSTEM_Initialize();

    // When using interrupts, you need to set the Global and Peripheral Interrupt Enable bits
    // Use the following macros to:

    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Enable the Peripheral Interrupts
    INTERRUPT_PeripheralInterruptEnable();

    // Disable the Global Interrupts
    //INTERRUPT_GlobalInterruptDisable();

    // Disable the Peripheral Interrupts
    //INTERRUPT_PeripheralInterruptDisable();

    setupI2C();

    __delay_ms(100);
    SSD1306_init();

    System_init();
    Keypad_init();
    Settings_init();
    Settings_load();

    SSD1306_setContrastLevel(Settings_data.display.brightness);

    showStartupScreen();
    __delay_ms(5000);
    SSD1306_clear();

    UI_init();

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

        if (runHeavyTasks) {
            uint8_t keyCode = Keypad_task();
    #if DEBUG_ENABLE_PRINT
            if (keyCode != 0) {
                printf(
                    "Keys(%02X): %1s %1s %1s %s\r\n",
                    keyCode,
                    keyCode & Keypad_Key1 ? "1" : "",
                    keyCode & Keypad_Key2 ? "2" : "",
                    keyCode & Keypad_Key3 ? "3" : "",
                    keyCode & Keypad_Hold ? "Hold" : ""
                );
            }
    #endif

            UI_keyEvent(keyCode);

            systemTaskResult = System_task();

            if (systemTaskResult.powerInputChanged) {
                UI_setExternalEvent(UI_ExternalEvent_PowerInputChanged);
            }

            if (context.adcConversionFinished) {
                context.adcConversionFinished = false;
                UI_setExternalEvent(UI_ExternalEvent_BatteryLevelMeasurementFinished);
            }

            if (systemTaskResult.action == System_TaskResult_EnterSleepMode) {
                UI_setExternalEvent(UI_ExternalEvent_SystemGoingToSleep);
            }

            UI_task();
        }

        OutputController_task();

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
                UI_setExternalEvent(UI_ExternalEvent_SystemWakeUp);
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