# RPTerm
Serial Terminal Firmware for RP2040 Boards

## Features

RPTerm will implement a serial terminal with the following features planed:

* VGA video output (30 lines of 80 characters, 256 colors)
* USB keyboard input
* Serial communication with UART on GPIO 12 & 13 (configurable baud rate)
* Support for VT-100 style commands
* Generates VT-100 style sequences for cursor keys
* Status Line (WIP)
* Configuration screen (WIP)
* SD Card interface, with send & receive files (TODO)

## Terminal Commands

The following ASCII control codes are supported:

* BEL (0x07): sounds a beep in the buzzer
* BS (0x08): move cursor to the right (stops on the first column)
* HT (0x09): moves cursor to the next tab stop (9, 17, 25, 33, 41, 49, 57, 65, 73)
* LF (0A): moves cursor down a line, scrolls up screen if at last line 
* CR (0x0D): moves cursor to the first column
* FF (0x0C): clears screen and moves cursor to home (fist column of first row)
* ESC (0x1B): start of a control sequence

The following ANSI sequences are supported:

* ESC[{n}A | Move the cursor up {n} lines
* ESC[{n}B | Move the cursor down {n} lines
* ESC[{n}C | Move the cursor forward {n} characters
* ESC[{n}D | Move the cursor backward {n} characters
* ESC[H | Move to home
* ESC[{Row};{Col}H | Move to {Col} column at {Row} row
* ESC[0K | Clear from cursor to the end of the line
* ESC[1K | Clear from the beginning of the current line to the cursor
* ESC[2K | Clear the whole line
* ESC[0J | clear screen from cursor
* ESC[1J | clear screen to cursor
* ESC[2J | Clear the screen and move the cursor to home
* ESC[3J | same as \ESC[2J
* ESC[{n}S | scroll screen up by {n} rows
* ESC[?25h | Cursor visible
* ESC[?25l | Cursor invisible
* ESC[0m | normal text (set foreground & background colors to normal)
* ESC[7m | reverse text (exchange foreground & background colors}
* ESC[{fcolor}m | Set foreground color 30=black, 31=red, 32=green, 33=yellow, 34=blue, 35=magenta, 36=cyan, 37=white)
* ESC[38;5;{color}m | Set foreground color to {color} (0 to 255)
* ESC[{bcolor}m | Set background color 40=black, 41=red, 42=green, 43=yellow, 44=blue, 45=magenta, 46=cyan, 47=white)
* ESC[48;5;{color}m | Set background color to {color} (0 to 255)
* ESC[s | Save the cursor position
* ESC[u | Move cursor to previously saved position

Sequence parameters are in decimal, if omitted zero is assumed. Where the parameter is a count {n}, zero is treated as 1. Columns and rows start at 1.

{color} is a number between 0 and 15, corresponding to black, red, green, yellow, blue, magenta, cyan, white)
 
## Key Codes

These are the sequences generated for special keys:

* F1: ESC O P
* F2: ESC O Q
* F3: ESC O R
* F4: ESC O S
* F5: ESC O T
* F6: ESC O U
* F7: ESC O V
* F8: ESC O W
* F9: ESC O X
* F10: ESC O Y
* Home: ESC [ H
* End: ESC [ K
* Up: ESC [ A
* Down: ESC [ B
* Right: ESC [ C
* Left: ESC [ D
* Del: 0x7F

## Local Functions

The following ALT letter combinations are used for local functions:

* ALT C: Enter configuration screen.
* ALT L: Changes between on-line mode and local mode (characters typed are treated as received characters).
* ALT R: Receive a file (TODO)
* ALT T: Transmit a file (TODO)

## Configuration Screen

*Work in Progress*

The configuration screen is entered by typing ALT C and left by typing ESC.

To navigate between fields, use the up and down arrows.

To change a field, use space or + to change to the next value and - the previous value.

## Credits

RPTerm is inspired and based on picoterm 
(https://github.com/RC2014Z80/picoterm) 
from Sheila Dixon (https://peacockmedia.software).

VGA output uses the wonderful picovga 
(http://www.breatharian.eu/hw/picovga/index_en.html) by Miroslav Nemecek.

Key sequences based on propeller-vt100 (https://github.com/maccasoft/propeller-vt100-terminal) by Marco Maccaferri.

## Software Development

The software was developed under Windows, with the standard Raspberry Pi Pico C/C++ SDK v1.3.0. 
tinyusb should v0.13.0. 
It should compile under other operating systems supported by the SDK.

## Hardware

Testing was done with a RP2040-Zero, but it should not be hard to get it to work on other RP2040 boards with the following pins available:

* Eight contiguous GPIOs (I am using GP0 to GP7)
* One GPIO for Composite Sync (I am usong GP8)
* One GPIO for the buzzer (if you want to support the BEL control code, I am using GP29)
* One GPIO for a status LED (optional, I am using GP11)
* UART pins (I am using UART0 at GPIO 12 & 13)

Hardware configuration is at the hwconfig.h file. The hardware directory has details for a few boards.

## Status LED

Microcontroller projects must have a blinking LED, right?

The (optional) status LED blinks to indicate that the firmware is executing. It will blink more rapidly while receiving data.

## What's done and what's missing

This is (and will be for a long time) a work in progress.

The basics are in place: vga, serial and keyboard support. 
Basic terminal emulation is working. I'm still revising/rewriting it and it needs a lot of testing.

Scrolling the screen is done by brute force. Having a table with line address will speed it up, but requires
changing assembly code in pico_vga (and hoping I will not mess up the timing).

The configuration screen is under construction.

Hardware will be update to include a SD card socket. Config will be stored in the SD card. Special keys will be used to send and receive text files from the SD.
