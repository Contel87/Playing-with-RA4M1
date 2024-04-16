/*--- Quando un segnale di Clock e' stabile e pronto da usare porta a 1 il suo relativo flag ---*/
/*--- (8.2.12 Renesas User's Manual: Hardware) ---*/

#define OSCSF ((volatile unsigned char *)0x4001E03C) 
uint8_t reg = *OSCSF;

void setup() {
  Serial.begin(9600);
  delay(2000);

  bool HOCO = bitRead(reg, 0); // bit 0
  Serial.print("HOCO Stabilization Flag: ");Serial.println(HOCO); //Internal Clock oscillator is ready!
  bool MOSC = bitRead(reg, 3); // bit 3
  Serial.print("MOSC Stabilization Flag: ");Serial.println(MOSC);//Main clock oscillator is ready!
  bool PLL = bitRead(reg, 5); // bit 5
  Serial.print("PLL Stabilization Flag: ");Serial.println(PLL);//PLL Clock Oscillation is ready!

bool RCKSEL = R_RTC->RCR4_b.RCKSEL;  // Check if use LOCO or Erternal 32.768Khz Osc.
  if(RCKSEL){
  Serial.println("RTC use LOCO internal oscillator");
            } else {
  Serial.println("RTC use sub-clock 32.768Khz external quartz");
            }

}

void loop() {
}
