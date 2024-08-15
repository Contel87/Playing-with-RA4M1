/* GTP Timer (GPT7) push out PWM signal on boot CHANNEL_A (CTIOC7A) and CHANNEL_A (CTIOC7A)*/

#include "FspTimer.h"

#define pinD7 ((volatile unsigned char  *) 0x4004085F) // ARDUINO UNO MINIMA D7 Register address (To check frequency with an oscilloscope on D7 pin)
#define pinD8  ((volatile unsigned int *)(0x400408D0))
#define pinD9  ((volatile unsigned int *)(0x400408CC))
#define ON  0x05
#define OFF 0x04

static volatile bool pinState = false;


static FspTimer GPT;

void setup() {  
  //-----Enable GPT7 pins
  setupPIN(3, 4); // Setting UNO R4 MINIMA D8 pin (P304) as GPT167 (CH7) PWM out (CTIOC7A)
  setupPIN(3, 3); // Setting UNO R4 MINIMA D9 pin (P303) as GPT167 (CH7) PWM out (CTIOC7B)
                     
GPT.begin(TIMER_MODE_PWM, GPT_TIMER, 7,
                    1000, 50, timer_callback, nullptr); // 1 Khz, 50% duty cycle
                    GPT.enable_pwm_channel(CHANNEL_A); // Push out PWM to CTIOC7A ( D8 UNO R4 MINIMA)
                    GPT.enable_pwm_channel(CHANNEL_B); // Push out PWM to CTIOC7B ( D9 UNO R4 MINIMA)
                    GPT.setup_overflow_irq(12); // IRQ priority
                    GPT.open();
                    GPT.start();

}

void loop() {}




void timer_callback(timer_callback_args_t *args) {
  if(args->event == TIMER_EVENT_CYCLE_END){ // Do something every PWM cycle    
  pinState? *pinD7 = OFF : *pinD7 = ON; // Change D7 pin state ... 0x04 set output LOW, 0x05 HIGH (to use oscilloscope on pin D7)
  pinState = !pinState;
  }
}


void setupPIN(uint8_t port, uint8_t pin) {
	R_PFS->PORT[port].PIN[pin].PmnPFS = (1 << R_PFS_PORT_PIN_PmnPFS_PDR_Pos)
			| (1 << R_PFS_PORT_PIN_PmnPFS_PMR_Pos)
			| (3 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos);
}
