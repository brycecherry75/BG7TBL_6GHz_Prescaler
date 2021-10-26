/*

  BG7TBL 6 GHz Prescaler Division Ratios by Bryce Cherry
  Microcontroller is an ATMEGA8 and runs on its internal calibrated 8 MHz RC oscillator with PCB provision for an external crystal + load capacitors.
  For adding support for ATMEGA8 (and certain others), visit https://github.com/MCUdude/MiniCore
  Fuse High: 11011001
  Fuse Low: 10100100

  Requires ArduinoHWpins library (which I wrote): https://github.com/brycecherry75/ArduinoHWpins

  U5 is a divide-by-2 flip-flop (most likely a TC74WH74) which is connected after the ADF4106 MUX output for regulating output duty cycle to 50%.
  At a 500000 division ratio, positive pulses at the ADF4106 MUX output are measured at ~6.4uS wide at 10 MHz, ~120nS at 500 MHz and ~60nS at 1 GHz.

  ICSP header (JP2 - Pin 1 has a square pad - +3.3V LEVELS ONLY OR DAMAGE WILL OCCUR!) - we won't need a bootloader here:

  GND 10 9 MISO (PB4)
  GND 8  7 SCK (PB5)
  GND 6  5 /RESET (PC6 - has 10K pullup to +3.3V)
  GND 4  3 No connection
  +3.3V 2  1 MOSI (PB3)

  +3.3V current draw on this board is approximately 30 mA.
  Although the original BG7TBL program and EEPROM data has been read protected, ISP programming (without requiring switching of +12V on Reset pin) is still possible.
  The common of the prescaler ratio switches is connected to GND and internal pullup resistors are used in this microcontroller for this code.

*/

#include <ArduinoHWpins.h>

const byte ADF4106_CLK = PC_4; // TQFP pin 27
const byte ADF4106_DATA = PC_3; // TQFP pin 26
const byte ADF4106_LE = PC_2; // TQFP pin 25
const byte StatusLED = PD_2; // TQFP pin 32
// A0 (TQFP pin 3) is used for an analog function (sample and hold filter on the output for monitoring duty cycle) which will not be used here

int PrescalerSW[2] = {PD_3, PD_5}; // TQFP pins 1 (Switch /Bit 0) and 9 (Switch /Bit 1) respectively - pins which can be easy to solder to have been chosen here
// Origianl BG7TBL program logic analysis:
// Four bytes are sent for every LE cycle (first byte is dropped by the ADF4106 on loading the fourth byte) and CLK is idle high - my code only sends the required three bytes for each cycle.
// Sequence: Reference, N Counter, Function - the Initialization latch is not programmed although the same data cycle (not required) is repeated a short time later.
uint32_t registers[3] = {0x00C00022, 0x001E8481, 0x00000FA0}; // Prescaler=64 and MUX=N, A=32 B=7812 (division ration 500000), Reference=1000
byte oldposition; // old frequency for comparison against switch changes
byte switch_position; // switch position
unsigned int i = 0;

void WriteRegisterADF4106(const uint32_t value) {
  digitalWrite(ADF4106_CLK, LOW); // correct clock phase
  digitalWrite(ADF4106_LE, LOW);
  for (int i = 2; i >= 0; i--) {
    shiftOut(ADF4106_DATA, ADF4106_CLK, MSBFIRST, ((value >> (8 * i)) & 0xFF));
  }
  digitalWrite(ADF4106_LE, HIGH);
}

void SetADF4106() {
  for (int i = 2; i >= 0; i--) {
    WriteRegisterADF4106(registers[i]);
  }
}

void setup() {
  pinMode(ADF4106_CLK, OUTPUT);
  pinMode(ADF4106_DATA, OUTPUT);
  pinMode(ADF4106_LE, OUTPUT);
  pinMode(StatusLED, OUTPUT);
  for (int i = 0; i < 2; i++) {
    pinMode(PrescalerSW[i], INPUT_PULLUP);
  }
  digitalWrite(ADF4106_LE, HIGH);
  digitalWrite(StatusLED, LOW);
  oldposition = 0x0E; // in a position a 0-3 switch cannot do
  delay(2000); // ensures that the ADF4106 is stable after power on
}

void loop() {
  byte switch_position = 0x00; // Clear the position byte for a new read
  for (int i = 0; i < 2; i++) {
    switch_position |= digitalRead(PrescalerSW[i]) << i;
  }
  switch_position ^= 0x03; // Invert bits 0-1 for active low inputs for switch
  switch (switch_position) {
    // Division ratio calculation: ((Prescaler * B) + A) where A is 0-63, B is 3-8191 and Prescaler is 8/16/32/64
    // With Prescaler 8, A=0 and B=3, minimum division ratio is 24.
    case 0x00: // Divide by 5000 (10000 with external flip-flop) - full range - we had a problem with the A value being higher than the B value when used with a prescaler of 32/64 which resulted in a prescaler of 33/65 and A values being ignored
      registers[1] = 0x00004E21; // A=8 B=78
      registers[0] = 0x00C00022; // Prescaler 64 and MUX=N
      break;
    case 0x01: // Divide by 500 (1000 with external flip-flop) - limited to 4.8 GHz by prescaler maximum output of 300 MHz - we had a problem with the A value being higher than the B value when used with a prescaler of 32/64 which resulted in a prescaler of 33/65 and A values being ignored
      registers[1] = 0x00001F11; // A=4 B=31
      registers[0] = 0x00400022; // Prescaler 16 and MUX=N
      break;
    case 0x02: // Divide by 50 (100 with external flip-flop) - limited to 4.8 GHz by prescaler maximum output of 300 MHz
      registers[1] = 0x00000309; // A=2 B=3
      registers[0] = 0x00400022; // Prescaler=16 and MUX=N
      break;
    case 0x03: // Divide by 25 (50 with external flip-flop) - limited to 2.4 GHz by prescaler maximum output of 300 MHz
      registers[1] = 0x00000305; // A=1 B=3
      registers[0] = 0x00000022; // Prescaler=8 and MUX=N
      break;
  }
  if (oldposition != switch_position) {
    digitalWrite(StatusLED, LOW);
    SetADF4106();
    oldposition = switch_position; // Update old frequency
    delay(500);
    digitalWrite(StatusLED, HIGH);
  }
}
