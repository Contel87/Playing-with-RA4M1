#include "Timer_AGT_One.h"
/*
R4_FastADC_Single.ino
This sketch runs the UNO R4 ADC in single-scan mode taking readings timed by the AGT1 timer  
and writes readings in groups of 1000 to be read by a program running on a computer.

by Jack Short

This is free software; no restrictions or limitations are placed on its use.
This software is provided "as is" without warranty of any kind with regard
to its usability or suitability for any purpose.
*/

// #defines from Susan Parker's file:  susan_ra4m1_minima_register_defines.h
#define MSTP           0x40040000                                         // Module Registers
#define MSTP_MSTPCRD   ((volatile unsigned int   *)(MSTP + 0x7008))       // Module Stop Control Register D
#define MSTPD16        16                                                 // setting bit 16 enables ADC140 - 14-Bit A/D Converter Module
#define ADCBASE        0x40050000                                         // ADC Base
#define ADC140_ADCSR   ((volatile unsigned short *)(ADCBASE + 0xC000))    // A/D Control Register
#define ADCSR_ADST     15                                                 // A/D Conversion Start bit (shift 1 by this)
#define ADC140_ADANSA0 ((volatile unsigned short *)(ADCBASE + 0xC004))    // A/D Channel Select Register A0
#define ADANSA0_ANSA00 0                                                  // bit 0 (== 1) selects A/D channel AN000, which is UNO in A1
#define ADC140_ADCER   ((volatile unsigned short *)(ADCBASE + 0xC00E))    // A/D Control Extended Register
#define ADC140_ADDR00  ((volatile unsigned short *)(ADCBASE + 0xC020))    // A1 data register

#define ADST (1 << ADCSR_ADST)                              // start-conversion bit

#define CMDLEN            4							// length of a command sent to the Arduino
#define ARDCMD_NULL       0                                 // Arduino command, no command
#define ARDCMD_STARTSCAN  1                                 // Arduino command, start a scan
#define ARDCMD_SETRATE    2                                 // Arduino command, set scan rate
#define ARDCMD_TRIGTYPE   3                                 // Arduino command, set trigger type
#define ARDCMD_TRIGLVL    4                                 // Arduino command, set trigger level


//#define _12BITS 1                                         // un-comment this for 12-bit mode
// The ADC can be set to either 14-bit or 12-bit resolution; the R4 starts up with the ADC in 12-bit mode.
// 12-bit mode is slightly faster than 14-bit.  With the code below, the fastest sampling rates seen were:
//  12-bit        355 kHz
//  14-bit        318 kHz
// The Arduino documentation says that the UNO R4 defaults to 10-bit mode for compatibility with R3 code
// This seems to be a software conversion of (probably) 12-bit readings, that occurs in the code
// for analogRead().  There seems to be no way to set it in the hardware.

#define BUFF_SIZE 1000                                      // number of samples / scan, fills most of a computer screen

// in order to read any AC waveform like a sine wave, the waveform must be no more than 5 volts peak to peak,
// and there must be a zero offset of +2.5 volts on the signal so that negative parts of the wave are made positive.
// With the offset only and no signal, "zero" readings should be half of the maximum reading:
// value of a reading that represents zero volts when using a +2.5-volt offset 
//  to see sine waves and other AC waveforms
#ifdef _12BITS
#define ZERO_LVL 2048                                       // half of 2^12
#else
#define ZERO_LVL 8192                                       // half of 2^14
#endif

int nSamplingInterval = 5;                                  // sampling interval, microseconds

unsigned short buffer[BUFF_SIZE],                           // readings will go here to be sent to the compute
        r1, r2;                                             // consecutive readings for trigger operation
uint8_t *pBuff8 = (uint8_t *)buffer;                        // pointer to the buffer as an array of bytes

// software trigger
#define TRIGTYPE_NONE    0                                  // no trigger, don't wait to start reading
#define TRIGTYPE_RISING  1                                  // trigger when readings go from below to above the trigger value
#define TRIGTYPE_FALLING 2                                  // trigger when readings go from above to below the trigger value
int nTrigType = TRIGTYPE_RISING;                            // trigger type, one of the above, default to rising
unsigned short nTrigLevel = ZERO_LVL;                       // trigger level, default is to trigger when readings cross the zero value

void SetRate(int nRateCode);                                // sets sampling rate
void SetTrigType(int nTrgTyp);                              // sets trigger type
void SetTrigLevel(unsigned char *pData);                    // sets trigger level

unsigned short ReadSingle();
void timerISR();

volatile bool bIdling, bReading, bDataReady;
volatile int nCount = 0;                                   // counter for readings

void setup()
{
  Serial.begin(115200);
  while(!Serial);
  r1 = 0xFFFF;                                              // initialize trigger
  bReading = false;                                         // flag for ISR, not reading
  bIdling = true;                                           // another flag, system is idlng
// set up the AGT1 timer
  Timer1.initialize(nSamplingInterval);                     // set initial timer interval
  Timer1.attachInterrupt(timerISR);                         // install the timer ISR
  *MSTP_MSTPCRD &= (0xFFFFFFFF - (0x01 << MSTPD16));        // clears bit 16, enables ADC140 module
#ifdef _12BITS
  *ADC140_ADCER = 0;                                        // 12-bit resolution, no self-diagnosis, flush right
#else
  *ADC140_ADCER = 6;                                        // 14-bit resolution, no self-diagnosis, flush right
#endif
  *ADC140_ADANSA0 = 1 << ADANSA0_ANSA00;                    // using channel A1
  *ADC140_ADCSR &= 0x1FFF;                                  // 0001111111111111b; bits 14, 13 = 00 - sets single scan mode
}

void loop()
{
  if(bDataReady) {                                          // true when the buffer has filled
    Serial.write(pBuff8, sizeof(buffer));                   // send data to controlling program
    nCount = 0;                                             // reset count
    bDataReady = false;                                     // get new data
    bReading = false;                                       // not reading until trigger fires
    r1 = 0xFFFF;                                            // for trigger initialization
    bIdling = true;                                         // idling until ARDCMD_STARTSCAN command is received from the controller
    }
   else if(Serial.available() > 0) {                        // buffer not full, check for command from controlling program; if there is one
   unsigned char cCmdBuff[CMDLEN];
    delay(2);                                               // make sure all command bytes have been received, probably superfluous
    Serial.readBytes(cCmdBuff, CMDLEN);                     // command is always CMDLEN bytes even if less are needed
    switch(cCmdBuff[0]) {                                   // first byte is command code
      case ARDCMD_STARTSCAN:                                // command:  start a scan
        bIdling = false;                                    // start the next scan
        break;
      case ARDCMD_SETRATE:                                  // command:  set sampling rate
        SetRate(cCmdBuff[1]);                               // 2nd byte is samplig rate code
        break;
      case ARDCMD_TRIGTYPE:                                 // set the trigger type
        SetTrigType(cCmdBuff[1]);                           // 2nd byte is trigger type code
        break;
      case ARDCMD_TRIGLVL:                                  // set trigger level
        SetTrigLevel(cCmdBuff + 1);                         // 2nd byte is LSB and 3rd byte is MSB of 2-byte trigger level
        break;
      }
    }
}

unsigned short ReadSingle()
// get a single reading from the active analog port
{
  *ADC140_ADCSR |= ADST;                                    // start conversion
  while(*ADC140_ADCSR & ADST);                              // bit clears when conversion has completed
  return *ADC140_ADDR00;                                    // return the result
}

void timerISR()
// interrupt service routine for the AGT1 timer
// called by ISR in the Timer_AGT_One code
{
  // ISR in the Timer_AGT_One code did the interrupt controller reset
  if(bIdling)                                               // if idling
    return;                                                 // do nothing
  if(!bReading) {                                           // if currently not making a scan
    if(nTrigType) {                                         // if there is a trigger
      if(r1 == 0xFFFF)                                      // if this is the first interrupt of a read sequence
        r1 = ReadSingle();                                  //  initialize trigger first value
       else  {                                              // not the first interrupt in a sequnce, nRt1 has been initialized
        r2 = ReadSingle();                                  // get a second reading
        if((nTrigType == TRIGTYPE_RISING &&                 // if trigger is rising-edge
                  (r1 < nTrigLevel && r2 >= nTrigLevel)) || //  the trigger fires when nRt1 < trigger level while nR2 is >= the trigger level
           (nTrigType == TRIGTYPE_FALLING &&                // else if it is a falling-edge trigger
                  (r1 > nTrigLevel && r2 <= nTrigLevel))) { //  the trigger fires when nRt1 < trigger level while nR2 is <= the trigger level
          buffer[nCount++] = r2;                            // trigger condition met, keep the reading and advance the count
          bReading = true;                                  // trigger has fired, start taking readings
          }
         else                                               // trigger condition not met yet
          r1 = r2;                                          // second reading is now initial reading
        }
      }
    }
   else if(nCount < BUFF_SIZE) {                            // trigger has fired, until the buffer is full
    buffer[nCount++] = ReadSingle();                        // take a reading and put it in the buffer
    if(nCount == BUFF_SIZE)                                 // if the buffer has filled
      bDataReady = true;                                    // signal loop() to send data to controlling program
    }
}

void SetRate(int nRateCode)
// Set sampling rate.  Controlling program in this demo sends a code
// to select 1 of 7 "standard" sampling rates.  Alternatively, the 
// controlling program could specify a rate in microseconds; the TimerAGTOne
// code allows the rate to be set to as low as more than 250 seconds 
// (about 4 min 16 sec) per sample.  A 32-bit number is required to set a rate
// that low (250,000,000 microseconds).  Using a 16-bit rate would be
// useful most of the time, allowing as much as 65535 microseconds 
// (65.6 milliseconds) per reading.  Multi-byte values would need to be 
// sent and received as described below for SetTrigLevel()
{
static unsigned long nRates[] = { 5, 10, 20, 50, 
                                          100, 200, 500 };  // sampling rates in microseconds per sample
// samples / second:  200k, 100k, 50k, 20k, 10k, 5k, 2k
  if(nRateCode >= 0 && nRateCode <= 6) {                    // many more are possible, but only 7 rates in this demo
    Timer1.stop();
    Timer1.setPeriod(nRates[nRateCode]);
    }
}

void StopScan()
// stop scanning
{
    bIdling = true;                                         // idling, no action until until ARDCMD_STARTSCAN command is received from the controller
    bReading = false;                                       // not reading until trigger fires
    bDataReady = false;                                     // get new data
    nCount = 0;                                             // reset count
    r1 = 0xFFFF;                                            // trigger initialization
}

void SetTrigType(int nTrgTyp)
// set the trigger type
{
  StopScan();                                               // stop scanning
  if(nTrgTyp >= TRIGTYPE_NONE && 
                              nTrgTyp <= TRIGTYPE_FALLING)  // make sure code passed to this function is valid
    nTrigType = nTrgTyp;                                    // if it is, save it
}

void SetTrigLevel(unsigned char *pData)
// set trigger level, a 12- or 14-bit, 2-byte number
// From looking at the register addresses in the chip manual, it is apparent
// that the Renesas chip uses the "big endian" byte order with more significant
// bytes preceeding less significant ones in multi-byte words.  All PC's 
// (and recent Macs, I think) use the "little endian" byte order with less 
// significant bytes first.  For this reason, the controller program explicitly 
// puts the low byte of the trigger level before the high byte when sending the 
// command, and the following code will put them back together with the proper 
// byte order, no matter what the byte order is.
{
  StopScan();                                               // stop scanning
  nTrigLevel = *pData | ((unsigned short)pData[1] << 8);    // trigger level is (first byte) | (second byte << 8)
}

