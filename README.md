# Watering System

This is a simple **wat**ering **sy**stem based on ATTinyX5 MCU.

The components are

- [Control board](https://easyeda.com/Alex079/watsy-v2 "Schematic and PCB")
- 3V-6V powered DC motor water pump with flyback diode
- 3V-5V powered capacitive soil moisture sensor with 3V output
- Normally open momentary switch (SPST NO)
- ~3V coin cell battery (CR2032)
- ~4.5V battery (3xAA)
- Enclosure

# Operating Mode

## Main flow

```
    ┌───────────┐        
    │   Boot    │        
    └─────┬─────┘        
    ┌─────┴─────┐        
    │   Setup   │◀──────┐ 
    └─────┬─────┘       │
    ┌─────┴─────┐ break │
┌──▶│  Measure  ├───────┤
│   └─────┬─────┘       │
│   ┌─────┴─────┐ break │
│   │   Pump    ├───────┤
│   └─────┬─────┘       │
│   ┌─────┴─────┐ break │
│   │   Idle    ├───────┘
│   └─────┬─────┘        
│  loop   │              
└─────────┘              
```
|Steps||
|-|-|
|Boot|configure peripherals|
|Setup|wait for user to set the pumping time interval|
|Measure|power the sensor, run value conversion, set the goal; can be interrupted by user|
|Pump|power the pump; can be interrupted by user or by failure check|
|Idle|conserve power; can be interrupted by user|

## Operation

At startup, the system goes into power down mode and waits for user to press the button.

When the button is pressed, a setup flow is started.
During the setup, the pump is started and the system starts counting time until the button is released.
Once the button is released, the pump is stopped, the elapsed time is saved, and the goal value reset is requested.

Measurement and pumping are done in a loop with a fixed delay.
This loop can be interrupted by user pressing the button and initiating the setup flow.
The goal reset is performed at the first measurement after the setup.
Pumping is performed when the currently measured value exceeds the goal value.
A failure condition is detected and the loop is interrupted when there is no decrease of the measured value for several measurements in a row despite pumping.

## Examples

Given fixed pumping interval there can be two cases

### OK

i.e. slowly drying soil, pumping interval is enough

```
value (• measured)
│                                
│                              • 
│        ·•·                 ··  
│      ··   ·    ·•·       ··    
│~~~~•·~~~~~~~~··~~~·~~~~·•~~~~~ ◀ goal
│  ··        ··        ··        
│··                  ··          
│                                time
└────┴────┴━━━────┴━━━────┴────┴ ◀ measure / pump
 zzzz zzzz  z zzzz  z zzzz zzzz  ◀ power down
```
### Failure

i.e. quickly drying soil, pumping interval is not enough

```
value (• measured)
│                    •           
│            •·     ·            
│    •·     ·  ·   ·             
│   ·  ·   ·      ·              
│~~·~~~~~~·~~~~~··~~~~~~~~~~~~~~ ◀ goal
│ ·     ··                       
│·                               
│                                time
└────┴━━━────┴━━━────┼────────── ◀ measure / pump
 zzzz  z zzzz  z zzzz│zzzzzzzzzz ◀ power down
                     failure

```