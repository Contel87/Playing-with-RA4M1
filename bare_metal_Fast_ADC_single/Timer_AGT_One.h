#ifndef TIMER_AGT_ONE_H
#define TIMER_AGT_ONE_H

#include "Arduino.h"
#include "IRQManager.h"

// origin of this file is forgotten
// modified by J. W. Short Feb. 8 2025
// changed ISR setup from using the deprecated attachEventLinkInterrupt()
// to using IRQManager::getInstance().addGenericInterrupt()

#define AGT1_RESOLUTION 0xFFFF                              // AGT1 is a 16-bit timer

typedef enum agt_opmode_t {                                 // timer operating mode, values defind on page 514
  TIMER,                                                    // timer mode
  PULSE_OUTPUT,                                             // pulse output mode
  EVENT_COUNTER,                                            // event counter mode
  PULSE_WIDTH,                                              // pulse width measurement mode
  PULSE_PERIOD                                              // pulse period measurement mode
};

typedef enum agt_cntsrc_t {                                 // source clock for timer
  PCLKB,                                                    // PCLKB
  PCLKB_8,                                                  // PCLKB / 8
  PCLKB_2 = 3,                                              // PCLKB / 2
  AGTLCLK,                                                  // divided clock specified by AGTLCLK
  AGT0_UF,                                                  // AGT0 underflow
  AGTSCLK                                                   // divided clock specified by AGTSCLK
};

// most of the comments below are by the original author

class TimerAGTOne {
private:
  static agt_cntsrc_t srcbits;
  static uint8_t divbits;
  static uint16_t reloadSetting;
  static uint8_t eventLinkIndex;
  static GenericIrqCfg_t cfgAgtIrq;

  static void internalCallback();                           // added by JWS

public:

  static void (*isrCallback)();                             // ISR in Timer_AGT_One.cpp, comment by JWS
  static void isrDefaultUnused(){};                         // default ISR callback, does nothing, replaced by internalCallback()  comment by JWS

  void initialize(unsigned long microseconds = 1000000) __attribute__((always_inline)) {
    // enable the timer in Module Stop Control Register D
    R_MSTP->MSTPCRD &= ~(1 << R_MSTP_MSTPCRD_MSTPD2_Pos);
    // We're using R_AGT1, but all the positions and bitmasks are defined as R_AGT0
    // set mode register 1
    //(-) (TCK[2:0]) (TEDGPL) (TMOD[2:0])
    //  Use TIMER mode with the LOCO clock (best we can do since Arduino doesn't have crystal for SOSC)
    R_AGT1->AGTMR1 = (AGTLCLK << R_AGT0_AGTMR1_TCK_Pos) | (TIMER << R_AGT0_AGTMR1_TMOD_Pos);
    // mode register 2
    // (LPM) (----) (CKS[2:0])
    R_AGT1->AGTMR2 = 0;
    // AGT I/O Control Register
    // (TIOGT[1:0]) (TIPF[1:0]) (-) (TOE) (-) (TEDGSEL)
    R_AGT1->AGTIOC = 0;
    // Event Pin Select Register
    // (-----) (EEPS) (--)
    R_AGT1->AGTISR = 0;
    // AGT Compare Match Function Select Register
    // (-) (TOPOLB) (TOEB) (TCMEB) (-) (TOPOLA) (TOEA) (TCMEA)
    R_AGT1->AGTCMSR = 0;
    // AGT Pin Select Register
    // (---) (TIES) (--) (SEL[1:0])
    R_AGT1->AGTIOSEL = 0;

    // AGT1_AGTI is the underflow event
    //  The event code is 0x21

// change made here - JWS
    cfgAgtIrq.irq = FSP_INVALID_VECTOR;                     // set up structure
    cfgAgtIrq.ipl = 12;                                     // priority level
    cfgAgtIrq.event = ELC_EVENT_AGT1_INT;                   // AGT1 interrupt
    IRQManager::getInstance().addGenericInterrupt(cfgAgtIrq, 
                                        internalCallback);  // attach our ISR
    setPeriod(microseconds);                                // set rate
  }

  void setPeriod(unsigned long microseconds) __attribute__((always_inline)) {

    // for smal periods we can use PCLKB instead of LOCO
    divbits = 0;  // No divider bits on PCLKB
    // PCLKB is running at SYSCLK/2 or 24MHz or 24 ticks per microsecond
    unsigned long ticks = (24 * microseconds);
    if (ticks < AGT1_RESOLUTION) {
      srcbits = PCLKB;
      reloadSetting = ticks;
    } else if (ticks < AGT1_RESOLUTION * 2) {
      srcbits = PCLKB_2;
      reloadSetting = ticks / 2;
    } else if (ticks < AGT1_RESOLUTION * 8) {
      srcbits = PCLKB_8;
      reloadSetting = ticks / 8;
    } else {
      //  Period is too long for PCLKB, use AGTLCLK (LOCO)
      // LOCO is 32.768KHz  is (1/32768) = 30.518us/tick
      srcbits = AGTLCLK;
      // recalculate ticks at new clock speed
      ticks = microseconds / (1000000.0 / 32768.0);
      if (ticks < AGT1_RESOLUTION) {
        divbits = 0;
        reloadSetting = ticks;
      } else if (ticks < AGT1_RESOLUTION * 2) {
        divbits = 1;
        reloadSetting = ticks / 2;
      } else if (ticks < AGT1_RESOLUTION * 4) {
        divbits = 2;
        reloadSetting = ticks / 4;
      } else if (ticks < AGT1_RESOLUTION * 8) {
        divbits = 3;
        reloadSetting = ticks / 8;
      } else if (ticks < AGT1_RESOLUTION * 16) {
        divbits = 4;
        reloadSetting = ticks / 16;
      } else if (ticks < AGT1_RESOLUTION * 32) {
        divbits = 5;
        reloadSetting = ticks / 32;
      } else if (ticks < AGT1_RESOLUTION * 64) {
        divbits = 6;
        reloadSetting = ticks / 64;
      } else if (ticks < AGT1_RESOLUTION * 128) {
        divbits = 7;
        reloadSetting = ticks / 128;
      }
    }
    //  Use TIMER mode with the LOCO clock (best we can do since Arduino doesn't have crystal for SOSC)
    R_AGT1->AGTMR1 = (srcbits << R_AGT0_AGTMR1_TCK_Pos) | (TIMER << R_AGT0_AGTMR1_TMOD_Pos);
    R_AGT1->AGTMR2 = divbits;
    R_AGT1->AGT = reloadSetting;
    start();
  }

  void start() __attribute__((always_inline)) {
    resume();
  }
  void stop() __attribute__((always_inline)) {
    R_AGT1->AGTCR = 0;
  }
  void restart() __attribute__((always_inline)) {
    start();
  }
  void resume() __attribute__((always_inline)) {
    R_AGT1->AGTCR = 1;
  }

  void attachInterrupt(void (*isr)()) __attribute__((always_inline)) {
    isrCallback = isr;
  }
  void attachInterrupt(void (*isr)(), unsigned long microseconds) __attribute__((always_inline)) {
    if (microseconds > 0) setPeriod(microseconds);
    attachInterrupt(isr);
  }
  void detachInterrupt() __attribute__((always_inline)){
    isrCallback = isrDefaultUnused;
  }
};

extern TimerAGTOne Timer1;

#endif  //TIMER_AGT1_H
