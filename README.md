# BG7TBL_6GHz_Prescaler

Programmable division ratios for BG7TBL 6 GHz RF prescaler

## Revisions:

v1.0.0 First release

## Features

+ Programmable division ratios of 24 to 524287 (double after the divide-by-2 flip flop) whereas the original BG7TBL program only has a fixed division ratio of 10 ^ 6
+ Can be programmed via I2C (up to 100 kHz) using easy to access pins
+ Power-on defaults can be stored in EEPROM

## How to use

Instructions are in each .ino sketch which includes required modifications to the SoftIIC library.

Requires the following libraries:

ArduinoHWpins.h: https://github.com/brycecherry75/ArduinoHWpins

ShiftX.h: https://github.com/brycecherry75/ShiftX

BitFieldManipulation: https://github.com/brycecherry75/BitFieldManipulation

SoftIIC: https://github.com/cirthix/SoftIIC with modifications as per I2Ccontrol sketch

The division ratio is: ((Prescaler * B Counter) + A Counter) where Prescaler is 8/16/32/64 and will be double after the divide-by-2 flip-flop e.g. a programmed division ratio of 128 will be 256 after the flip-flop.

B Counter must be 3-8191, A Counter must be 0-63 and must be less than or equal to B Counter and as per ADF4106 datasheet, Prescaler output must not exceed 300 MHz (e.g. 2.4 GHz RF input is the limit with a prescaler programmed to 8/9).

The circuit is in a PDF file and the schematic file is in Eagle format.