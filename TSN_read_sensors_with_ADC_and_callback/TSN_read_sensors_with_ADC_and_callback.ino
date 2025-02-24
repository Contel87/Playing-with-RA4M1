
#include "r_adc.h"
#include "IRQManager.h"

#define PERIOD          1000    // Intervallo di lettura
#define SLOPE          -0.0034  // Sensibilità del sensore di temperatura V/℃ ((vs - v1) / (24 - 125)) --> Correggere qui eventuale errore di temperatura reale/misurata (con termometro su chip)
#define T125            125     // Temperatura di calibrazione di fabbrica del microcontrollore RA4M1
#define VREF            1.45    // Tensione interna di riferimento (vedere datasheet RA4M1), correggere qui eventuale discrepanza tra la tensione letta con un tester e quella stampata su serial monitor

// Configurazione dei registri del modulo ADC per leggere la tensione sul pin aref del microcontrollore
  adc_instance_ctrl_t aref_ctrl;
  adc_cfg_t aref_cfg;
  adc_extended_cfg_t aref_extended_cfg_t;
  adc_channel_cfg_t aref_channel_cfg;

// Configurazione dei registri del modulo ADC per leggere la vCC del microcontrollore
adc_instance_ctrl_t vcc_ctrl;
adc_cfg_t vcc_cfg;
adc_extended_cfg_t vcc_extended_cfg_t;
adc_channel_cfg_t vcc_channel_cfg;

// Configurazione dei registri del modulo ADC per il sensore di temperatura
adc_instance_ctrl_t tsn_ctrl;
adc_cfg_t tsn_cfg;
adc_extended_cfg_t tsn_extended_cfg_t;
adc_channel_cfg_t tsn_channel_cfg;
adc_info_t tsn_info; // lettura valori delle conversioni (dai registri del modulo ADC)

adc_status_t status; // status del campionamento adc


GenericIrqCfg_t cfg = {FSP_INVALID_VECTOR, 12, ELC_EVENT_ADC0_SCAN_END}; // Configura l'interrupt che richiama la funzione adc0_callback

float degC, vAref, vCC; // Variabili dove memorizzare i valori letti

void setup() {
  initialize_aref_reg(); // Configura il modulo ADC per leggere la tensione AREF
  initialize_vcc_reg();  // Configura il modulo ADC per leggere la tensione Vcc
  initialize_tsn_reg();  // Configura il modulo ADC per leggere la temperatura interna del microcontrollore
  pinMode(21,OUTPUT);    // il led TX della UNO R4 indica la lettura dell'interrupt dal modulo ADC
  IRQManager::getInstance().addGenericInterrupt(cfg, adc0_callback);//Richiama la funzione adc0_callback ad ogni lettura della temperatura
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
  //vCC   = readVcc();     // Misura della tensione Vcc (5V) --- Lo fa già la funzione readTemp()
  Serial.print("Temp:");Serial.print(degC, 2);Serial.print("°C");  // Temperatura di uscita su seriale
  Serial.print(", ");
  Serial.print("vAref:");Serial.print(vAref, 3);Serial.print("V");  // Uscita in tensione Aref su seriale
  Serial.print(", ");
  Serial.print("vCC:");Serial.print(vCC, 3);Serial.println("V");  // Uscita tensione Vcc su seriale
  }
}

void initialize_aref_reg(){
  aref_channel_cfg.scan_mask           = ADC_MASK_VOLT;//ADEXICR
  aref_extended_cfg_t.adc_vref_control = ADC_VREF_CONTROL_VREFH0_AVSS0;//ADHVREFCNT
  aref_cfg.p_extend = &aref_extended_cfg_t;
}

void initialize_vcc_reg(){
vcc_channel_cfg.scan_mask           = ADC_MASK_VOLT;//ADEXICR
vcc_extended_cfg_t.adc_vref_control = ADC_VREF_CONTROL_AVCC0_AVSS0; //ADHVREFCNT
vcc_cfg.p_extend = &vcc_extended_cfg_t;
}

void initialize_tsn_reg(){
tsn_channel_cfg.scan_mask           = ADC_MASK_TEMPERATURE; //ADEXICR
tsn_cfg.p_extend = &tsn_extended_cfg_t;
}


float readTemp(){
  uint16_t adc_data = 0; // R_ADC0->ADTSDR & 0xFFFF;
  float mcu_temp_c = 0;
  int16_t v125;
  float v1,vs;
  vCC   = readVcc();
  R_BSP_IrqEnable((IRQn_Type)cfg.irq); // la funzione adc0_callback viene richiamata soltanto quando viene letta la temperatura
  analogReference(AR_INTERNAL);
  R_ADC_Open(&tsn_ctrl, &tsn_cfg);
  R_ADC_ScanCfg(&tsn_ctrl, &tsn_channel_cfg);
  R_ADC_ScanStart(&tsn_ctrl);
  delay(1);
    
    R_ADC_Read (&tsn_ctrl, ADC_CHANNEL_TEMPERATURE, &adc_data);
    while( ADC_STATE_SCAN_IN_PROGRESS == status.state) {
     R_ADC_StatusGet(&tsn_ctrl, &status);
    }
    R_ADC_InfoGet(&tsn_ctrl, &tsn_info); 
    v125 = (int16_t) tsn_info.calibration_data; // v125   = (R_TSN->TSCDRH << 8) + R_TSN->TSCDRL
    v1 = 3.3f * v125 / 4096; // Voltage output by the TNS at 125 degreeC
    vs = vCC * adc_data / 4096; // Voltage output by the TNS at the time of measurement of T1 (volt)
    //Serial.print("v125: ");Serial.print(v125);Serial.print(" v1: ");Serial.print(v1);Serial.print(" vs: ");Serial.println(vs);
    mcu_temp_c = (vs - v1) / SLOPE + T125;
    //Serial.println(mcu_temp_c);
    return mcu_temp_c;
}

float readAref(){
  uint16_t adc_data = 0; // R_ADC0->ADTSDR & 0xFFFF;
  R_ADC_Open(&aref_ctrl, &aref_cfg);
  R_ADC_ScanCfg(&aref_ctrl, &aref_channel_cfg);
  R_ADC_ScanStart(&aref_ctrl);
  delay(1);
    R_ADC_Read (&aref_ctrl, ADC_CHANNEL_VOLT, &adc_data);
    while( ADC_STATE_SCAN_IN_PROGRESS == status.state) {
     R_ADC_StatusGet(&aref_ctrl, &status);
    }
    vAref = VREF * 4096.0 / adc_data;
    //Serial.println(vCC);
    return vAref;
}

float readVcc(){
  uint16_t adc_data = 0; // R_ADC0->ADTSDR & 0xFFFF;
  R_ADC_Open(&vcc_ctrl, &vcc_cfg);
  R_ADC_ScanCfg(&vcc_ctrl, &vcc_channel_cfg);
  R_ADC_ScanStart(&vcc_ctrl);
  delay(1);
    R_ADC_Read (&vcc_ctrl, ADC_CHANNEL_VOLT, &adc_data);
    while( ADC_STATE_SCAN_IN_PROGRESS == status.state) {
     R_ADC_StatusGet(&vcc_ctrl, &status);
    }
    vCC = (VREF * 4096.0) / adc_data;
    delay(1);
    //Serial.println(vCC);
    return vCC;
}

static void adc0_callback (){ //adc_callback_args_t * p_args
  IRQn_Type irq = R_FSP_CurrentIrqGet();
  R_BSP_IrqStatusClear(irq);
  //R_ICU->IELSR[irq] &= ~(R_ICU_IELSR_IR_Msk); // Same to R_BSP_IrqStatusClear(irq);
  digitalWrite(21, !digitalRead(21));
  R_BSP_IrqDisable((IRQn_Type)cfg.irq); // Disabilita la funzione di callback(viene abilitata soltanto per il controllo temperatura, non per vcc e vref)
}

