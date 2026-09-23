/*Esempio di lettura del sensore di temperatura interno TSN con calcoli fatti in int32_t anzichè float
* Lo scketch funziona sia sulle R4 alimentate a 3.3V che a 5V
*/
#define PERIOD         1000

#define ADC_FACTORY_VCC_MICROVOLT                (3300000)
#define ADC_TEMPERATURE_RESOLUTION               (12U)
#define ADC_REFERENCE_CALIBRATION_TEMPERATURE    (125)

int32_t degC;
float vAref, vCC;

union {
         float float_adc_value;
         int32_t int32_adc_value;
        } adc_value;

void setup() {
  setupADC0();
  Serial.begin(115200);
  delay(2000);  
}

void loop() {
static uint32_t last = millis();
  uint32_t now = millis();
  if(now - last >= PERIOD) {
    last = now;
    degC  = readTemp();      // Misurazione della temperatura con sensore integrato nella CPU
  vAref = readAref();      // Misurazione della tensione del pin Aref (3,3 V)
  vCC   = readVcc();       // Misura della tensione Vcc (5V) --- Lo fa già la funzione readTemp()
   Serial.print("Temp:");Serial.print(degC);Serial.print("°C");  // Temperatura di uscita in serie
  Serial.print(", ");
  Serial.print("vAref:");Serial.print(vAref, 3);Serial.print("V");  // Uscita in tensione Aref in serie
  Serial.print(", ");
  Serial.print("vCC:");Serial.print(vCC, 3);Serial.println("V");  // Uscita tensione Vcc in serie
  }

}

void setupADC0(){
  R_ADC0->ADCER_b.ADPRC = 0b00; // 00: 12-bit accuracy - 11: 14-bit accuracy
  R_ADC0->ADCSR = 0U;
  R_ADC0->ADANSA[0] = 0U;
  R_ADC0->ADANSA[1] = 0U;
  R_ADC0->ADSSTRT = 0xF1; // 5.03uSec @ 48Mhz ("RA4M1 User’s Manual: Hardware" required minimun 5uSec (F0h))
  R_ADC0->ADSSTRO = 0xF1; // 5.03uSec @ 48Mhz ("RA4M1 User’s Manual: Hardware" required minimun 5uSec (F0h))
}

int32_t readTemp(){

adc_value.int32_adc_value = readVcc() * 1000; // Vcc mVolt
int32_t vCC_microvolt = adc_value.int32_adc_value * 1000; // Vcc uVolt

  R_ADC0->ADEXICR = 0x100;     // Temperature Sensor Output A/D Conversion Select
  R_ADC0->ADHVREFCNT = 0U;      // AVCC0 - AVSS0 as Potential Reference
  R_ADC0->ADCSR_b.ADST = 1U; // Start A/D conversion
  delay(5);
  uint16_t temperature_conversion_result = (uint16_t)R_ADC0->ADTSDR;//ADTSDR stores raw ADC value of Temperature Sensor Data Register
  int32_t reference_calibration_data = ((int32_t)(R_TSN->TSCDRH << 8) + R_TSN->TSCDRL);//Temperature Sensor Factory Calibration Data Register
  int32_t slope_uv_per_c = BSP_FEATURE_ADC_TSN_SLOPE;// Typical slope for the temperature sensor (-3.65 mV/degC / -3650 uV/degC). From "48.7 TSN Characteristics" of RA4M1 User’s Manual: Hardware
  int32_t v1_uv = (ADC_FACTORY_VCC_MICROVOLT >> ADC_TEMPERATURE_RESOLUTION) * reference_calibration_data;
  int32_t vs_uv = (vCC_microvolt >> ADC_TEMPERATURE_RESOLUTION) * temperature_conversion_result;
  int32_t temperature_c = (vs_uv - v1_uv) / slope_uv_per_c + ADC_REFERENCE_CALIBRATION_TEMPERATURE;
  return temperature_c;
}

float readVcc(){
  R_ADC0->ADEXICR = 0x200;      // Internal Reference Voltage A/D Conversion Select
  R_ADC0->ADHVREFCNT = 0U;      // AVCC0 - AVSS0 as Potential Reference  
  uint16_t adc_data;
  R_ADC0->ADCSR_b.ADST = 1U;
  delay(5);
  adc_data = (uint16_t)R_ADC0->ADOCDR;//A/D Internal Reference Voltage Data Register
  vCC = (AR_INTERNAL_VOLTAGE * 4096) / adc_data; //4096 12bit - 16383 14bit
  return vCC;
}

float readAref(){
  R_ADC0->ADEXICR = 0x200;      // Internal Reference Voltage A/D Conversion Select
  R_ADC0->ADHVREFCNT = 0x01;    // VREFH0 - AVSS0 as Potential Reference  
  uint16_t adc_data;
  R_ADC0->ADCSR_b.ADST = 1U;
  delay(5);
  adc_data = (uint16_t)R_ADC0->ADOCDR;
  vCC = (AR_INTERNAL_VOLTAGE * 4096) / adc_data; //4096 12bit - 16383 14bit
  return vCC;
}

