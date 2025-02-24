
#include "r_adc.h"

#define PERIOD         1000
#define SLOPE          -0.0034  // Sensibilità del sensore di temperatura V/℃ ((vs - v1) / (24 - 125)) --> Correggere qui eventuale errore di temperatura reale/misurata (con termometro su chip)
#define T125             125     // Correggere qui eventuale differenza di temperatura reale/misurata sul RA4M1
#define VREF           1.45    // Tensione interna di riferimento (vedere datasheet)

adc_instance_ctrl_t g_adc_ctrl;
adc_cfg_t g_adc_cfg;
adc_extended_cfg_t g_adc_extended_cfg_t;
adc_status_t status;
adc_info_t adc_info;
adc_channel_cfg_t g_adc_channel_cfg;

float degC, vAref, vCC;

void setup() {
  pinMode(21,OUTPUT);
  Serial.begin(9600);
  delay(2000);
}

void loop() {
  static uint32_t last = millis();
  uint32_t now = millis();
  if(now - last >= 1000) {
    last = now; 
 digitalWrite(21, HIGH);  // LED indicatore di funzionamento acceso
  degC  = readTemp();      // Misurazione della temperatura con sensore integrato nella CPU
  vAref = readAref();      // Misurazione della tensione del pin Aref (3,3 V)
  //vCC   = readVcc();       // Misura della tensione Vcc (5V) --- Lo fa già la funzione readTemp()
  digitalWrite(21, LOW);   // LED off
 
  Serial.print("Temp:");Serial.print(degC, 2);Serial.print("°C");  // Temperatura di uscita in serie
  Serial.print(", ");
  Serial.print("vAref:");Serial.print(vAref, 3);Serial.print("V");  // Uscita in tensione Aref in serie
  Serial.print(", ");
  Serial.print("vCC:");Serial.print(vCC, 3);Serial.println("V");  // Uscita tensione Vcc in serie
  }
}

void initializeADC(void)
{
R_ADC_Open(&g_adc_ctrl, &g_adc_cfg);
R_ADC_ScanCfg(&g_adc_ctrl, &g_adc_channel_cfg);
R_ADC_ScanStart(&g_adc_ctrl);
}


float readTemp(){
  uint16_t adc_data = 0; // R_ADC0->ADTSDR & 0xFFFF;
  float mcu_temp_c = 0;
  int16_t v125;
  float v1,vs;
  vCC   = readVcc();
  analogReference(AR_INTERNAL);
  //g_adc_extended_cfg_t.adc_vref_control = ADC_VREF_CONTROL_AVCC0_AVSS0;
  g_adc_channel_cfg.scan_mask           = ADC_MASK_TEMPERATURE; //ADEXICR
  //g_adc_cfg.mode                        = ADC_MODE_SINGLE_SCAN;     //ADCSR
  //g_adc_cfg.trigger                     = ADC_TRIGGER_SOFTWARE; //ADCSR - ADSTRGR
  //g_adc_extended_cfg_t.clearing         = ADC_CLEAR_AFTER_READ_ON; //ADCER.ACE
  g_adc_cfg.p_extend = &g_adc_extended_cfg_t;
  initializeADC();
  delay(1);
    //R_ADC_ScanStart(&g_adc_ctrl);
    R_ADC_Read (&g_adc_ctrl, ADC_CHANNEL_TEMPERATURE, &adc_data);
    while( ADC_STATE_SCAN_IN_PROGRESS == status.state) {
     R_ADC_StatusGet(&g_adc_ctrl, &status);
    }
    R_ADC_InfoGet(&g_adc_ctrl, &adc_info);   
    v125 = (int16_t) adc_info.calibration_data; // v125   = (R_TSN->TSCDRH << 8) + R_TSN->TSCDRL
    v1 = 3.3f * v125 / 4096; // Voltage output by the TNS at 125 degreeC
    vs = vCC * adc_data / 4096; // Voltage output by the TNS at the time of measurement of T1 (volt)
    mcu_temp_c = (vs - v1) / SLOPE + T125;
    //Serial.println(mcu_temp_c);
    return mcu_temp_c;
}

float readAref(){
  uint16_t adc_data = 0; // R_ADC0->ADTSDR & 0xFFFF;
  g_adc_channel_cfg.scan_mask           = ADC_MASK_VOLT;//ADEXICR
  g_adc_extended_cfg_t.adc_vref_control = ADC_VREF_CONTROL_VREFH0_AVSS0;//ADHVREFCNT
  //g_adc_cfg.mode                        = ADC_MODE_SINGLE_SCAN;     //ADCSR
  //g_adc_cfg.trigger                     = ADC_TRIGGER_SOFTWARE; //ADCSR - ADSTRGR
  //g_adc_extended_cfg_t.clearing         = ADC_CLEAR_AFTER_READ_ON; //ADCER.ACE
  g_adc_cfg.p_extend = &g_adc_extended_cfg_t;
  initializeADC();
  delay(1);
    //R_ADC_ScanStart(&g_adc_ctrl);
    R_ADC_Read (&g_adc_ctrl, ADC_CHANNEL_VOLT, &adc_data);
    while( ADC_STATE_SCAN_IN_PROGRESS == status.state) {
     R_ADC_StatusGet(&g_adc_ctrl, &status);
    }
    //R_ADC_InfoGet(&g_adc_ctrl, &adc_info);
    vAref = VREF * 4096.0 / adc_data;
    //Serial.println(vCC);
    return vAref;
}

float readVcc(){
  uint16_t adc_data = 0; // R_ADC0->ADTSDR & 0xFFFF;
  g_adc_channel_cfg.scan_mask           = ADC_MASK_VOLT;//ADEXICR
  g_adc_extended_cfg_t.adc_vref_control = ADC_VREF_CONTROL_AVCC0_AVSS0; //ADHVREFCNT
  //g_adc_cfg.mode                        = ADC_MODE_SINGLE_SCAN; //ADCSR + ADSTRGR
  //g_adc_extended_cfg_t.clearing         = ADC_CLEAR_AFTER_READ_ON; //ADCER.ACE
  //g_adc_extended_cfg_t.add_average_count = ADC_ADD_AVERAGE_FOUR; //ADADC
  g_adc_cfg.p_extend = &g_adc_extended_cfg_t;
  initializeADC();
  delay(1);
    //R_ADC_ScanStart(&g_adc_ctrl);
    R_ADC_Read (&g_adc_ctrl, ADC_CHANNEL_VOLT, &adc_data);
    while( ADC_STATE_SCAN_IN_PROGRESS == status.state) {
     R_ADC_StatusGet(&g_adc_ctrl, &status);
    }
    //R_ADC_InfoGet(&g_adc_ctrl, &adc_info);
    vCC = (VREF * 4096.0) / adc_data;
    delay(1);
    //Serial.println(vCC);
    return vCC;
}

