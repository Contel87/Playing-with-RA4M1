

/* Stampa diretta del valore di un registro del RA4M1*/

#define regMSTPCRC   (volatile unsigned short *) 0x40047004
uint32_t reg = *regMSTPCRC; // Se il registro è lungo 8bit dichiarare uint8_t

void setup() {
  Serial.begin(9600);
  delay(2000);
  Serial.println(reg, BIN); 
}

void loop() {
}
