#include "analog.h"
#include "IRQManager.h"

#define PERIOD         1000
#define SLOPE          -0.0082  // Sensibilità del sensore di temperatura V/℃ ((vs - v1) / (24 - 125)) --> Correggere qui eventuale errore di temperatura reale/misurata (con termometro su chip)
#define T125            125     // Temperatura di fabbrica alla quale e' stato calibrato il sensore
#define VREF           1.43     // Tensione interna di riferimento (vedere datasheet)

#define PRINT_REG(BUCKET, REGISTER) \
  do {                              \
    uint32_t t = BUCKET->REGISTER;  \
    Serial.print(#REGISTER ":0x");  \
    Serial.print(t, HEX);           \
    Serial.print(" ");              \
  } while (false)

bool first = true;
float degC, vAref, vCC;


adc_status_t status;
adc_info_t adc_info;

void setup() {
  pinMode(21, OUTPUT);
  Serial.begin(115200);
  delay(2000);
  Serial.print("Registri ADC prima del setup: ");
  printADCR();
  Serial.println();
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
  ADC_Container adc(0,adc0_callback);
  uint16_t adc_data = 0; // R_ADC0->ADTSDR & 0xFFFF;
  float mcu_temp_c = 0;
  int16_t v125;
  float v1,vs;
  vCC   = readVcc();
  adc.channel_cfg.scan_mask           = ADC_MASK_TEMPERATURE; //ADEXICR
  analogReference(AR_INTERNAL);
  IRQManager::getInstance().addADCScanEnd(&adc);
  R_ADC_Open(&adc.ctrl, &adc.cfg);
  R_ADC_ScanCfg(&adc.ctrl, &adc.channel_cfg);
  R_ADC_ScanStart(&adc.ctrl); // ADCSR.ADST
  delay(1);
    R_ADC_Read (&adc.ctrl, ADC_CHANNEL_TEMPERATURE, &adc_data);
    while( ADC_STATE_SCAN_IN_PROGRESS == status.state) {
     R_ADC_StatusGet(&adc.ctrl, &status);
    }
    R_ADC_InfoGet(&adc.ctrl, &adc_info);   
    v125 = (int16_t) adc_info.calibration_data; // v125   = (R_TSN->TSCDRH << 8) + R_TSN->TSCDRL
    v1 = 3.3f * v125 / 16383; // Voltage output by the TNS at 125 degreeC
    vs = vCC * adc_data / 16383; // Voltage output by the TNS at the time of measurement of T1 (volt)
    //Serial.print("vs: ");Serial.print(vs);Serial.print(" v1: ");Serial.println(v1);
    mcu_temp_c = (vs - v1) / SLOPE + T125;
    //Serial.println(mcu_temp_c);
    R_ADC_Close(&adc.ctrl);
    return mcu_temp_c;
}

float readVcc(){
ADC_Container adc(0,adc0_callback);
adc.channel_cfg.scan_mask           = ADC_MASK_VOLT;//ADEXICR
adc.cfg_extend.adc_vref_control      = ADC_VREF_CONTROL_AVCC0_AVSS0; //ADHVREFCNT
uint16_t adc_data = 0; // R_ADC0->ADOCDR & 0xFFFF;
float vCC;
R_ADC_Open(&adc.ctrl, &adc.cfg);
R_ADC_ScanCfg(&adc.ctrl, &adc.channel_cfg);
R_ADC_ScanStart(&adc.ctrl);// ADCSR.ADST
delay(1);
R_ADC_Read (&adc.ctrl, ADC_CHANNEL_VOLT, &adc_data);
    while( ADC_STATE_SCAN_IN_PROGRESS == status.state) {
     R_ADC_StatusGet(&adc.ctrl, &status);
    }
    vCC = (VREF * 16383.0) / adc_data; //4096 12bit - 16383 14bit
    //Serial.print("Vcc: ");Serial.print(vCC);Serial.println("V");
    R_ADC_Close(&adc.ctrl);
    return vCC;
}

float readAref(){
  ADC_Container adc(0,adc0_callback);
  uint16_t adc_data = 0; // R_ADC0->ADOCDR & 0xFFFF;
  adc.channel_cfg.scan_mask           = ADC_MASK_VOLT;//ADEXICR
  adc.cfg_extend.adc_vref_control     = ADC_VREF_CONTROL_VREFH0_AVSS0;//ADHVREFCNT
  R_ADC_Open(&adc.ctrl, &adc.cfg);
  R_ADC_ScanCfg(&adc.ctrl, &adc.channel_cfg);
  R_ADC_ScanStart(&adc.ctrl);// ADCSR.ADST
  delay(1);
    R_ADC_Read (&adc.ctrl, ADC_CHANNEL_VOLT, &adc_data);
    while( ADC_STATE_SCAN_IN_PROGRESS == status.state) {
     R_ADC_StatusGet(&adc.ctrl, &status);
    }
    vAref = VREF * 16383 / adc_data;
    //Serial.println(vCC);
    R_ADC_Close(&adc.ctrl);
    return vAref;
}

static void adc0_callback (adc_callback_args_t * p_args){
  if(p_args->event == ADC_EVENT_SCAN_COMPLETE){
  digitalWrite(21, !digitalRead(21));
  }
}

void printADCR(){
  PRINT_REG(R_ADC0, ADCSR);  // A/D Control Register (ADCSR)
  PRINT_REG(R_ADC0, ADANSA[0]);//select the analog input channels for A/D conversion from AN000 to AN014 (xTSN 0)
  PRINT_REG(R_ADC0, ADANSA[1]);// A/D Channel Select Register A1 (xTSN 0)
  PRINT_REG(R_ADC0, ADANSB[0]);//select the analog input channels for A/D conversion from AN000 to AN014 in group B (xTSN 0)
  PRINT_REG(R_ADC0, ADANSB[1]);//select the analog input channels for A/D conversion from AN016 to AN025 in group B (xTSN 0)
  PRINT_REG(R_ADC0, ADADS[0]); //Only set the ADADS0 register bits when the ADCSR.ADST bit is 0.
  PRINT_REG(R_ADC0, ADADS[1]);//ADADC.AVEE bit is 0,value obtained by addition, 1, obtained the mean value of the results (Only set the ADADS1 register when the ADCSR.ADST bit is 0)
  PRINT_REG(R_ADC0, ADADC); //A/D-Converted Value Addition/Average Count Select Register (ADADC)
  PRINT_REG(R_ADC0, ADCER); //A/D Control Extended Register (ADCER)
  PRINT_REG(R_ADC0, ADSTRGR);//A/D Conversion Start Trigger Select Register (ADSTRGR)
  PRINT_REG(R_ADC0, ADEXICR);//IMportante per TSN!!
  //A/D Sampling State Register n (ADSSTRn) sets the sampling time for analog input.should be set while the ADCSR.ADST bit is 0!

 PRINT_REG(R_ADC0, ADSSTRL);//should be set while the ADCSR.ADST bit is 0!
 PRINT_REG(R_ADC0, ADSSTRT);//should be set while the ADCSR.ADST bit is 0!
 PRINT_REG(R_ADC0, ADSSTRO);//should be set while the ADCSR.ADST bit is 0!

  PRINT_REG(R_ADC0, ADDISCR);// Disable in case of:Temperature sensor,Internal reference voltage,A/D self-diagnosis.Set while the ADCSR.ADST bit is 0!
  PRINT_REG(R_ADC0, ADGSPCR);//Set while the ADCSR.ADST bit is 0!
  PRINT_REG(R_ADC0, ADCMPCR);//Set while the ADCSR.ADST bit is 0!
  PRINT_REG(R_ADC0, ADCMPANSR[0]);//Only set the CMPCHAn bits when the ADCSR.ADST bit is 0.
  PRINT_REG(R_ADC0, ADCMPANSR[1]);//Set while the ADCSR.ADST bit is 0!
  PRINT_REG(R_ADC0, ADCMPANSER);//TSN!!Set while the ADCSR.ADST bit is 0!
  PRINT_REG(R_ADC0, ADCMPLR[0]);
  PRINT_REG(R_ADC0, ADCMPLR[1]);
  PRINT_REG(R_ADC0, ADCMPLER);
  PRINT_REG(R_ADC0, ADCMPDR0);
  PRINT_REG(R_ADC0, ADCMPDR1);
  PRINT_REG(R_ADC0, ADWINLLB);
  PRINT_REG(R_ADC0, ADWINULB);
  PRINT_REG(R_ADC0, ADCMPSR[0]);
  PRINT_REG(R_ADC0, ADCMPSR[1]);
  PRINT_REG(R_ADC0, ADCMPSER); //TNS!!
  PRINT_REG(R_ADC0, ADCMPBNSR); //TNS!!Set while the ADCSR.ADST bit is 0!
  PRINT_REG(R_ADC0, ADCMPBSR); //TNS!!
  PRINT_REG(R_ADC0, ADWINMON); //TNS!!
  PRINT_REG(R_ADC0, ADHVREFCNT);// Set reference voltage
}
