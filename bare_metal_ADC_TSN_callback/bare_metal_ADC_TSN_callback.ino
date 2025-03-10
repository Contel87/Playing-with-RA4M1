/*
 * UNO R4 MINIMA ADC HAL module use PCLKC clock @24Mhz, so 1 ADCLK = 24Mhz (41.7nSec).PCLKB clock working @48Mhz.
 * PCLKB to PCLKC (ADCLK) frequency ratio = 2:1
 * According to "Renesas RA4M1 Group User's Manual: Hardware": Maximum frequency of PCLKB is 32 MHz and maximum frequency of PCLKC (ADCLK) is 64MHz.
 * For Sleep mode of ADC module set ADHVREFCNT.ADSLP = 1
 *
*/

#include "IRQManager.h"

// =========== ADC14 ============
// 35.2 Register Descriptions
  #define ADCBASE 0x40050000 /* ADC Base */
  #define ADC140_ADCSR   ((volatile unsigned short *)(ADCBASE + 0xC000)) // A/D Control Register
  #define ADCSR_ADST        15 // A/D Conversion Start
  #define ADC140_ADANSA0 ((volatile unsigned short *)(ADCBASE + 0xC004)) // A/D Channel Select Register A0
  #define ADC140_ADANSA1 ((volatile unsigned short *)(ADCBASE + 0xC006)) // A/D Channel Select Register A1
  #define ADC140_ADANSB0    ((volatile unsigned short *)(ADCBASE + 0xC014))// A/D Channel Select Register B0
  #define ADC140_ADANSB1    ((volatile unsigned short *)(ADCBASE + 0xC016))// A/D Channel Select Register B1
  #define ADC140_ADADS0     ((volatile unsigned short *)(ADCBASE + 0xC008))// A/D-Converted Value Addition/Average Channel Select Register 0
  #define ADC140_ADADS1     ((volatile unsigned short *)(ADCBASE + 0xC00A))// A/D-Converted Value Addition/Average Channel Select Register 1
  #define ADC140_ADADC      ((volatile unsigned char  *)(ADCBASE + 0xC00C))// A/D-Converted Value Addition/Average Count Select Register
  #define ADC140_ADCER   ((volatile unsigned short *)(ADCBASE + 0xC00E)) // A/D Control Extended Register 
  #define ADC140_ADEXICR ((volatile unsigned short *)(ADCBASE + 0xC012)) // A/D Conversion Extended Input Control Register
  #define ADC140_ADDISCR    ((volatile unsigned char  *)(ADCBASE + 0xC07A)) // A/D Disconnection Detection Control Register
  #define ADC140_ADHVREFCNT ((volatile unsigned char  *)(ADCBASE + 0xC08A)) // A/D High-Potential/Low-Potential Reference Voltage Control Register
  #define ADC140_ADOCDR  ((volatile unsigned short *)(ADCBASE + 0xC01C)) // A/D result of internal reference voltage
  #define ADC140_ADTSDR  ((volatile unsigned short *)(ADCBASE + 0xC01A)) // A/D conversion result of temperature sensor output
  #define MSTP 0x40040000 // Module Registers
  #define MSTP_MSTPCRD   ((volatile unsigned int   *)(MSTP + 0x7008))      // Module Stop Control Register D
  #define MSTPD16 16 // ADC140 - 14-Bit A/D Converter Module
  #define ADC140_ADTSDR  ((volatile unsigned short *)(ADCBASE + 0xC01A)) // A/D conversion result of temperature sensor output
  #define ADC140_ADSSTRT  ((volatile unsigned char *)(ADCBASE + 0xC0DE))      // AN00 A/D Sampling State Register
  #define ADC140_ADSTRGR    ((volatile unsigned short *)(ADCBASE + 0xC010)) // A/D Conversion Start Trigger Select Register
  #define ADC140_ADGSPCR    ((volatile unsigned short *)(ADCBASE + 0xC080)) // A/D Group Scan Priority Control Register
  #define ADC140_ADCMPCR    ((volatile unsigned short *)(ADCBASE + 0xC090)) // // A/D Compare Function Control Register
  #define ADC140_ADCMPANSR0 ((volatile unsigned short *)(ADCBASE + 0xC094)) // A/D Compare Function Window A Channel Select Register 0
  #define ADC140_ADCMPANSR1 ((volatile unsigned short *)(ADCBASE + 0xC096)) // A/D Compare Function Window A Channel Select Register 1
  #define ADC140_ADCMPLR0   ((volatile unsigned short *)(ADCBASE + 0xC098)) // A/D Compare Function Window A Comparison Condition Setting Register 0
  #define ADC140_ADCMPLR1   ((volatile unsigned short *)(ADCBASE + 0xC09A)) // A/D Compare Function Window A Comparison Condition Setting Register 1
  #define ADC140_ADCMPLER   ((volatile unsigned char  *)(ADCBASE + 0xC093)) // A/D Compare Function Window A Extended Input Comparison Condition Setting Register
  #define ADC140_ADCMPDR0   ((volatile unsigned short *)(ADCBASE + 0xC09C)) // A/D Compare Function Window A Lower-Side Level Setting Register
  #define ADC140_ADCMPDR1   ((volatile unsigned short *)(ADCBASE + 0xC09E)) // A/D Compare Function Window A Upper-Side Level Setting Register
  #define ADC140_ADWINLLB   ((volatile unsigned short *)(ADCBASE + 0xC0A8)) // A/D Compare Function Window B Lower-Side Level Setting Register
  #define ADC140_ADWINULB   ((volatile unsigned short *)(ADCBASE + 0xC0AA)) // A/D Compare Function Window B Upper-Side Level Setting Register
  #define ADC140_ADCMPSR0   ((volatile unsigned short *)(ADCBASE + 0xC0A0)) // A/D Compare Function Window A Channel Status Register 0
  #define ADC140_ADCMPSR1   ((volatile unsigned short *)(ADCBASE + 0xC0A2)) // A/D Compare Function Window A Channel Status Register 1
  #define ADC140_ADCMPSER   ((volatile unsigned char  *)(ADCBASE + 0xC0A4)) // A/D Compare Function Window A Extended Input Channel Status Register
  #define ADC140_ADCMPBNSR ((volatile unsigned char  *)(ADCBASE + 0xC0A6)) // A/D Compare Function Window B Channel Select Register
  #define ADC140_ADCMPBSR   ((volatile unsigned char  *)(ADCBASE + 0xC0AC)) // A/D Compare Function Window B Status Register
  #define ADC140_ADWINMON   ((volatile unsigned char  *)(ADCBASE + 0xC08C)) // A/D Compare Function Window A/B Status Monitor Register
  // Temperature Sensor
//   The sensor outputs a voltage directly proportional to the die temperature
//   The relationship between the die temperature and the output voltage is (aproximatly) linear.
//   Note: The temperature scale is inverted i.e. colder is a bigger number
#define TSN_TSCDRH   ((volatile unsigned char *)(0x407EC229))             // Temperature Sensor Calibration Data Register H
#define TSN_TSCDRL   ((volatile unsigned char *)(0x407EC228))             // Temperature Sensor Calibration Data Register L

#define SYSTEM 0x40010000 // System Registers
#define SYSTEM_SCKDIVCR  ((volatile unsigned int *)(SYSTEM + 0xE020))  // System Clock Division Control Register

#define PRINT_REG(BUCKET, REGISTER) \
  do {                              \
    uint32_t t = BUCKET->REGISTER;  \
    Serial.print(#REGISTER ":0x");  \
    Serial.print(t, HEX);           \
    Serial.print(" ");              \
  } while (false)


#define PERIOD         1000
#define SLOPE          -0.0082  // Sensibilità del sensore di temperatura V/℃ ((vs - v1) / (24 - 125)) --> Correggere qui eventuale errore di temperatura reale/misurata (con termometro su chip)
#define T125            125     // Temperatura di fabbrica alla quale e' stato calibrato il sensore
#define VREF           1.55     // Tensione interna di riferimento (vedere datasheet)

float degC, vAref, vCC = 0;
uint16_t adc_data = 0; // to store readed data
GenericIrqCfg_t cfg = {FSP_INVALID_VECTOR, 12, ELC_EVENT_ADC0_SCAN_END}; // Configura l'interrupt che richiama la funzione adc0_callback

//inline float readTemp() __attribute__((always_inline));
//inline float readVcc() __attribute__((always_inline));
//inline float readAref() __attribute__((always_inline));

void setup() {
  init_ADC();
  IRQManager::getInstance().addGenericInterrupt(cfg, adc0_callback);//Richiama la funzione adc0_callback ad ogni lettura della temperatura
   pinMode(21, OUTPUT);
  Serial.begin(115200);
  delay(2000);
  Serial.println();
}


void loop() {
  static uint32_t last = millis();
  uint32_t now = millis();
  if(now - last >= PERIOD) {
    last = now;
  degC  = readTemp();      // Misurazione della temperatura con sensore integrato nella CPU
  vAref = readAref();      // Misurazione della tensione del pin Aref (3,3 V)
  vCC   = readVcc();       // Misura della tensione Vcc (5V) --- Lo fa già la funzione readTemp()
   Serial.print("Temp:");Serial.print(degC, 2);Serial.print("°C");  // Temperatura di uscita in serie
  Serial.print(", ");
  Serial.print("vAref:");Serial.print(vAref, 3);Serial.print("V");  // Uscita in tensione Aref in serie
  Serial.print(", ");
  Serial.print("vCC:");Serial.print(vCC, 3);Serial.println("V");  // Uscita tensione Vcc in serie
  }
}

void init_ADC(){
  *MSTP_MSTPCRD &= ~(0x01 << MSTPD16);  // Enable ADC140 module
  *ADC140_ADCSR  = 0x0000;           // R_ADC_Close(),Set single scan mode,Deselect double trigger mode
  // Select Channels (when usins TSN all 25 channels must be 0)
  *ADC140_ADANSA0 = 0x0000;          // set the ADANSA0 register to 0000h to deselect all analog input channels.
  *ADC140_ADANSA1 = 0x0000;          // set the ADANSA1 register to 0000h to deselect all analog input channels.
  *ADC140_ADANSB0 = 0x0000;          // set the ADANSB0 register to 0000h to deselect all analog input channels.
  *ADC140_ADANSB1 = 0x0000;          // set the ADANSB1 register to 0000h to deselect all analog input channels.
  // Set Addition/Average channels
  *ADC140_ADADS0 = 0x0000;           //Associated Addition/Average channels to ADANSA0 
  *ADC140_ADADS1 = 0x0000;           //Associated Addition/Average channels to ADANSA1
  *ADC140_ADADC  = 0x00;             // Single Conversion, no averaging
  *ADC140_ADEXICR = 0x0000;          // Chose Temperature or internal reference voltage and set (if used) average channels
  *ADC140_ADCER = 0x0026;            //14-bit accuracy,Automatic clearing enabled,Flush-right for the A/D data register format selected
  // Set conversion time ...
  //After executing discharge, the A/D converter executes sampling ...
  //When executing the A/D conversion of the temperature sensor output, the ADDISCR register is set to 0Fh and the
  //ADC14 executes discharge (15 ADCLK) before executing sampling. The minimum sampling time is 5 μs. The ADC14
  //executes discharge each time it executes A/D conversion of the temperature sensor output.
  *ADC140_ADDISCR = 0x0F; // Precharge/Discharge period (0Fh = 15 ADCLK cycles)
  *ADC140_ADSSTRT = 0x78; // 0x78 = 5uS @24Mhz --> ((1/24000000) * 120 = 5uSec)

// Read only result registers:
//ADDRy (y = 0 to 14, 16 to 27): 16-bit read-only registers for storing A/D conversion results
//ADDBLDR: 16-bit read-only register for storing the result of A/D conversion in response to the second trigger in double trigger mode
//ADDBLDRA and ADDBLDRB: 16-bit read-only registers for storing the A/D conversion results in response to the respective triggers during extended operation in double trigger mode

  //---------SETUP GROUP MODE---------//
  *ADC140_ADSTRGR = 0x0000; // Start Trigger Select for Group B or Group A
  *ADC140_ADGSPCR = 0x0000;//Group A Priority Control Setting,Group B Restart Setting,Group B Single Scan Continuous Start
  *ADC140_ADCMPCR = 0x0000;// Enable / Disable interrupts
  //----------------------WINDOW A Channel Select--------------//
  *ADC140_ADCMPANSR0 = 0x0000; // A/D Compare Function Window A Channel Select Register 0
  *ADC140_ADCMPANSR1 = 0x0000; // A/D Compare Function Window A Channel Select Register 0
  //----------------------WINDOW B Channel Select--------------//
  *ADC140_ADCMPBNSR = 0x00;// A/D Compare Function Window B Channel Select Register ???
  *ADC140_ADCMPBSR = 0x00; // A/D Compare Function Window B Status Register
  //---Comparison Condition---//
  *ADC140_ADCMPLR0 = 0x0000;// A/D Compare Function Window A Comparison Condition Setting Register 0
  *ADC140_ADCMPLR1 = 0x0000; // A/D Compare Function Window A Comparison Condition Setting Register 1
  *ADC140_ADCMPLER = 0x0000; // TSN or voltage reference settings !!!
  //---Level Setting GROUP A and GROUP B---/
  *ADC140_ADCMPDR0 = 0x0000; // A/D Compare Function Window A Lower-Side Level Setting Register
  *ADC140_ADCMPDR1 = 0x0000; // A/D Compare Function Window A Upper-Side Level Setting Register
  *ADC140_ADWINLLB = 0x0000; // A/D Compare Function Window B Lower-Side Level Setting Register
  *ADC140_ADWINULB = 0x0000; // A/D Compare Function Window B Upper-Side Level Setting Register
  //---Comparison status A---//
  *ADC140_ADCMPSR0 = 0x0000; // Conparison result Window A Channel Status Register 0
  *ADC140_ADCMPSR1 = 0x0000; // Conparison result Window A Channel Status Register 1
  //---Comparison status B---//
  // ADC140_ADWINMON is rean only register that indicate result of comparison B
  //--- Comparison status for TSN or reference voltage ---//
  *ADC140_ADCMPSER =0x00; // Compare function result for TSN and reference voltage !!!
}

void deinit_ADC(){
  *MSTP_MSTPCRD |= (0x01 << MSTPD16);  // Disable ADC140 module
}

void print_PCLKC(){ // Check System Clock settings (ADC moodule use PCLKC clock)
  Serial.print("SysClocks = "); // UNO R4 MINIMA PCLKC is set to 24Mhz        
  Serial.println(*SYSTEM_SCKDIVCR, HEX); //UNO R4 MINIMA PCLKB is set to 48Mhz
}

float readTemp(){
  R_BSP_IrqEnable((IRQn_Type)cfg.irq); // la funzione adc0_callback viene richiamata soltanto quando viene letta la temperatura
  float mcu_temp_c = 0;
  int16_t v125;
  float v1,vs;
  vCC   = readVcc();
  *ADC140_ADEXICR = 0x0100;          // ADC_MASK_VOLT
  *ADC140_ADHVREFCNT = 0x00;         // ADC_VREF_CONTROL_AVCC0_AVSS0;
  *ADC140_ADCSR |= (0x01 << ADCSR_ADST);     // Start A/D conversion process - R_ADC_ScanStart()
  while(*ADC140_ADCSR != 0){}  // whait end conversion
    v125 = (*TSN_TSCDRH << 8) + *TSN_TSCDRL;    // My RA4M1 chip gives ADC value of 844 - yours will be different
    v1 = 3.3 * v125 / 16383; // Voltage output by the TNS at 125 degreeC - Example: 0.17001 V when v125 = 844 for 125°C
    adc_data = *ADC140_ADTSDR; // R_ADC0->ADTSDR & 0xFFFF;
    vs = vCC * adc_data / 16383; // Voltage output by the TNS at the time of measurement of T1 (volt)
    //Serial.print("vs: ");Serial.print(vs);Serial.print(" v1: ");Serial.println(v1);
    mcu_temp_c = (vs - v1) / SLOPE + T125;
    return mcu_temp_c;
}


float readVcc(){
  *ADC140_ADEXICR = 0x0200;          // ADC_MASK_VOLT
  *ADC140_ADHVREFCNT = 0x00;         // ADC_VREF_CONTROL_AVCC0_AVSS0;
  *ADC140_ADCSR |= (0x01 << ADCSR_ADST);     // Start A/D conversion process - R_ADC_ScanStart()
  while(*ADC140_ADCSR != 0){}  // whait end conversion
adc_data = *ADC140_ADOCDR; // R_ADC0->ADOCDR & 0xFFFF;
vCC = (VREF * 16383.0) / adc_data; //4096 12bit - 16383 14bit
return vCC;
}

float readAref(){
  *ADC140_ADEXICR = 0x0200;          // ADC_MASK_VOLT
  *ADC140_ADHVREFCNT = 0x01;         // ADC_VREF_CONTROL_AVCC0_AVSS0;
  *ADC140_ADCSR |= (0x01 << ADCSR_ADST);     // Start ADC conversion - R_ADC_ScanStart()
while(*ADC140_ADCSR != 0){}  // whait end conversion
adc_data = *ADC140_ADOCDR; // R_ADC0->ADOCDR & 0xFFFF;
vCC = (VREF * 16383.0) / adc_data; //4096 12bit - 16383 14bit
return vCC;
}

static void adc0_callback (){ //adc_callback_args_t * p_args
  IRQn_Type irq = R_FSP_CurrentIrqGet();
  R_BSP_IrqStatusClear(irq);
  //R_ICU->IELSR[irq] &= ~(R_ICU_IELSR_IR_Msk); // Same to R_BSP_IrqStatusClear(irq);
  digitalWrite(21, !digitalRead(21));
  R_BSP_IrqDisable((IRQn_Type)cfg.irq); // Disabilita la funzione di callback(viene abilitata soltanto per il controllo temperatura, non per vcc e vref)
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
