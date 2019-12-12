# Watering System #

This is a simplistic **wat**ering **sy**stem based on ATTiny85 MCU.

The components are

- Control board
- 3V-6V powered DC motor water pump with flyback diode
- 3V-5V powered capacitive soil moisture sensor with 3V output
- normally open momentary switch (SPST NO)
- 3.3V coin cell battery
- 4.5V battery (3xAA)
- enclosure

# How it works #

Momentarily closing the switch causes the currently measured value to be set as the goal value for the regulator. The MCU checks the sensor value in a loop and runs the pump if needed.
