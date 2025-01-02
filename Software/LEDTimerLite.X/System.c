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

#undef DEBUG_ENABLE_PRINT
#define DEBUG_ENABLE_PRINT 1
#define DEBUG_REDIRECT_EUSART 1

static void setupAdcAndFvr(void);
static void prepareSleepMode(void);
static void onWakeUp(void);

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
        bool initialMeasurementDone;
        bool measurementRunning;
    } monitoring;
} System_context = {
    .sleep = {
        .enabled = true,
        .lastWakeUpTime = 0,
        .wakeUpReason = System_WakeUpReason_None
    },
    .monitoring = {
        .lastUpdateTime = 0,
        .batteryLevel = 0,
        .initialMeasurementDone = 0,
        .measurementRunning = 0
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

    System_context.monitoring.batteryLevel = (uint8_t)(
        (uint32_t)(vbat - Config_System_VBatMinMilliVolts) * 10u
        / (Config_System_VBatMaxMilliVolts - Config_System_VBatMinMilliVolts)
    );

#if DEBUG_ENABLE_PRINT
    printf(
        "SYS:VDD=%u(ADC=%u),VBat=%u,L=%u\r\n",
        System_getVDDMilliVolts(),
        System_interruptContext.adc.result,
        System_getVBatMilliVolts(),
        System_context.monitoring.batteryLevel
    );
#endif
}

void System_init()
{
    // Initialize the LED first to indicate if any of the following steps stuck

    // GPIO: RC2 - LED
    LATCbits.LATC2 = 1;                     // Drive high
    ANSELCbits.ANSC2 = 0;                   // Pin is digital
    TRISCbits.TRISC2 = 0;                   // Pin is output

    // Oscillator
    OSCCON1bits.NOSC = 0b110;               // HFINTOSC
    OSCCON1bits.NDIV = 0;                   // Divider: 1
    OSCFRQbits.HFFRQ = 0b0100;              // 8 MHz
    OSCENbits.HFOEN = 1;                    // Enable HFINTOSC
    OSCENbits.SOSCEN = 1;                   // Enable the secondary oscillator
    while (                                 // Wait for all the oscillators
        !OSCSTAT1bits.HFOR
//        || !OSCSTAT1bits.SOR
    ) {
        continue;
    }

    // Comparator
    CM1CON1 = 0b100100;                     // CxV(N/P) is unconnected
    CM2CON1 = 0b100100;

    // GPIO: RA2 - LDO_SENSE
    ANSELAbits.ANSA2 = 0;                   // Pin is digital
    TRISAbits.TRISA2 = 1;                   // Pin is input
    WPUAbits.WPUA2 = 1;                     // Enable pull-up
    IOCANbits.IOCAN2 = 1;                   // Interrupt on negative edge
    IOCAPbits.IOCAP2 = 1;                   // Interrupt on positive edge

    // EUSART: 57600 Bps
#if DEBUG_REDIRECT_EUSART
    // GPIO: RC0 - TP1 (Debug-mode UART TX)
    ANSELCbits.ANSC0 = 0;                   // Pin is digital
    RC0PPS = 0b10100;                       // UART TX

    // GPIO: RC1 - TP2 (Debug-mode UART RX)
    ANSELCbits.ANSC1 = 0;                   // Pin is digital
    RXPPS = 0b10001;                        // RC1
    WPUCbits.WPUC1 = 1;                     // Enable weak pull-up
#else
    // GPIO: RA0 - ICSPDAT_TXD (Non-debug mode)
    ANSELAbits.ANSA0 = 0;                   // Pin is digital
    RA0PPSbits.RA0PPS = 0b10100;            // UART TX

    // GPIO: RA1 - ICSPDAT_RXD (Non-debug mode)
    ANSELAbits.ANSA1 = 0;                   // Pin is digital
    RXPPSbits.RXPPS = 0b00001;              // RA1
    WPUAbits.WPUA1 = 1;                     // Enable weak pull-up
#endif
    TX1STAbits.BRGH = 1;                    // High-speed BRG
    BAUD1CONbits.BRG16 = 1;                 // 16-bit BRG
    SP1BRGH = 0x00;                         // Actual: 57142.86 Bps (-0.79% error))
    SP1BRGL = 0x22;
    RCIF = 0;                               // Clear the RX interrupt flag
    RCIE = 1;                               // Enable the RX interrupt
    RC1STAbits.CREN = 1;                    // Enable continuous receive
    TX1STAbits.TXEN = 1;                    // Enable the transmitter
    RC1STAbits.SPEN = 1;                    // Enable the serial port

    // GPIO: RC3 - SW
    ANSELCbits.ANSC3 = 0;                   // Pin is digital
    TRISCbits.TRISC3 = 1;                   // Pin is input
    WPUCbits.WPUC3 = 1;                     // Enable weak pull-up
    IOCCNbits.IOCCN3 = 1;                   // Interrupt on negative edge

    // PWM5: RC4 - LED_EN_PWM, max PWM freq.: 10 kHz (BCR 321 limit)
    ANSELCbits.ANSC4 = 0;                   // Pin is digital
    PWMTMRSbits.P5TSEL = 0b01;              // Based on TMR2
    PR2 = 0x3F;                             // 8-bits, 7812.5 Hz @ 8 MHz
    PWM5DCHbits.PWM5DCH = 0;                // DC = 0
    PWM5DCLbits.PWM5DCL = 0;
    TMR2IF = 0;
    T2CONbits.T2CKPS = 0b01;                // 1:4
    T2CONbits.TMR2ON = 1;                   // Enable the timer
    while (!TMR2IF) {                       // Wait for the timer
        continue;
    }
    TRISCbits.TRISC4 = 0;                   // Enable the output driver
    RC4PPSbits.RC4PPS = 0b00010;            // Route PWM5 output
    PWM5CONbits.PWM5EN = 1;                 // Enable PWM

    // Timer1 : RTC (2s period)
    TMR1H = 0;
    TMR1L = 0;
    TMR1IF = 0;
    TMR1IE = 1;
    T1CON =
        (0b10 << 6)                         // TMR1CS=SOSC
        | (1 << 3)                          // T1SOSC=1
        | (1 << 2)                          // nT1SYNC=1
        | 1;                                // TMR1ON=1

    // Timer4 : fast tick timer, 25000 Hz (10 ms) @ 16 MHz
    PR4 = 0xF9;
    TMR4IF = 0;                             // Clear the interrupt flag
    TMR4IE = 1;                             // Enable the interrupt
    T4CON =
        (0b0100 << 3)                       // Postscaler: 1:5
        | (1 << 2)                          // TMR4ON=1
        | 0b10;                             // Prescaler: 1:16

    // PMD
    PMD0 =
        (1 << 1);                           // CLKRMD=1
    PMD1 =
        (1 << 7)                            // NCOMD=1
        | (1 << 6)                          // TMR6MD=1
        | (1 << 5)                          // TMR5MD=1
        | (1 << 3)                          // TMR3MD=1
        | 1                                 // TMR0MD=1
        ;
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
    IOCIE = 1;                              // Enable IOC module
    PEIE = 1;                               // Enable peripheral interrupts
    GIE = 1;                                // Enable interrupts globally

    if (System_isRunningFromBackupBattery()) {
        setupAdcAndFvr();

        measureVDD();

        // Wait for the initial measurement to avoid invalid readings after startup
        while (!System_interruptContext.adc.updated);

        updateBatteryLevel();

        System_context.monitoring.initialMeasurementDone = 1;
    }

    // Turn off the indicator LED
    LATCbits.LATC2 = 0;
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
        System_context.sleep.wakeUpReason == System_WakeUpReason_None
        && result.powerInputChanged
    ) {
        System_onWakeUp(System_WakeUpReason_PowerInputChanged);
    }

    System_interruptContext.ldoSense.updated = false;

//    if (
//        (
//            result.powerInputChanged
//            || Clock_getElapsedTicks(context.monitoring.lastUpdateTime)
//                >= Config_System_MonitoringUpdateIntervalTicks
//        )
//        && System_isRunningFromBackupBattery()
//    ) {
//        printf("Measuring Vbat\r\n");
//
//
//
//        measureVDD();
//        updateBatteryLevel();
//
//        context.monitoring.lastUpdateTime = Clock_getTicks();
//    }

    // Disable the PWM output if running on backup battery
//    if (result.powerInputChanged) {
//        if (System_isRunningFromBackupBattery()) {
//            printf("Disabling peripherals\r\n");
//
//            PWM5CONbits.PWM5EN = 0;
////
////            OSCENbits.ADOEN = 0;
////
////            ADCMD = 1;
////            FVRMD = 1;
//        } else {
//            printf("Enabling PWM\r\n");
//            PWM5CONbits.PWM5EN = 1;
//        }
//    }

    if (!System_context.sleep.enabled) {
//        Clock_Ticks elapsedSinceWakeUp
//            = Clock_getElapsedTicks(context.sleep.lastWakeUpTime);

        switch (System_context.sleep.wakeUpReason) {
            default:
                System_context.sleep.enabled = System_isRunningFromBackupBattery();
                break;

//            case System_WakeUpReason_None:
//                context.sleep.enabled = System_isRunningFromBackupBattery();
//                break;
//
//            case System_WakeUpReason_Startup:
//                if (elapsedSinceWakeUp >= Config_System_StartupAwakeLengthTicks) {
//                    context.sleep.enabled = System_isRunningFromBackupBattery();
//                }
//                break;
//
//            case System_WakeUpReason_KeyPress:
//                if (elapsedSinceWakeUp >= Config_System_KeyPressWakeUpLengthTicks) {
//                    context.sleep.enabled = System_isRunningFromBackupBattery();
//                }
//                break;
//
//            case System_WakeUpReason_PowerInputChanged:
//                if (elapsedSinceWakeUp >= Config_System_PowerInputChangeWakeUpLengthTicks) {
//                    context.sleep.enabled = System_isRunningFromBackupBattery();
//                }
//                break;
        }
    } else {
        result.action = System_TaskResult_EnterSleepMode;
    }

    return result;
}

void System_onWakeUp(const System_WakeUpReason reason)
{
    System_context.sleep.enabled = false;
    System_context.sleep.lastWakeUpTime = Clock_getTicks();
    System_context.sleep.wakeUpReason = reason;
}

inline System_WakeUpReason System_getLastWakeUpReason()
{
    return System_context.sleep.wakeUpReason;
}

System_SleepResult System_sleep()
{
#if DEBUG_ENABLE_PRINT
    printf(
        "SYS:SLP,T=%u,FT=%u,MSM=%u,UTC=%lu\r\n",
        Clock_getTicks(),
        Clock_getFastTicks(),
        Clock_getMinutesSinceMidnight(),
        Clock_getUtcEpoch()
    );
#endif

    // Disable the FVR to conserve power
//    FVRCONbits.FVREN = 0;

    System_context.sleep.wakeUpReason = System_WakeUpReason_None;

#if DEBUG_ENABLE
    _DebugState.sleeping = true;
    _DebugState.externalWakeUp = false;
    _DebugState.ldoSenseValue = IO_LDO_SENSE_GetValue();
    UI_updateDebugDisplay();
#endif

    prepareSleepMode();

    SLEEP();

    onWakeUp();

    // The next instruction will always be executed before the ISR

//    FVRCONbits.FVREN = 1;

    // Clear the flag so the next task() call can update it properly
    System_context.sleep.enabled = false;

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
    return System_context.monitoring.batteryLevel;
}

static void setupAdcAndFvr()
{
    OSCENbits.ADOEN = 1;

    ADCMD = 0;
    FVRMD = 0;

    // FVR
    FVRCONbits.ADFVR = 0b01;                // 1x gain
    FVRCONbits.FVREN = 1;                   // Enable the module
    while (!FVRCONbits.FVRRDY) {            // Wait for the module to be ready
        continue;
    }

    // ADC
    ADCON0bits.CHS = 0b111111;              // FVR
    ADCON1bits.ADFM = 1;                    // Right-justified result
    ADCON1bits.ADCS = 0b111;                // ADCRC
    ADIF = 0;                               // Clear the interrupt flag
    ADIE = 1;                               // Enable the interrupt
    ADCON0bits.ADON = 1;                    // Enable the module
}

static void prepareSleepMode()
{
//    printf("prepareSleepMode()\r\n");

    if (System_isRunningFromBackupBattery()) {
//        printf("Disabling peripherals\r\n");

        PWM5CONbits.PWM5EN = 0;

        OSCENbits.ADOEN = 0;

        ADCMD = 1;
        FVRMD = 1;
    }

    // Send out all the data before going to sleep
    while (UART1MD == 0 && TRMT == 0);
}

static void onWakeUp()
{
//    printf("onWakeUp()\r\n");

    if (System_isRunningFromBackupBattery()) {
        if (
            !System_context.monitoring.initialMeasurementDone
            || Clock_getElapsedTicks(System_context.monitoring.lastUpdateTime)
                >= Config_System_MonitoringUpdateIntervalTicks
        ) {
//            printf("Measuring Vbat\r\n");

            setupAdcAndFvr();
            measureVDD();
            // FIXME
            while (!System_interruptContext.adc.updated);
            updateBatteryLevel();

            System_context.monitoring.lastUpdateTime = Clock_getTicks();
            System_context.monitoring.initialMeasurementDone = 1;
        }
    } else{
        printf("Enabling PWM\r\n");
        PWM5CONbits.PWM5EN = 1;
    }
}

void System_prepareForSleepMode(void)
{
#if DEBUG_ENABLE_PRINT
    printf("System_prepareForSleepMode: t=%lld, ft=%d\r\n", Clock_getUtcEpoch(), Clock_getFastTicks());
#endif

    if (
        !System_context.monitoring.initialMeasurementDone
        || Clock_getElapsedTicks(System_context.monitoring.lastUpdateTime)
            >= Config_System_MonitoringUpdateIntervalTicks
    ) {
#if DEBUG_ENABLE_PRINT
        printf("System_prepareForSleepMode: starting VDD measurement\r\n");
#endif

        // Start the VDD measurement. The ADC interrupt will wake up the CPU and
        // the result will be stored.
        setupAdcAndFvr();
        measureVDD();
    } else {
        // Don't interrupt an ongoing acquisition
        if (!ADCON0bits.ADGO) {
#if DEBUG_ENABLE_PRINT
            printf("System_prepareForSleepMode: turning off ADC and FVR\r\n");
#endif

            // Disable ADC RC oscillator
            OSCENbits.ADOEN = 0;

            // Disable unused peripherals
            ADCMD = 1;
            FVRMD = 1;
        } else {
#if DEBUG_ENABLE_PRINT
            printf("System_prepareForSleepMode: ADC acquisition in progress\r\n");
#endif
        }
    }

    // Disable PWM output
    PWM5CONbits.PWM5EN = 0;

    // Send out all the data before going to sleep
    while (UART1MD == 0 && TRMT == 0);
}

void System_runTasksAfterWakeUp(void)
{
    // Wait for the oscillator to stabilize
    while (OSCCON3bits.ORDY == 0 || OSCSTAT1bits.HFOR == 0);

#if DEBUG_ENABLE_PRINT
    printf("System_runTasksAfterWakeUp: t=%lld, ft=%d\r\n", Clock_getUtcEpoch(), Clock_getFastTicks());
#endif

    if (System_interruptContext.adc.updated) {
#if DEBUG_ENABLE_PRINT
        printf("System_runTasksAfterWakeUp: VDD measurement finished\r\n");
#endif
        updateBatteryLevel();

        System_interruptContext.adc.updated = false;
        System_context.monitoring.lastUpdateTime = Clock_getTicks();
        System_context.monitoring.initialMeasurementDone = 1;
    }

    if (!System_isRunningFromBackupBattery()) {
#if DEBUG_ENABLE_PRINT
        printf("System_runTasksAfterWakeUp: enabling PWM output\r\n");
#endif
        PWM5CONbits.PWM5EN = 1;
    }
}