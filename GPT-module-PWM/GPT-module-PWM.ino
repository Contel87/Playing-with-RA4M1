/* PWM signal from GPT module every cycle of PWV change the state of D7 UNO R4 MINIMA*/

#include "FspTimer.h"

#define pinD7 ((volatile unsigned char  *) 0x4004085F) // ARDUINO UNO MINIMA D7 Register address (To check frequency with an oscilloscope on D7 pin)
#define ON  0x05
#define OFF 0x04
static volatile bool     pinState = false;

static FspTimer GPT;

void setup() {
  Serial.begin(9600);
  delay(2000);
setupPIN(3, 4); // Setting UNO R4 MINIMA D8 pin (P304) as GPT167 (CH7) PWM out (CTIOC7A)
setupPIN(3, 3); // Setting UNO R4 MINIMA D9 pin (P303) as GPT167 (CH7) PWM out (CTIOC7B)
 
//----- Start PWM ... CHANNEL_A if use pin D8, CHANNEL_B D9
GPT.begin_pwm(GPT_TIMER, 7, CHANNEL_A) ? Serial.println("CTIOC4A enabled as PWM") : Serial.println("Unable to start CTIOC4A"); // use standard 490Hz - 50.0DUTY_CYCLE_PERC

//--- If you need IRQ
GPT.setup_overflow_irq(12); // IRQ Priority
GPT.set_irq_callback(timer_callback);
                    
// --- Setting period 
//GPT.set_frequency(1000); // 1 Khz Hz OK
//GPT.set_period_ms(1); // Set 1 Khz OK - RAW mode
GPT.set_period_us(1000); // Set 1 Khz OK - Raw mode

// --- Setting duty cycle CHANNEL_A if use pin D8, CHANNEL_B D9
GPT.set_duty_cycle(24000, CHANNEL_A); // duty_cycle_counts = (uint32_t)(((float)_period_counts *  duty_perc) / 100.0) -- teorically 25000 at 1 Khz
}

void loop() {}


void timer_callback(timer_callback_args_t *args) {
  if(args->event == TIMER_EVENT_CYCLE_END){
  pinState? *pinD7 = OFF : *pinD7 = ON; // Change D7 pin state ... 0x04 set output LOW, 0x05 HIGH (to use oscilloscope on pin D7)
  pinState = !pinState;
  }
}

void setupPIN(uint8_t port, uint8_t pin) {
	R_PFS->PORT[port].PIN[pin].PmnPFS = (1 << R_PFS_PORT_PIN_PmnPFS_PDR_Pos)
			| (1 << R_PFS_PORT_PIN_PmnPFS_PMR_Pos)
			| (3 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos);
}

