# led-timer
Programmable timer for low voltage LED lights (e.g. christmas LED strings) with a Microchip PIC16

**This project is in work-in-progress state**

# Features:
* Sophisticated scheduling system - you can choose from 48 individual 30 minute-length intervals to turn on the LED output
* Controllable LED brightness via PWM (Pulse Width Modulation) in 255 steps
* Dedicated LED driver circuit on the output
* Low power microcontroller - battery-friendly power consumption (LED output current not included)
  * 4 mA (screen on)
  * 0.2 mA (screen off)
  * 41 μA (deep-sleep, only time-keeping) - more than 4000 hours from a standard CR2032 lithium battery
* Easy-to-use and intuitive user interface:
  * 3 buttons for changing settings, toggling the output etc.
  * OLED screen with schedule, output state, battery level indication and more
  * Each screen shows a legend for individual key functions
* Backup battery - timekeeping and changing settings work without external power

A custom-made PCB and a 3D-printable enclosure is still in development and should be published soon.

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
