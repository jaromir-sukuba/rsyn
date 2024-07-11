# What is in this repository?
Here is the complete set of design files, BOM, source files, test results and other resources that I used in the process of building a simple resistance synthesizer.

# What is resistance synthesizer?
A resistance synthesizer is a device allowing the emulation of a resistance or (nearly) any value for use in control and test instrumentation. It can emulate resistive components like resistive temperature devices (thermistors, platinum sensors) or other resistive sensors, too. There are commercial instruments that do this task, but they are often way outside the average hobby budget. Therefore, I decided to build one.

This project demonstrates a simple resistance synthesizer device built around a high-precision digital-to-analog converter (DAC) that is controlled by a microcontroller, allowing communication with a computer to control and calibrate the instrument, allowing integration into automated test setups. As standard in test equipment, the analog portion of the instrument is isolated from the digital portion, which makes the integration much easier as the synthesizer doesn't suffer from ground loops and is insulated from potentially dangerous voltage levels. Apart from control via USB, the instrument contains a local user interface consisting of a keypad and display.
![enter image description here](https://github.com/jaromir-sukuba/rsyn/blob/main/media/IMG_20230515_151439193.jpg)

# How does resistance synthesis work?
Reistance synthesis can be done in numerous ways. The obvious one is a network of resistors and relays to switch the desired value. The number of relays and resistors can be minimized by varying exponential values and switching in binary code patterns. That would employ resistor values of 1R-2R-4R-8R-16,  etc., and an appropriate switching pattern allowing the resistor to be included or excluded from the circuit. This approach is quite straightforward but has a few issues. Contact resistance is included in the resistor value and can matter, especially on the low end of the range (unless a four-wire arrangement is used, which can solve most of the contact problems); the contact resistance may change over the relay lifetime and other environmental factors. Furthermore, switching takes time, may be acoustically noisy, and causes relay contact wear.
More elegant is the electrical resistance adjustment. Digital potentiometers may be adequate for many uses, but they usually suffer from low resolution and low precision. While those can be a perfect fit for many applications, my idea of a resistance synthesizer requires somehow a more nuanced approach.
Fortunately, there are other ways to emulate the resistance, reducing the problem to current sensing and voltage generation, both of which can be done relatively easily and with much higher precision than the digital potentiometer can offer.
The principle is simple and this picture demonstrates this approach. 
![enter image description here](https://github.com/jaromir-sukuba/rsyn/blob/main/media/ss1b.png)

The left side of the picture captures voltage and current in a simple circuit with a resistor, following Ohm's law. Should we include voltage source U in series with the resistor, the apparent resistance of the circuit changes, following the formula on the right side of the picture. If the Us is derived from the current Ir, the resulting resistance value is invariant of the current Ir, so that the circuit behaves like a resistor with apparent resistance Rs between zero (Us = Ir x Rr) and Rr (Us = 0). 

Second picture depicts this idealized schematic in a way that somehow resembles the circuit in my resistance synthesizer. 
![enter image description here](https://github.com/jaromir-sukuba/rsyn/blob/main/media/ss2.PNG)
The voltage on Rr is brought to a voltage follower (so that Ir isn't influenced by following circuits) and subsequently to a potentiometer, allowing it to regulate the synthesized resistance value. This circuit has the advantage of a potentiometer acting like a grounded voltage divider, so it can be substituted by a multiplying DAC, in this case LTC2756.

# Instrument overview
Schematic sheet 'Interconnect' depicts top-level schematics and internal instrument wiring.
![enter image description here](https://github.com/jaromir-sukuba/rsyn/blob/main/media/block_diagram.png)

## Resistance synthesizer board
Reference resistors R19 and R29/30 are switched using DG442 analog switches U11C and U11D. There are options to change the gain of the reference resistor amplifier (R20, R24, R28, and U7B, U11A, and U11B) and DAC amplifier (R8, R9, R10, and switches U7A, U7C, and U7D), but those aren't used in the current firmware, so that U7A and U7B are switched continuously.
U4 is the heart of the resistance synthesizer board. It works in a more-or-less datasheet configuration, with one interesting point: its reference voltage is switchable by zero-ohm resistors R2 and R3 (only one is installed). When R2 is populated, the output works in the range 0..5 V and is useful for testing the DAC. In this mode, the circuit doesn't work as a resistance synthesizer. When R3 is installed instead of R2, the reference voltage is derived from the resistance drop on a reference resistor (refer to Picture #2), and the circuit works as a resistance synthesizer.
The remaining parts of the PCB are just support functions for the DAC. Microcontroller U10 acts as a UART-DAC bridge, converting ASCII commands from serial lines to DAC SPI control packets and analog switch actions. U3 acts as a digital isolator, allowing the digital and analog boards to be separated electrically.

## Display board
Three SCT2024 shift registers with constant current drivers are chained into a 48-bit-long string. Six pieces of dual starburst LED displays are multiplexed in a 1:4 multiplex ratio by the microcontroller U7. This microcontroller communicates via serial line and transforms single ASCII commands into text readable on the display.

## Digital and PSU boards
U5 on this board acts as the main controlling element of the device. It talks to internal microcontrollers (on the resistance synthesizer board and display board) as well as external communication channels via USB. Apart from that, it reads the keyboard to allow direct user control. The majority of the complexity of this part lies in firmware, available on GitHub.
PSU is relatively straight-forward. Both analog and digital sections are fed from separate transformer windings, each with a diode rectifier and voltage regulator. In the case of the digital section, I opted for a Murata OKI-78-SR-33 switching regulator to cut down on heat generated in the PSU.

# Physical construction
Resistance synthesizers and display boards were designed as 4-layer PCBs in KiCAD. Digital PSU and keypad boards were simple enough to be made on a perfboard.
All electrical components are mounted into a CP-15-32 enclosure with custom milled front and back panels. Construction is obvious from the photos.
![enter image description here](https://github.com/jaromir-sukuba/rsyn/blob/main/media/IMG_20230515_151719835.jpg)

# Instrument operation
It is operated using either a local interface or serial commands. For the local interface, keys 1 and 2 select the digit to be modified, and keys 3 and 4 perform incrementation or decrementation of the digit. The digit to be modified blinks for approximately 4 seconds. Pressing keys 1 and 2 simultaneously changes the range (10 and 100 kOhm ranges).
For the serial interface, the following commands are implemented in current firmware:

### idn
returns instrument identification
PC->Instrument: idn<LF>
Instrument->PC: RSYN Ver 1.00<LF>

### srv
set resistance value
PC->Instrument: srv 1.234LF>
Instrument->PC: srv 1.234LF>

### grv
set resistance value
PC->Instrument: grv<LF>
Instrument->PC: grv 1.234LF>

### srr
set resistance range (0: 0-10 kOhm; 1: 0-100 kOhm)
PC->Instrument: srr 1LF>
Instrument->PC: srr 1LF>

### grr
Get the resistance range (0: 0-10 kOhm; 1: 0-100 kOhm).
PC->Instrument: grr<LF>
Instrument->PC: grr 1LF>

### scc 
Set the calibration coefficient (the first argument is the coefficient number, the second is the coefficient).
PC->Instrument: scc 2, 3.456LF>
Instrument->PC: scc 2, 3.456LF>

### gcc 
Get the calibration coefficient (the argument is the coefficient number).
PC->Instrument: gcc 2LF>
Instrument->PC: gcc 2, 3.456LF>

### gcd
get a calibration dump
PC->Instrument: gcd<LF>
Instrument->PC: gcc [followed by calibration constants on each line] <LF>

### wmc
write calibration coefficient (into EEPROM memory; previous calibration is overwritten)
PC->Instrument: wcc<LF>
Instrument->PC: wcc<LF>

After first powering up, default calibration constants 0 and 12345 are loaded for offset and gain, respectively. The user needs to replace those constants with valid ones obtained by calibration. For calibration, a ohmmeter, usually in a form of multimeter has to be used. The precision of the resistance synthesizer is determined by he meter used for adjustment. On the other hand, thermal and long time drift is given by design and construction of the device.
The calibration procedure is as follows:
1. Turn on the calibration meter and resistance synthesizer, connect the appropriate leads, and let them warm up (typical precision meters do have a few hours of warmup required).
2, set the meter to the 10 kOhm manual range, set the synthesizer to the 10 kOhm range, and set the synthesizer to 50 kOhm. Adjust the offset to range 0 so that the meter correctly displays 50 ohms.
3. Set the synthesizer to 10 kOhm. Adjust gain at range 0 so that the meter correctly displays 10 kOhm.
4. Repeat steps 2 and 3 if needed.
5, do the same as step 2 on 100 kOhm ranges and adjust offset at range 1.
6. Do the same as step 3, with a 100 kOhm value, and adjust gain at range 1.
7. Repeat steps 4 and 5 if needed.
8. Issue the wmc command to overwrite previous calibration values in the EEPROM.


## Verification
For parameter verification, I performed two tests: a linearity check and short-term stability. 
![enter image description here](https://github.com/jaromir-sukuba/rsyn/blob/main/media/ss3-lin.png)
For the linearity test, I set the values of the DAC converter to values from 0 to 262143 in increments of 32768. I measured the resitance values using an HP34401A multimeter and calculated the difference from the linear fit of the values, expressed in the least significant digits of the value. As apparent from the graph, the typical deviation from linearity is around 0.2 LSB, indicating that the used DAC has very good linearity, and apart from that, linearity isn't degraded by the analog functionality (chosen opamps and reference resistors).
Short-term stability: I measured the value of resistance set to 5 kOhm over the course of one hour. The value was stable within the 20 mOhm range. 
![enter image description here](https://github.com/jaromir-sukuba/rsyn/blob/main/media/ss4-stab.png)

# Resources
A video showing the synthesizer in operation is at https://youtu.be/5EVBY4_3-oE

# Resume
This resistance synthesizer is not a very common target of DIY designs, but nonetheless, it's a useful tool for automatic calibration of test and measurement instruments or as a general-purpose tool for testing and adjusting multimeters, controllers, and similar apparatus.

