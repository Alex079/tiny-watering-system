# Watering System #

This is a simplistic **wat**ering **sy**stem based on ATTiny85 MCU.

(important: this project is intended to be powered by a 3xAA battery pack and a CR2032, the MOSFETs used in this project might not be able to dissipate power from voltage-stabilized source)

The components are

- Control board
- 3V-6V powered DC motor water pump with flyback diode
- 3V-5V powered capacitive soil moisture sensor with 3V output
- normally open momentary switch (SPST NO)
- 4.5V battery
- enclosure

# How it works #

Momentarily closing the switch causes the currently measured value to be set as the goal value for the regulator. The MCU checks the sensor value in a loop and runs the pump if needed.

The exact algorithm depends on the MCU program, the sensor, and the pump.
