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
    
    System_init();
    Clock_init();
    Keypad_init();

    SSD1306_init();
    SSD1306_setContrastLevel(SSD1306_CONTRAST_LOWEST);
    
    ScheduleSegmentData ssd = {
        0, 0, 0, 0, 0, 0
    };

    Graphics_drawScheduleBar(ssd);
    Graphics_drawScheduleSegmentIndicator(12);

    uint16_t lastMinutesFromMidnight = 0;
    
    static const char UpdateIndicatorChars[] = { '.', ':', '=', ':' };
    uint8_t vddUpdateIndicatorIndex = 0;
    uint8_t updateIndicatorIndex = 0;
    
    while (1)
    {
        Keypad_task();
        
        // This must be the last task to handle sleep mode properly
        System_TaskResult systemTaskResult = System_task();

        char s[20];
        
        snprintf(s, sizeof(s), ":%02u", Clock_getSeconds());
        Text_draw(s, 0, 30, 0);
        
        if (System_isVDDReadingUpdated()) {
            snprintf(s, sizeof(s), "VDD=%4umV %c", System_getVDDMilliVolts(), UpdateIndicatorChars[vddUpdateIndicatorIndex]);
            Text_draw(s, 2, 0, 0);

            if (++vddUpdateIndicatorIndex >= sizeof(UpdateIndicatorChars)) {
                vddUpdateIndicatorIndex = 0;
            }
        }
        
        snprintf(s, sizeof(s), "Updates: %c", UpdateIndicatorChars[updateIndicatorIndex]);
        Text_draw(s, 3, 0, 0);
        if (++updateIndicatorIndex >= sizeof(UpdateIndicatorChars)) {
            updateIndicatorIndex = 0;
        }
        
        if (
            lastMinutesFromMidnight == 0
            || Clock_getMinutesSinceMidnight() != lastMinutesFromMidnight
        ) {
            lastMinutesFromMidnight = Clock_getMinutesSinceMidnight();
            
            uint8_t hours = (uint8_t)lastMinutesFromMidnight / 60;
            uint8_t minutes = (uint8_t)lastMinutesFromMidnight - hours * 60;

            snprintf(s, sizeof(s), "%02u:%02u", hours, minutes);
            Text_draw(s, 0, 0, 0);
        }
        
        if (systemTaskResult == System_TaskResult_EnterSleepMode) {
            System_sleep();
        }
    }
}
/**
 End of File
*/