# Watering System #

This is a simple **wat**ering **sy**stem based on ATTinyX5 MCU.

The components are

- [Control board](https://easyeda.com/Alex079/watsy-v2 "Schematic and PCB")
- 3V-6V powered DC motor water pump with flyback diode
- 3V-5V powered capacitive soil moisture sensor with 3V output
- normally open momentary switch (SPST NO)
- ~3V coin cell battery (CR2032)
- ~4.5V battery (3xAA)
- enclosure

# How it works #

Momentarily closing the switch causes the currently measured value to be set as the goal value for the regulator. The MCU checks the sensor value in a loop and runs the pump if needed.
