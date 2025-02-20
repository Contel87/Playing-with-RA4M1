#include "Timer_AGT_One.h"

// modified by J. W. Short Feb. 8 2025
// changed ISR setup from using the deprecated attachEventLinkInterrupt()
// to using IRQManager::getInstance().addGenericInterrupt()

TimerAGTOne Timer1;   // preinstantiate.

uint16_t TimerAGTOne::reloadSetting = 0xFFFF;
agt_cntsrc_t TimerAGTOne::srcbits = PCLKB;
uint8_t TimerAGTOne::divbits = 0;
uint8_t TimerAGTOne::eventLinkIndex = 0;
void (*TimerAGTOne::isrCallback)() = TimerAGTOne::isrDefaultUnused;

GenericIrqCfg_t TimerAGTOne::cfgAgtIrq = 
    { FSP_INVALID_VECTOR, 0, ELC_EVENT_NONE };                    // needed initialization - compiler complains without it

void TimerAGTOne::internalCallback()
// installed ISR
{
// Reset the interrupt link and the flag for the timer
  R_ICU->IELSR[cfgAgtIrq.irq] &= ~(R_ICU_IELSR_IR_Msk);           // reset interrupt controller - JWS
  R_AGT1->AGTCR &= ~(R_AGT0_AGTCR_TUNDF_Msk);                     // reset the flag for the timer
  isrCallback();                                                  // callback in sketch, set in initialize()
}
