cmake_minimum_required(VERSION 3.3)

# set up the Microchip cross toolchain
set(CMAKE_TOOLCHAIN_FILE cmake-microchip/toolchain.cmake)

# set the default MCU model
set(MICROCHIP_MCU PIC16F18326)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

project(led-timer C)

add_definitions(
    -DDEBUG_ENABLE_PRINT=0
    -DDEBUG_ENABLE=0
    -DSUNRISE_SUNSET_USE_LUT=1
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=16F18326 -c -mdfp=\"${DFP_DIR}/xc8\" -fshort-double -fshort-float -O3 -fasmfile -maddrqual=ignore -xassembler-with-cpp -mwarn=-3 -Wa,-a -msummary=+psect,+class,+mem,-hex,-file -ginhx32 -Wl,--data-init -mno-keep-startup -mno-osccal -mno-resetbits -mno-save-resetbits -mno-download -mno-stackcall -mdefault-config-bits -mc90lib -gdwarf-3 -mstack=compiled:auto:auto")

add_library(mcc-generated STATIC 
    mcc_generated_files/adc.c
    mcc_generated_files/adc.h
    mcc_generated_files/device_config.c
    mcc_generated_files/device_config.h
    mcc_generated_files/docs
    mcc_generated_files/eusart.c
    mcc_generated_files/eusart.h
    mcc_generated_files/fvr.c
    mcc_generated_files/fvr.h
    mcc_generated_files/i2c1_master.c
    mcc_generated_files/i2c1_master.h
    mcc_generated_files/interrupt_manager.c
    mcc_generated_files/interrupt_manager.h
    mcc_generated_files/mcc.c
    mcc_generated_files/mcc.h
    mcc_generated_files/memory.c
    mcc_generated_files/memory.h
    mcc_generated_files/pin_manager.c
    mcc_generated_files/pin_manager.h
    mcc_generated_files/pwm5.c
    mcc_generated_files/pwm5.h
    mcc_generated_files/tmr1.c
    mcc_generated_files/tmr1.h
    mcc_generated_files/tmr2.c
    mcc_generated_files/tmr2.h
    mcc_generated_files/tmr4.c
    mcc_generated_files/tmr4.h
)

add_executable(led-timer
    Clock.c
    Clock.h
    Config.h
    Graphics.c
    Graphics.h
    Keypad.c
    Keypad.h
    main.c
    MainScreen.c
    MainScreen.h
    Makefile
    OutputController.c
    OutputController.h
    SSD1306.c
    SSD1306.h
    Settings.c
    Settings.h
    SettingsScreen_DST.c
    SettingsScreen_DST.h
    SettingsScreen_Date.c
    SettingsScreen_Date.h
    SettingsScreen_DisplayBrightness.c
    SettingsScreen_DisplayBrightness.h
    SettingsScreen_LEDBrightness.c
    SettingsScreen_LEDBrightness.h
    SettingsScreen_Location.c
    SettingsScreen_Location.h
    SettingsScreen_Scheduler.c
    SettingsScreen_Scheduler.h
    SettingsScreen_SegmentScheduler.c
    SettingsScreen_SegmentScheduler.h
    SettingsScreen_Time.c
    SettingsScreen_Time.h
    SettingsScreen_TimeZone.c
    SettingsScreen_TimeZone.h
    Settings_MenuScreen.c
    Settings_MenuScreen.h
    SunriseSunsetLUT.c
    SunsetSunrise.c
    SunsetSunrise.h
    System.c
    System.h
    Text.c
    Text.h
    Types.c
    Types.h
    UI.c
    UI.h
    Utils.c
    Utils.h
)

target_link_libraries(led-timer
    PRIVATE
        mcc-generated
)