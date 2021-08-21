# Introduction

This program is meant to help those with faulty pedals who want to add their own hall effect sensors without changing the pedals' own sensors and be able to calibrate them at hardware level. This code is tested on Arduino Leonardo.

Using this code with an Arduino, you'll have a separate device for pedals which all proper simulators should support.

# Requirements

- 1 linear hall effect sensor for each faulty pedal.
- 1 neodymium magnet for each faulty pedal.
- Soldering tools to solder wires to the sensor(s)
- An Arduino that has built-in USB (ATmega32U4 microcontroller). Arduino Leonardo and Micro have this feature.
- Wires. phone cables with 4 wires could be used, 3 of them would be used for 1 pedal, 1 for GND, 1 for 5V, and 1 for the sensor output.
- A cable to connect the Arduino board to the computer.

# Installation

## Hardware

Solder the 3 wires to the sensor pins.
Tape around the unprotected wires to prevent them from touching metal or the pins to connect to each other.
Attach the sensor to one arm of the pedal, preferably the one that is mounted and doesn't move, and a neodymium magnet to the other arm.
It's recommended to align them in a way so that when the pedal is released, the edge of the magnet is close to the edge of the sensor and when depressed the center of the magnet is close to the center of the sensor but doesn't go past the center, or vice versa (at the edge when depressed and centers close without passing when released).
The magnet pole doesn't matter.
Make sure that the magnet doesn't slide.
At the other end, connect the wires to GND, 5V and one of A0 to A2 (starting from A0).
Make sure to connect the wires to the correct pins by looking up the sensor pins. VCC should be connected to 5V.

For example:
```
╭─────────────╮
│╲           ╱│
│ ├─────────┤ │
│ │ UGN3503 │ │
│ │         │ │
 ╲│         │╱
  ╰┬───┬───┬╯
   │   │   │
 +VCC GND OUT
 ```
Search for your own sensor model to find out which pins are which.

## Software

1. Connect the Arduino to the computer.
2. Download Arduino IDE or use the online tool coupled with the plugin.
3. Install Joystick library: https://github.com/MHeironimus/ArduinoJoystickLibrary
4. Open `pedals.ino` downloaded from this repository or just paste the code into the sketch.
5. Set the settings by changing the const variable values with all CAPS names at the start of the file and upload to the board.

Don't forget to disable the test print options before the final upload if they're enabled.
 
# Calibration

Calibration is done on start, leave the pedals released when the Arduino is starting up.
Once the "L" labeled LED starts blinking slowly, completely depress and release each pedal that is connected.
Calibration can be done as long as the LED blinks slowly, if pedals have had enough travel, after a few seconds the blink stops.
Pedals are usable in games during and after the calibration.

## Why calibrate on startup?

1. EEPROM is not used.
2. Could be more accurate as conditions might change over time.
3. Simple recalibration by pressing the reset button on the board.
