#define PERIOD         1000
#define SLOPE -0.00365 //Temperature slope (-3.65 mV/°C) From "48.7 TSN Characteristics" of RA4M1 User’s Manual: Hardware

float degC, vAref, vCC;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
}

void loop() {
static uint32_t last = millis();
  uint32_t now = millis();
  if(now - last >= PERIOD) {
    last = now;
    degC  = readTemp();      // Misurazione della temperatura con sensore integrato nella CPU
    vAref = readAref();      // Misurazione della tensione del pin Aref
    //vCC   = readVcc();       // Misura della tensione di alimentazione --- Lo fa già la funzione readTemp() 
   Serial.print("Temp:");Serial.print(degC, 2);Serial.print("°C");  // Temperatura di uscita in serie
   Serial.print(", ");
   Serial.print("vAref:");Serial.print(vAref, 3);Serial.print("V");  // Uscita in tensione Aref in serie
   Serial.print(", ");
   Serial.print("vCC:");Serial.print(vCC, 3);Serial.println("V");  // Uscita tensione Vcc in serie
  }

}

float readTemp(){
  uint16_t adc_data = 0;
  float mcu_temp_c = 0;
  int16_t v125;
  float v1,vs;
  vCC   = readVcc();
  // Setup ADC module //
  R_ADC0->ADCER_b.ADPRC = 0b00; // 00: 12-bit accuracy - 11: 14-bit accuracy
  R_ADC0->ADCSR = 0U;
  R_ADC0->ADANSA[0] = 0U;
  R_ADC0->ADANSA[1] = 0U;
  R_ADC0->ADSSTRT = 0xF1; // A/D Sampling State Register (1/ADCLK Sec.)
  R_ADC0->ADSSTRO = 0xF1; // A/D Sampling State Register (1/ADCLK Sec.)
  //--------------------//
  R_ADC0->ADEXICR = 0x100;     // Temperature Sensor Output A/D Conversion Select
  R_ADC0->ADHVREFCNT = 0U;      // AVCC0 - AVSS0 as Potential Reference
  R_ADC0->ADCSR_b.ADST = 1U; // Start A/D conversion
  delay(1);
  adc_data = (uint16_t)R_ADC0->ADTSDR; //ADTSDR stores Temperature Sensor Data Register
  v125 = (R_TSN->TSCDRH << 8) + R_TSN->TSCDRL;//Temperature Sensor Calibration Data Register
  v1 = 3.3f * v125 / 4095; // Voltage output by the TNS at 125 degreeC
  vs = vCC * adc_data / 4095; // Voltage output by the TNS at the time of measurement of T1 (volt)
  mcu_temp_c = (vs - v1) / SLOPE + 125;
  return mcu_temp_c;
}

float readVcc(){
  // Setup ADC module //
  R_ADC0->ADCER_b.ADPRC = 0b00; // 00: 12-bit accuracy - 11: 14-bit accuracy
  R_ADC0->ADCSR = 0U;
  R_ADC0->ADANSA[0] = 0U;
  R_ADC0->ADANSA[1] = 0U;
  R_ADC0->ADSSTRT = 0xF1; // A/D Sampling State Register (1/ADCLK Sec.)
  R_ADC0->ADSSTRO = 0xF1; // A/D Sampling State Register (1/ADCLK Sec.)
 //--------------------//
  R_ADC0->ADEXICR = 0x200;      // Internal Reference Voltage A/D Conversion Select
  R_ADC0->ADHVREFCNT = 0U;      // AVCC0 - AVSS0 as Potential Reference  
  uint16_t adc_data;
  float vCC;
  R_ADC0->ADCSR_b.ADST = 1U;
  delay(1);
  adc_data = (uint16_t)R_ADC0->ADOCDR;//A/D Internal Reference Voltage Data Register
  vCC = (AR_INTERNAL_VOLTAGE * 4095) / adc_data; //4096 12bit - 16383 14bit
  return vCC;
}

float readAref(){
  // Setup ADC module //
  R_ADC0->ADCER_b.ADPRC = 0b00; // 00: 12-bit accuracy - 11: 14-bit accuracy
  R_ADC0->ADCSR = 0U;
  R_ADC0->ADANSA[0] = 0U;
  R_ADC0->ADANSA[1] = 0U;
  R_ADC0->ADSSTRT = 0xF1; // A/D Sampling State Register (1/ADCLK Sec.)
  R_ADC0->ADSSTRO = 0xF1; // A/D Sampling State Register (1/ADCLK Sec.)
  // Setup ADC module //
  R_ADC0->ADEXICR = 0x200;      // Internal Reference Voltage A/D Conversion Select
  R_ADC0->ADHVREFCNT = 0x01;    // VREFH0 - AVSS0 as Potential Reference  
  uint16_t adc_data;
  float vCC;
  R_ADC0->ADCSR_b.ADST = 1U;
  delay(1);
  adc_data = (uint16_t)R_ADC0->ADOCDR;
  vCC = (AR_INTERNAL_VOLTAGE * 4095) / adc_data; //4096 12bit - 16383 14bit
  return vCC;
}


