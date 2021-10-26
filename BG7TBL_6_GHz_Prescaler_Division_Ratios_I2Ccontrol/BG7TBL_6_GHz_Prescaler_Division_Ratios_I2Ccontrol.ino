/*

  BG7TBL 6 GHz Prescaler Division Ratios with I2C Control by Bryce Cherry
  Also the basis for a programmable RF prescaler which can have an output programmed as a divided frequency output

  Requires the following libraries:
  ArduinoHWpins : https://github.com/brycecherry75/ArduinoHWpins
  ShiftX: https://github.com/brycecherry75/ShiftX
  BitFieldManipulation: https://github.com/brycecherry75/BitFieldManipulation
  SoftIIC: https://github.com/cirthix/SoftIIC

  SoftIIC requires the following modifications concerning TIMSKx/TIFRx/OCR2x/TCCR2x (other relevant Timer1/Timer2 registers are about the same):

  In SoftIIC.h:

  After #define SOFTIIC_h, add the following lines:
  #if !defined(__AVR_ATmega328P__) && !defined(__AVR_ATmega168__) && !defined(__AVR_ATmega88__) && !defined(__AVR_ATmega48__) && !defined(__AVR_ATmega8__)
  #error Unsupported microcontroller, please edit timer definitions to correspond
  #endif

  In SoftIIC.cpp:

  Change TIFR1 |= (1 << OCF1A) to the following:
  #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega48__)
  TIFR1 |= (1 << OCF1A);
  #elif defined(__AVR_ATmega328P__)
  TIFR |= (1 << OCF1A);
  #endif

  Change uint8_t SoftIIC::TimerElapsed(){if(TIFR1 & (1 << OCF1A)){TIFR1 |= (1 << OCF1A); return 1;} return 0;} to the following:
  #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega48__)
  uint8_t SoftIIC::TimerElapsed(){if(TIFR1 & (1 << OCF1A)){TIFR1 |= (1 << OCF1A); return 1;} return 0;}
  #elif defined(__AVR_ATmega8__)
  uint8_t SoftIIC::TimerElapsed(){if(TIFR & (1 << OCF1A)){TIFR |= (1 << OCF1A); return 1;} return 0;}
  #endif

  Change //  TIFR1 |= (1 << OCF1A) to the following:
  #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega48__)
  //  TIFR1 |= (1 << OCF1A);
  #elif defined(__AVR_ATmega8__)
  //  TIFR |= (1 << OCF1A);
  #endif

  Change TIMSK1/TIFR1 references (4 lines long) starting at TIMSK1  = p_TIMSK1:
  #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega48__)
  TIMSK1  = p_TIMSK1;
  TIFR1 = p_TIFR1;
  #elif defined(__AVR_ATmega8__)
  TIMSK = p_TIMSK1;
  TIFR  = p_TIFR1;
  #endif

  Change SoftIIC::GetCurrentTimer2Divider(){return TCCR2B&0b00000111;} to the following:
  #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega48__)
  uint8_t  SoftIIC::GetCurrentTimer2Divider(){return TCCR2B&0b00000111;}
  #elif defined(__AVR_ATmega8__)
  uint8_t  SoftIIC::GetCurrentTimer2Divider(){return TCCR2&0b00000111;}
  #endif

  Change TIMSK1/TIFR1 references (4 lines long) starting at TIMSK1  = p_TIMSK1:
  #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega48__)
  TIMSK1  = p_TIMSK1;
  TIFR1 = p_TIFR1;
  #elif defined(__AVR_ATmega8__)
  TIMSK = p_TIMSK1;
  TIFR  = p_TIFR1;
  #endif

  Change TIMSK1/TIFR1 references (4 lines long) starting at p_TIMSK1 = TIMSK1:
  #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega48__)
  p_TIMSK1  = TIMSK1;
  p_TIFR1   = TIFR1;
  #elif defined(__AVR_ATmega8__)
  p_TIMSK1  = TIMSK;
  p_TIFR1   = TIFR;
  #endif

  Change TIMSK1/TIFR1 references (4 lines long) starting at TIMSK1 = my_TIMSK1:
  #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega48__)
  TIMSK1  = my_TIMSK1 ;
  TIFR1   = my_TIFR1  ;
  #elif defined(__AVR_ATmega8__)
  TIMSK   = my_TIMSK1 ;
  TIFR  = my_TIFR1  ;
  #endif

  Change #ifdef SOFTIIC_DEGBUG_MODE section to the following:
  #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega48__)
  TCCR2A = 0; // turn off PWM
  TCCR2B = 0x00; // set clock scaler to 0
  OCR2A = 0;
  OCR2B = 0;
  TIMSK2 = 0 ;
  TCNT2 = 0;
  //    TCCR2B = 0x03; // set clock scaler to 8
  TCCR2B = 0x01; // set clock scaler to 1
  #elif defined(__AVR_ATmega8__)
  TCCR2= 0; // turn off PWM and set clock scaler to 0
  OCR2 = 0;
  TIMSK &= (~0b11000000);
  TCNT2 = 0;
  //    TCCR2 = 0x03; // set clock scaler to 8
  TCCR2 = 0x01; // set clock scaler to 1
  #endif

  Microcontroller is an ATmega8 and runs on its internal calibrated 8 MHz RC oscillator with PCB provision for an external crystal + load capacitors.
  The 2.7V brownout detection option needs to be programmed since this board does not have an external Reset generator.
  For adding support for ATmega8 (and certain others), visit https://github.com/MCUdude/MiniCore
  Fuse High: 11011001
  Fuse Low: 10100100

  U5 is a divide-by-2 flip-flop (most likely a TC74WH74) which is connected after the ADF4106 MUX output for regulating output duty cycle to 50%.
  At a 500000 division ratio as per the original code, positive pulses at the ADF4106 MUX output are measured at ~6.4uS wide at 10 MHz, ~120nS at 500 MHz and ~60nS at 1 GHz.

  ICSP header (JP2 - Pin 1 has a square pad - +3.3V LEVELS ONLY OR DAMAGE WILL OCCUR!) - we won't need a bootloader here:

    GND 10 9 MISO (PB4)
    GND 8  7 SCK (PB5)
    GND 6  5 /RESET (PC6 - has 10K pullup to +3.3V)
    GND 4  3 No connection
  +3.3V 2  1 MOSI (PB3)

  +3.3V current draw on this board is approximately 30 mA.
  Although the original BG7TBL program and EEPROM data has been read protected, ISP programming (without requiring switching of +12V on Reset pin) is still possible.

  Pin change interrupts are not supported on the ATmega8 although an interrupt pin for I2C is relatively easy to access by an average hobbyist unlike the I2C hardware pins for a TQFP package.

  HOW TO PROGRAM VIA I2C (100 kHz or slower) - I2C_address is 7 bit and for a Bus Pirate, I2C_address is multiplied by 2 and if Read, Bit 0 is set:
  Change register: START, I2C_write_address, A_Counter(0x00)/B_Counter_High/(0x02)/B_Counter_Low(0x04)/Prescaler_Value(0x06) (set Bit 7 to also write to EEPROM), required field bytes (MSB first - one byte for A counter and prescaler, two bytes for B counter)
  Read status (required after changing a register due to the code in the SoftIIC library; the system is not ready until the byte read by this operation has Bit 7 cleared and will only be updated when a write to the ADF4106 and/or EEPROM is complete): START, I2C_read_address, read_one_byte, STOP
  Status Byte (bit set = true) - changed on register change via I2C:
  Bit 0: Invalid command
  Bit 1: Field value for register out of range
  Bit 2: A value exceeds B value
  Bits 3-6: Reserved for future use
  Bit 7: /System Ready

  Required programming sequence which will not result in an error:
  Set the 5 high bits of B counter to 31
  Set A counter to 0
  Set the low 8 bits of B counter (0-255) to 255
  If high 5 bits of required value for B counter are 0:
  Set the low 8 bits of B counter (0-255) to the required value (must be at least 3 if the high 5 bits are to be set to 0)
  Set the high 5 bits of B counter (0-31) to the required value which is a multiple of 256 (if 0, the low 8 bits of B counter must be at least 3)
  Otherwise, perform the above two steps in reverse order
  Set A counter to required value (0-63) which must be equal to or less than the total of all 13 bits in the B counter
  Set prescaler (0-3) to the required value corresponding to 8/16/32/64

  The Status LED will quickly flash off when a register is correctly programmed and will flash off for two seconds if an incorrect programming attempt is made.

*/

#include <EEPROM.h>
#include <ArduinoHWpins.h> // obtain at https://github.com/brycecherry75/ArduinoHWpins
#include <ShiftX.h> // obtain at https://github.com/brycecherry75/ShiftX
#include <BitFieldManipulation.h> // obtain at https://github.com/brycecherry75/BitFieldManipulation
#include <SoftIIC.h> // obtain at https://github.com/cirthix/SoftIIC with aforementioned modifications

// Conflict free pins which can be easy to solder to have been chosen here
const byte SDApin = PD_3; // TQFP pin 1 - hardware interrupt is available here which is triggered by falling edge of SDA on a start condition
const byte SCLpin = PD_5; // TQFP pin 9

const byte ADF4106_CLK = PC_4; // TQFP pin 27
const byte ADF4106_DATA = PC_3; // TQFP pin 26
const byte ADF4106_LE = PC_2; // TQFP pin 25
const byte StatusLED = PD_2; // TQFP pin 32

// Origianl BG7TBL program logic analysis:
// Four bytes are sent for every LE cycle (first byte is dropped by the ADF4106 on loading the fourth byte) and CLK is idle high - my code only sends the required three bytes for each cycle.
// Sequence: Reference, N Counter, Function - the Initialization latch is not programmed although the same data cycle (not required) is repeated a short time later.

// I2C (first byte)
const byte I2C_address = 0x67; // 7 bit excluding R/_W flag

// register addresses (second byte) - the SoftIIC library will use Bit 0 as a R/_W flag
const byte A_Counter = 0x00;
const byte B_Counter_High = 0x02;
const byte B_Counter_Low = 0x04;
const byte Prescaler_Value = 0x06;

const byte WriteMask = 0x80; // A/B/prescaler - OR with register value to enable write to EEPROM
// third byte is the field value to be written to the selected register

const byte I2C_speed = 25; // for an 8 MHz AVR - the slave demo included with the SoftIIC library uses a value of 50 for a 16 MHz AVR
static uint8_t current_register_address_for_BG7TBL = 0x00;
byte command;
byte field;
bool CommandComplete = false; // all routines used by SlaveHandleTransaction must be completed as quickly as possible before writes to ADF4106 and/or EEPROM
byte StatusByte = 0;

// StatusByte
const byte InvalidCommand_mask = 0x01;
const byte OutOfRange_mask = 0x02;
const byte AexceedsB_mask = 0x04;

// EEPROM
const word A_Counter_EEPROM = 0x0000;
const word B_Counter_High_EEPROM = (A_Counter_EEPROM + 1);
const word B_Counter_Low_EEPROM = (B_Counter_High_EEPROM + 1);
const word Prescaler_Value_EEPROM = (B_Counter_Low_EEPROM + 1);

const byte ADF4106_DelayPerClockTransition = 1; // uS
const byte ADF4106_BitsUsed = 24;
const byte ADF4106_RegisterCount = 3;
uint32_t ADF4106registers[ADF4106_RegisterCount] {0x00000004, 0x00000001, 0x00000122}; // R0: Reference = 1 (not used here), R1: A/B counters, R2: Prescaler, MUXOUT = N, Charge pump floating, normal operation (no power down), normal counter operation

SoftIIC SoftWire = SoftIIC(SCLpin, SDApin, I2C_speed, true, false, false); // bools: pullups/multimaster support/timeout

void delayTimerless(word value) {
  for (word i = 0; i < value; i++) {
    delayMicroseconds(1000);
  }
}

uint8_t respond_to_address(uint8_t chipaddr) {
  if ((chipaddr >> 1) == I2C_address) {
    return 0x01;
  }
  return 0x00;
}

uint8_t respond_to_command(uint8_t commandaddr) {
  command = commandaddr;
  return 0x01;
}

uint8_t respond_to_data(uint8_t commandaddr) {
  return 0x01;
}

uint8_t get_current_register_address(uint8_t chipaddr) {
  if (chipaddr == I2C_address) {
    return current_register_address_for_BG7TBL;
  }
  return 0x00;
}

uint8_t set_current_register_address(uint8_t chipaddr, uint8_t regaddr) {
  if (chipaddr == I2C_address) {
    current_register_address_for_BG7TBL = regaddr;
  }
  return 0x00;
}

uint8_t read_iic_slave(uint8_t chipaddress, uint8_t* value) {
  *value = StatusByte;
  return 0x00;
}

uint8_t write_iic_slave(uint8_t chipaddr, uint8_t value) {
  field = value;
  CommandComplete = true; // all routines used by SlaveHandleTransaction must be completed as quickly as possible before writes to ADF4106 and/or EEPROM
  return 0x00;
}

bool WriteADF4106(byte RegisterType, byte DataToChange) {
  bool DataValid = false;
  StatusByte &= (~InvalidCommand_mask);
  StatusByte &= (~OutOfRange_mask);
  StatusByte &= (~AexceedsB_mask);
  RegisterType &= (~WriteMask);
  byte A_temp = BitFieldManipulation.ReadBF_dword(2, 6, ADF4106registers[1]);
  word B_temp = BitFieldManipulation.ReadBF_dword(8, 13, ADF4106registers[1]);
  switch (RegisterType) {
    case A_Counter:
      if (DataToChange > 63) {
        StatusByte |= OutOfRange_mask;
      }
      else if (DataToChange > B_temp) { // A counter must 63 or less and equal to or less than B counter as per ADF4106 datasheet
        StatusByte |= AexceedsB_mask;
      }
      else {
        ADF4106registers[1] = BitFieldManipulation.WriteBF_dword(2, 6, ADF4106registers[1], DataToChange);
        DataValid = true;
      }
      break;
    case B_Counter_High:
      if (DataToChange <= 31) {  // B counter must be 3-8191 and equal to or greater than A counter as per ADF4106 datasheet
        if (DataToChange == 0 && (B_temp & 0xFF) < 3) {
          StatusByte |= OutOfRange_mask;
        }
        else if (DataToChange == 0 && (B_temp & 0xFF) < A_temp) {
          StatusByte |= AexceedsB_mask;
        }
        else {
          ADF4106registers[1] = BitFieldManipulation.WriteBF_dword((8 + 8), 5, ADF4106registers[1], DataToChange);
          DataValid = true;
        }
      }
      else {
        StatusByte |= OutOfRange_mask;
      }
      break;
    case B_Counter_Low:
      if (DataToChange <= 255) {
        if (DataToChange >= 3 || B_temp > 255) {
          if (B_temp <= 255 && A_temp > DataToChange) {
            StatusByte |= AexceedsB_mask;
          }
          else {
            ADF4106registers[1] = BitFieldManipulation.WriteBF_dword(8, 8, ADF4106registers[1], DataToChange);
            DataValid = true;
          }
        }
        else {
          StatusByte |= OutOfRange_mask;
        }
      }
      else {
        StatusByte |= OutOfRange_mask;
      }
      break;
    case Prescaler_Value:
      if (DataToChange > 3) { // according to ADF4106 datasheet, 0: 8/9, 1: 16/17, 2: 32/33, 3: 64/65
        StatusByte |= OutOfRange_mask;
      }
      else {
        ADF4106registers[2] = BitFieldManipulation.WriteBF_dword(22, 2, ADF4106registers[2], DataToChange);
        DataValid = true;
      }
      break;
    default:
      StatusByte |= InvalidCommand_mask;
      break;
  }
  if (DataValid == true) { // within range and if programming A or B, A is equal to or less than B
    digitalWrite(ADF4106_CLK, LOW); // correct clock phase
    for (int RegisterCount = 0; RegisterCount < ADF4106_RegisterCount; RegisterCount++) {
      digitalWrite(ADF4106_LE, LOW);
      ShiftX.out_Dword(ADF4106_DATA, ADF4106_CLK, MSBFIRST, ADF4106_BitsUsed, ADF4106_DelayPerClockTransition, ADF4106registers[RegisterCount]);
      digitalWrite(ADF4106_LE, HIGH);
    }
    digitalWrite(StatusLED, LOW);
    delayTimerless(250);
    digitalWrite(StatusLED, HIGH);
  }
  else { // not within range or A is greater than B
    digitalWrite(StatusLED, LOW);
    delayTimerless(2000);
    digitalWrite(StatusLED, HIGH);
  }
  return DataValid;
}

void setup() {
  pinMode(ADF4106_CLK, OUTPUT);
  pinMode(ADF4106_DATA, OUTPUT);
  pinMode(ADF4106_LE, OUTPUT);
  pinMode(StatusLED, OUTPUT);
  digitalWrite(ADF4106_LE, HIGH); // idle state
  digitalWrite(ADF4106_CLK, LOW);
  digitalWrite(StatusLED, HIGH);
  byte A_Counter_temp = EEPROM.read(A_Counter_EEPROM);
  byte B_Counter_High_temp = EEPROM.read(B_Counter_High_EEPROM);
  byte B_Counter_Low_temp = EEPROM.read(B_Counter_Low_EEPROM);
  word B_Counter_temp = B_Counter_High_temp;
  B_Counter_temp *= 256;
  B_Counter_temp += B_Counter_Low_temp;
  byte Prescaler_Value_temp = EEPROM.read(Prescaler_Value_EEPROM);
  if (A_Counter_temp > 63 || B_Counter_temp < 3 || B_Counter_temp > 8191 || A_Counter_temp > B_Counter_temp || Prescaler_Value_temp > 3) { // reset to 128 (256 after divide by 2 flip-flop) as required by a Hantek HDG2000
    A_Counter_temp = 0;
    B_Counter_High_temp = 0;
    B_Counter_Low_temp = 4;
    Prescaler_Value_temp = 2; // prescaler 32/33
    EEPROM.write(A_Counter_EEPROM, A_Counter_temp);
    EEPROM.write(B_Counter_High_EEPROM, B_Counter_High_temp);
    EEPROM.write(B_Counter_Low_EEPROM, B_Counter_Low_temp);
    EEPROM.write(Prescaler_Value_EEPROM, Prescaler_Value_temp);
  }
  WriteADF4106(B_Counter_High, 31);
  WriteADF4106(A_Counter, 0);
  WriteADF4106(B_Counter_Low, 255);
  if (B_Counter_High_temp == 0) {
    WriteADF4106(B_Counter_Low, B_Counter_Low_temp);
    WriteADF4106(B_Counter_High, B_Counter_High_temp);
  }
  else {
    WriteADF4106(B_Counter_High, B_Counter_High_temp);
    WriteADF4106(B_Counter_Low, B_Counter_Low_temp);
  }
  WriteADF4106(A_Counter, A_Counter_temp);
  WriteADF4106(Prescaler_Value, Prescaler_Value_temp);
  noInterrupts();
}

void loop() {
  SoftWire.SlaveHandleTransaction(respond_to_address, respond_to_command, respond_to_data, get_current_register_address, set_current_register_address, read_iic_slave, write_iic_slave);
  if (CommandComplete == true) {
    interrupts();
    if (WriteADF4106(command, field) == true && (command & WriteMask) != 0) { // if an EEPROM write is required, only write if values are within range
      switch (command) {
        case (A_Counter | WriteMask):
          EEPROM.write(A_Counter_EEPROM, field);
          break;
        case (B_Counter_High | WriteMask):
          EEPROM.write(B_Counter_High_EEPROM, field);
          break;
        case (B_Counter_Low | WriteMask):
          EEPROM.write(B_Counter_Low_EEPROM, field);
          break;
        case (Prescaler_Value | WriteMask):
          EEPROM.write(Prescaler_Value_EEPROM, field);
          break;
      }
    }
    CommandComplete = false; // all routines used by SlaveHandleTransaction must be completed as quickly as possible before writes to ADF4106 and/or EEPROM
    noInterrupts();
  }
}
