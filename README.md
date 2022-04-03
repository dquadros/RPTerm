# RPTerm
Serial Terminal Firmware for RP2040 Boards

## Features

RPTerm will implement a serial terminal with the following features planed:

* VGA video output (60 lines of 80 characters, 256 colors)
* USB keyboard input
* Serial communication with UART on GPIO 12 & 13 (configurable baud rate)
* Support for VT-100 style commands
* Generates VT-100 style sequences for cursor keys

## Credits

RPTerm is inspired and based on picoterm 
(https://github.com/RC2014Z80/picoterm) 
from Sheila Dixon (https://peacockmedia.software).

VGA output uses the wonderful picovga 
(http://www.breatharian.eu/hw/picovga/index_en.html) by Miroslav Nemecek.

## Software Development

The software was developed under Windows, with the standard Raspberry Pi Pico C/C++ SDK v1.3.0. 
tinyusb should v0.13.0. 
It should compile under other operating systems supported by the SDK.

## Hardware

Testing was done with a RP2040-Zero, but it should not be hard to get it to work on other RP2040 boards with the following pins available:

* Eight contiguous GPIOs (I am using GP0 to GP7)
* One GPIO for Composite Sync (I am usong GP8)
* One GPIO for the buzzer (if you want to support the BEL control code, I am using GP29)
* UART pins (I am using UART0 at GPIO 12 & 13)

Hardware configuration is at the hwconfig.h file. The harware directory has details for a few boards.

## What's done and what's missing

This is (and will be for a long time) a work in progress.

The basics are in place: vga, serial and keyboard support (no cursor keys yet). 

Terminal emulation is very crude (a subset of what is implemented in picoterm). I'm revising/rewriting it.

Scrolling the screen is done by brute force. Having a table with line address will speed it up, but requires
changing assembly code in pico_vga (and hopping I will not mess up the timing).

No local configuration yet. The plan is to access it with some special key combination. Still have to figure out 
where to store it: flash, external eeprom or SD card (this would also allow to save and send files, but will need
a lot of work).


