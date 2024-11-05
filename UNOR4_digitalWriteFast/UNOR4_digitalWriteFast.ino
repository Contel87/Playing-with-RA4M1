/*
 *    Original project:https://github.com/KurtE/UNOR4-stuff/blob/main/libraries/UNOR4_digitalWriteFast/UNOR4_digitalWriteFast.h
 *    UNOR4_digitalWriteFast.h - A quick and dirty digitalWriteFast
 *    and digitalToggleFast for Arduino UNO R4.  There are better
 *    versions out there, but this good enough for my testing
 * 
 *    Permission is hereby granted, free of charge, to any person
 *    obtaining a copy of this software and associated documentation
 *    files (the "Software"), to deal in the Software without
 *    restriction, including without limitation the rights to use,
 *    copy, modify, merge, publish, distribute, sublicense, and/or sell
 *    copies of the Software, and to permit persons to whom the
 *    Software is furnished to do so, subject to the following
 *    conditions:
 * 
 *    This permission notice shall be included in all copies or 
 *    substantial portions of the Software.
 * 
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *    OTHER DEALINGS IN THE SOFTWARE.
 */


static inline void digitalWriteFast(uint8_t pin, uint8_t val) __attribute__((always_inline, unused));
//#include "digitalFast.h"
void setup() {
  Serial.begin(115200);
  Serial.println("\n\nTest");
  while (!Serial && millis() < 5000)
    ;

  pinMode(21, OUTPUT);
  uint32_t start_time = micros();
  for (int i = 0; i < 1000; i++) {
    digitalWrite(21, HIGH);
    digitalWrite(21, LOW);
  }
  uint32_t delta_time = micros() - start_time;
  Serial.print("digitalWrite: ");
  Serial.println(delta_time, DEC);

  start_time = micros();
  start_time = micros();
  for (int i = 0; i < 1000; i++) {
    digitalWriteFast(21, HIGH);
    digitalWriteFast(21, LOW);
  }

  delta_time = micros() - start_time;
  Serial.print("digitalWriteFast: ");
  Serial.println(delta_time, DEC);


  start_time = micros();
  for (int i = 0; i < 1000; i++) {
    R_PORT1->POSR = bit(2);
    R_PORT1->PORR = bit(2);
  }
  delta_time = micros() - start_time;
  Serial.print("POSR/PORR: ");
  Serial.println(delta_time, DEC);
}

void loop() {
}

static R_PORT0_Type * const port_table[] = { R_PORT0, R_PORT1, R_PORT2, R_PORT3, R_PORT4, R_PORT5, R_PORT6, R_PORT7 };// Associa ad ogni PORTA
static const uint16_t mask_table[] = { 1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, 1 << 7,
                                       1 << 8, 1 << 9, 1 << 10, 1 << 11, 1 << 12, 1 << 13, 1 << 14, 1 << 15 };
// quick and dirty digitalWriteFast
static inline void digitalWriteFast(pin_size_t pin, PinStatus val) {
  // la struttura g_pin_cfg[pin].pin si trova nel file ...\Arduino15\packages\arduino\hardware\renesas_uno\1.2.2\variants\MINIMA\variant.cpp
  uint16_t hardware_port_pin = g_pin_cfg[pin].pin; // Converte in HEX il numero del PIN della UNO R4 in numero PORTA e PIN RA4M1 (es.2 restituisce 105, cioe P105, cioe PORTA 1 PIN 5)
  //uint16_t mask = 1 << (hardware_port_pin & 0xf);
  uint16_t pin_mask = mask_table[hardware_port_pin & 0xf]; // Estrae soltanto il numero PIN (es. P105 significa port 1, pin 5 ... quindi estrae 5)
  // Gli indirizzi di memoria delle PORTE sono definiti nel file ...\Arduino15\packages\arduino\hardware\renesas_uno\1.2.2\variants\MINIMA\includes\ra\fsp\src\bsp\cmsis\Device\RENESAS\Include\R7FA4M1AB.h
  R_PORT0_Type * const portX = port_table[hardware_port_pin >> 8]; // Associa il numero della PORTA all' indirizzo di memoria della PORTA (che serve per cambiare lo stato del pin associato a quella porta)
 // I 16 bits POSR e i 16 bits PORR compongono il registro "Port Control Register 3 (PCNTR3/PORR/POSR)"
  if (val) portX->POSR = pin_mask; // POSR porta a 1 il bit PODR che serve ad avere l'uscita a 1 sul relativo pin
  else portX->PORR = pin_mask; // PORR porta a o il bit PODR che serve ad avere l'uscita a 0 sul relativo pin
}

static inline void digitalToggleFast(pin_size_t pin) {
  uint16_t hardware_port_pin = g_pin_cfg[pin].pin;
  uint16_t pin_mask = mask_table[hardware_port_pin & 0xf];
  R_PORT0_Type * const portX = port_table[hardware_port_pin >> 8];

  if (portX->PODR & pin_mask) portX->PORR = pin_mask;
  else portX->POSR = pin_mask;
}

static inline uint16_t digitalReadFast(pin_size_t pin) {
  uint16_t hardware_port_pin = g_pin_cfg[pin].pin;
  uint16_t pin_mask = mask_table[hardware_port_pin & 0xf];
  R_PORT0_Type * const portX = port_table[hardware_port_pin >> 8];

  return (portX->PIDR & pin_mask);
}

