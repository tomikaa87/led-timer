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
        Device            :  PIC16F1825
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
#include "Graphics.h"
#include "Keypad.h"
#include "SSD1306.h"
#include "System.h"
#include "Text.h"
#include "UI.h"

#include <stdio.h>
#include <stdbool.h>

inline void setupI2C()
{
    SSP1CON1bits.SSPEN = 1;
}

/*
                         Main application
 */
void main(void)
{
    // initialize the device
    SYSTEM_Initialize();

    setupI2C();

    __delay_ms(100);
    SSD1306_init();

    System_init();
    Clock_init();
    Keypad_init();
    UI_init();

    SSD1306_setContrastLevel(SSD1306_CONTRAST_LOWEST);

    System_onWakeUp(System_WakeUpReason_Startup);

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

    uint8_t lastKeyCode = 0;
    (void)lastKeyCode;

    while (1)
    {
        uint8_t keyCode = Keypad_task();
#if 0
        if (keyCode != lastKeyCode) {
            lastKeyCode = keyCode;

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

        UI_task(keyCode);

        // This must be the last task to handle sleep mode properly
        System_TaskResult systemTaskResult = System_task();

        if (systemTaskResult.powerInputChanged) {
            UI_onPowerInputChanged();
        }

        if (systemTaskResult.action == System_TaskResult_EnterSleepMode) {
            System_sleep();
            UI_onSystemWakeUp();
        }
    }
}
/**
 End of File
*/