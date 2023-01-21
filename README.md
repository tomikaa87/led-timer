# led-timer
Programmable timer for low voltage LED lights (e.g. christmas LED strings) with a Microchip PIC16.

**This project is in work-in-progress state**

# Features:
* Sophisticated scheduling system - you can choose from 48 individual 30 minute-length intervals to turn on the LED output
* Controllable LED brightness via PWM (Pulse Width Modulation) in 255 steps
* Dedicated LED driver circuit on the output
* Low power microcontroller - battery-friendly power consumption (LED output current not included)
  * 4 mA (screen on)
  * 0.2 mA (screen off)
  * ~6 μA (deep-sleep, only time-keeping) - more than 2 years from a standard CR2032 lithium battery
* Easy-to-use and intuitive user interface:
  * 3 buttons for changing settings, toggling the output etc.
  * OLED screen with schedule, output state, battery level indication and more
  * Each screen shows a legend for individual key functions
* Backup battery - timekeeping and changing settings work without external power

A custom-made PCB and a 3D-printable enclosure is still in development and should be published soon.

# Case

I've made a simple 3D-printable case in FreeCAD. It's a two-part design which can be assembled using M2 self-tapping screws.

![Front](/Images/case-front.png)
![Bottom-right](/Images/case-bottom-right.png)
![Top-left](/Images/case-top-left.png)
![Back-left](/Images/case-back-left.png)
![Without back cover](/Images/case-no-back.png)

# PCB

The first version of the PCB is a 4-layer design that can be manufactured for $2 at JLC PCB.
It's almost as small as the 0.96" SSD1306 OLED display module itself. Because different display modules have varying pinouts (VDD and GND swapped), the function of the first two pins are configurable via solder jumpers (JP1 and JP2).
The switch on the side can turn of the timer completely to conserve battery power while the timer is in storage.
The three buttons on the bottom are for navigation on the user interface.
The battery holder on the back side receives a standard CR2032 3V lithium cell.
The power input and LED output connectors are standard 2.54 mm pitch pin headers.

The interactive BOM can be accessed ![HERE](/Hardware/LEDTimer/bom/ibom.html)

## 3D renders of the PCB

![Front side](/Images/pcb-front.jpg)

![Front side with display](/Images/pcb-front-display.jpg)

![Back side](/Images/pcb-back.jpg)

# Images of the prototype

## Main screen
![Main screen](/Images/IMG_2650.jpg)

## Main screen functions explained
![Main screen functions](/Images/IMG_2650_legend.jpg)

## Settings screen
![Settings screen](/Images/IMG_2651.jpg)

## Scheduler settings
![Scheduler settings](/Images/IMG_2653.jpg)

## LED output brightness setting
![LED brightness setting](/Images/IMG_2654.jpg)

## Clock setting
![Clock setting](/Images/IMG_2655.jpg)

## Prototype setup
![Prototype setup](/Images/IMG_2649.jpg)
