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

// CONFIG1
#pragma config FEXTOSC = OFF    // FEXTOSC External Oscillator mode Selection bits (Oscillator not enabled)
#pragma config RSTOSC = HFINT1  // Power-up default value for COSC bits (HFINTOSC (1MHz))
#pragma config CLKOUTEN = OFF   // Clock Out Enable bit (CLKOUT function is disabled; I/O or oscillator function on OSC2)
#pragma config CSWEN = ON       // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is disabled)

// CONFIG2
#pragma config MCLRE = ON       // Master Clear Enable bit (MCLR/VPP pin function is MCLR; Weak pull-up enabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config WDTE = OFF       // Watchdog Timer Enable bits (WDT disabled; SWDTEN is ignored)
#pragma config LPBOREN = OFF    // Low-power BOR enable bit (ULPBOR disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bits (Brown-out Reset disabled)
#pragma config BORV = LOW       // Brown-out Reset Voltage selection bit (Brown-out voltage (Vbor) set to 2.45V)
#pragma config PPS1WAY = OFF    // PPSLOCK bit One-Way Set Enable bit (The PPSLOCK bit can be set and cleared repeatedly (subject to the unlock sequence))
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable bit (Stack Overflow or Underflow will cause a Reset)
#pragma config DEBUG = OFF      // Debugger enable bit (Background debugger disabled)

// CONFIG3
#pragma config WRT = OFF        // User NVM self-write protection bits (Write protection off)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (High Voltage on MCLR/VPP must be used for programming.)

// CONFIG4
#pragma config CP = OFF         // User NVM Program Memory Code Protection bit (User NVM code protection disabled)
#pragma config CPD = OFF        // Data NVM Memory Code Protection bit (Data NVM code protection disabled)

#include "Clock.h"
#include "Config.h"
#include "System.h"

#include <stdio.h>

#include <xc.h>

volatile uint16_t _System_adcResult = 0;

#define DEBUG_REDIRECT_EUSART 1

System_InterruptContext System_interruptContext = {
    .adc = {
        .result = 0,
        .updated = false
    },
    .ldoSense = {
        .updated = false
    },
    .externalWakeUpSource = false
};

static struct SystemContext
{
    struct _Sleep
    {
        volatile bool enabled;
        volatile Clock_Ticks lastWakeUpTime;
        volatile System_WakeUpReason wakeUpReason;
    } sleep;

    struct _Monitoring
    {
        Clock_Ticks lastUpdateTime;
        uint8_t batteryLevel;               // Estimated battery level: 0..10
    } monitoring;
} context = {
    .sleep = {
        .enabled = true,
        .lastWakeUpTime = 0,
        .wakeUpReason = System_WakeUpReason_None
    },
    .monitoring = {
        .lastUpdateTime = 0,
        .batteryLevel = 0,
    }
};

static void measureVDD()
{
    if (!FVRCONbits.FVREN) {
        return;
    }

    // Wait for the FVR to stabilize
    while (!FVRCONbits.FVRRDY);

//    ADC_SelectChannel(channel_FVR);
    ADCON0bits.GO = 1;
}

void updateBatteryLevel()
{
    uint16_t vbat = System_getVBatMilliVolts();
    vbat =
        vbat > Config_System_VBatMaxMilliVolts
            ? Config_System_VBatMaxMilliVolts
            : (
                vbat < Config_System_VBatMinMilliVolts
                ? Config_System_VBatMinMilliVolts
                : vbat
            );

    context.monitoring.batteryLevel = (uint8_t)(
        (uint32_t)(vbat - Config_System_VBatMinMilliVolts) * 10u
        / (Config_System_VBatMaxMilliVolts - Config_System_VBatMinMilliVolts)
    );

#if DEBUG_ENABLE_PRINT
    printf(
        "SYS:VDD=%u(ADC=%u),VBat=%u,L=%u\r\n",
        System_getVDDMilliVolts(),
        System_interruptContext.adc.result,
        System_getVBatMilliVolts(),
        context.monitoring.batteryLevel
    );
#endif
}

void System_init()
{
    // Oscillator
    OSCCON1 =
        (0b110 << 4)                        // NOSC=HFINTOSC
        | 0b0000;                           // NDIV=1
    OSCEN =
        (1 << 6)                            // HFOEN=1
        | (1 << 4)                          // LFOEN=1
        | (1 << 3)                          // SOSCEN=1
        | (1 << 2);                         // ADOEN=1
    OSCFRQ = 0b0110;                        // HFFRQ=16 (Mhz)

    // Comparator
    CM1CON1 = 0b100100;                     // CxV(N/P) is unconnected
    CM2CON1 = 0b100100;

    // GPIO
    ANSELA = 0x37
        & ~(1 << 2)                         // RA2 is digital
#if !DEBUG_REDIRECT_EUSART
        & ~(1 << 1)                         // RA1 is digital
        & ~1                                // RA0 is digital
#endif
        ;

    TRISA = 0x3F
#if !DEBUG_REDIRECT_EUSART
        & ~1                                // RAO is output
#endif
        ;

    WPUA = 0x3F
#if !DEBUG_REDIRECT_EUSART
        & ~1                                // WPUA0=0
#endif
        ;

    ANSELC = 0xFF
        & ~(1 << 4)                         // RC4 is digital
        & ~(1 << 3)                         // RC3 is digital
        & ~(1 << 2)                         // RC2 is digital
#if DEBUG_REDIRECT_EUSART
        & ~(1 << 1)                         // RC1 is digital
        & ~1                                // RC0 is digital
#endif
        ;

    TRISC = 0xFF
        & ~(1 << 4)                         // RC4 is output
        & ~(1 << 2)                         // RC2 is output
#if DEBUG_REDIRECT_EUSART
        & ~1                                // RC0 is output
#endif
        ;

    WPUC = 0xFF
        & ~(1 << 4)                         // WPUC4=0
        & ~(1 << 2)                         // WPUC2=0
#if DEBUG_REDIRECT_EUSART
        & ~1                                // WPUC0=0
#endif
        ;

    // PPS
#if DEBUG_REDIRECT_EUSART
    RC0PPS = 0b10100;                       // RC0PPS=TX
    RXPPS = 0b10001;                        // RXPPS=RC1
#else
    RA0PPS = 0b10100;                       // RA0PPS=TX
    RXPPS = 0b00001;                        // RXPPS=RA1
#endif
    RC4PPS = 0b00010;                       // RC4PPS=PWM5

    // IOC
    IOCAN =
        (1 << 2);                           // RA2 interrupt on negative edge
    IOCAP =
        (1 << 2);                           // RA2 interrupt on positive edge
    IOCCN =
        (1 << 3);                           // RC3 interrupt on negative edge

    // FVR
    FVRCON =
        (1 << 7)                            // FVREN=1
        | 0b01;                             // ADCFVR=1x

    // PWM5
    PWM5CON = (1 << 7);                     // PWM5EN=1
    PWMTMRS = 0b01;                         // P5TSEL=TMR2

    // ADC
    ADCON0 =
        (0b111111 << 2)                     // CHS=FVR
        | 0b1;                              // ADON=1
    ADCON1 =
        (1 << 7)                            // ADFM=1
        | (0b111 << 4);                     // ADCS=ADCRC

    // Timer1
    TMR1L = 0;
    TMR1H = 0;
    T1CON =
        (0b10 << 6)                         // TMR1CS=SOSC
        | (1 << 3)                          // T1SOSC=1
        | (1 << 2)                          // T1SYNC=off
        | 1;                                // TMR1ON=1

    // Timer2 : PWM clock, 62500 Hz (16 us) @ 16 MHz
    PR2 = 0x3F;                             // 32768 Hz @ Fosc/4=4MHz
    T2CON = 1 << 2;                         // TMR2ON=1

    // Timer4 : 25000 Hz (10 ms) @ 16 MHz
    PR4 = 0xF9;
    T4CON =
        (0b1001 << 3)                       // T4OUTPS=10
        | (1 << 2)                          // TMR4ON
        | 0b10;                             // T4CKPS=16

    // EUSART : 115200 bps @ 16 MHz
    TX1STA =
        (1 << 5)                            // TXEN=1
        | (1 << 2);                         // BRGH=1
    RC1STA =
        (1 << 7)                            // SPEN=1
        | (1 << 4);                         // CREN=1
    BAUD1CON = 1 << 3;                      // BRG16=1
    SP1BRGL = 0x22;                         // Actual: 114285.71 bps
    SP1BRGH = 0x00;

    // PMD
    PMD0 =
        (1 << 1);                           // CLKRMD=1
    PMD1 =
        (1 << 7)                            // NCOMD=1
        | (1 << 6)                          // TMR6MD=1
        | (1 << 5)                          // TMR5MD=1
        | (1 << 3)                          // TMR3MD=1
        | 1;                                // TMR0MD=1
    PMD2 =
        (1 << 6)                            // DACMD=1
        | (1 << 2)                          // CMP2MD=1
        | (1 << 1);                         // CMP1MD=1
    PMD3 =
        (1 << 7)                            // CWG2MD=1
        | (1 << 6)                          // CWG1MD=1
        | (1 << 5)                          // PWM6MD=1
        | (1 << 3)                          // CCP4MD=1
        | (1 << 2)                          // CCP3MD=1
        | (1 << 1)                          // CCP2MD=1
        | 1;                                // CCP1MD=1
    PMD4 =
        (1 << 2)                            // MSSP2MD=1
        | (1 << 1);                         // MSSP1MD=1;
    PMD5 =
        (1 << 4)                            // CLC4MD=1
        | (1 << 3)                          // CLC3MD=1
        | (1 << 2)                          // CLC2MD=1
        | (1 << 1)                          // CLC1MD=1
        | 1;                                // DSMMD=1

    // Interrupts
    PIE0 =
        (1 << 4);                           // IOCIE=1
    PIE1 =
        (1 << 6)                            // ADIE=1
        | (1 << 5)                          // RCIE=1
        | 1;                                // TMR1IE=1
    PIE2 =
        (1 << 1);                           // TMR4IE=1
    INTCON =
        (1 << 7)                            // GIE=1
        | (1 << 6);                         // PEIE=1

    measureVDD();

    // Wait for the initial measurement to avoid invalid readings after startup
    while (!System_interruptContext.adc.updated);

    updateBatteryLevel();
}

System_TaskResult System_task()
{
    System_TaskResult result = {
        .action = System_TaskResult_NoActionNeeded,
        .powerInputChanged = System_interruptContext.ldoSense.updated
    };

    // To keep the screen on for a while after a power input change,
    // but avoid overwriting the existing reason (if any)
    if (
        context.sleep.wakeUpReason == System_WakeUpReason_None
        && result.powerInputChanged
    ) {
        System_onWakeUp(System_WakeUpReason_PowerInputChanged);
    }

    System_interruptContext.ldoSense.updated = false;

    if (
        result.powerInputChanged
        || Clock_getElapsedTicks(context.monitoring.lastUpdateTime)
            >= Config_System_MonitoringUpdateIntervalTicks
    ) {
        context.monitoring.lastUpdateTime = Clock_getTicks();
        measureVDD();
        updateBatteryLevel();
    }

    if (!context.sleep.enabled) {
        Clock_Ticks elapsedSinceWakeUp
            = Clock_getElapsedTicks(context.sleep.lastWakeUpTime);

        switch (context.sleep.wakeUpReason) {
            default:
            case System_WakeUpReason_None:
                context.sleep.enabled = System_isRunningFromBackupBattery();
                break;

            case System_WakeUpReason_Startup:
                if (elapsedSinceWakeUp >= Config_System_StartupAwakeLengthTicks) {
                    context.sleep.enabled = System_isRunningFromBackupBattery();
                }
                break;

            case System_WakeUpReason_KeyPress:
                if (elapsedSinceWakeUp >= Config_System_KeyPressWakeUpLengthTicks) {
                    context.sleep.enabled = System_isRunningFromBackupBattery();
                }
                break;

            case System_WakeUpReason_PowerInputChanged:
                if (elapsedSinceWakeUp >= Config_System_PowerInputChangeWakeUpLengthTicks) {
                    context.sleep.enabled = System_isRunningFromBackupBattery();
                }
                break;
        }
    } else {
        result.action = System_TaskResult_EnterSleepMode;
    }

    return result;
}

void System_onWakeUp(const System_WakeUpReason reason)
{
    context.sleep.enabled = false;
    context.sleep.lastWakeUpTime = Clock_getTicks();
    context.sleep.wakeUpReason = reason;
}

inline System_WakeUpReason System_getLastWakeUpReason()
{
    return context.sleep.wakeUpReason;
}

System_SleepResult System_sleep()
{
#if DEBUG_ENABLE_PRINT
    printf(
        "SYS:SLP,T=%u,FT=%u,MSM=%u,SEC=%u\r\n",
        Clock_getTicks(),
        Clock_getFastTicks(),
        Clock_getMinutesSinceMidnight(),
        Clock_getSeconds()
    );
#endif

    // Send out all the data before going to sleep
    while (UART1MD == 0 && TRMT == 0);

    // Disable the FVR to conserve power
    FVRCONbits.FVREN = 0;

    context.sleep.wakeUpReason = System_WakeUpReason_None;

#if DEBUG_ENABLE
    _DebugState.sleeping = true;
    _DebugState.externalWakeUp = false;
    _DebugState.ldoSenseValue = IO_LDO_SENSE_GetValue();
    UI_updateDebugDisplay();
#endif

    SLEEP();

    // The next instruction will always be executed before the ISR

    FVRCONbits.FVREN = 1;

    // Clear the flag so the next task() call can update it properly
    context.sleep.enabled = false;

#if DEBUG_ENABLE
    _DebugState.sleeping = false;
    _DebugState.externalWakeUp = System_interruptContext.externalWakeUpSource;
    _DebugState.ldoSenseValue = IO_LDO_SENSE_GetValue();
    UI_updateDebugDisplay();
#endif

    // Wait for the oscillator to stabilize
    while (
        OSCCON3bits.ORDY == 0
        || OSCSTAT1bits.HFOR == 0
    );

    if (System_interruptContext.externalWakeUpSource) {
        System_interruptContext.externalWakeUpSource = false;
        return System_SleepResult_WakeUpFromExternalSource;
    }

    return System_SleepResult_WakeUpFromInternalSource;
}

inline bool System_isRunningFromBackupBattery()
{
    return RA2 == 1;
}

uint16_t System_getVDDMilliVolts()
{
    return (uint16_t)(
        Config_System_VDDCalMilliVolts
        * Config_System_VDDCalADCValue
        / System_interruptContext.adc.result
    );
}

uint16_t System_getVBatMilliVolts()
{
    return System_getVDDMilliVolts() + Config_System_VBatDiodeDropMilliVolts;
}

inline uint8_t System_getBatteryLevel()
{
    return context.monitoring.batteryLevel;
}