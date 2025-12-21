/* Software standby mode - arduino minima
 * Esempio di come mettere in standby la Uno R4 minima (~40 uA ?) e farla risvegliare con l'interrupt
 * sul pin P411. L'interrupt viene forzato sul IELSR[5] perchè i primi registri IELSR[0] - IELSR[4]
 * sono già occupati a sketch vuoto.
 */

#define IELSR5    *((volatile uint32_t*)0x40006314) // ICU event link setting register

void setup() {
  
  pinMode(21, OUTPUT);
  
  R_SYSTEM -> PRCR = 0xA503;                                     // Write access to SBYCR 
  R_SYSTEM -> SBYCR_b.SSBY = 1;                                 //software standby mode   
                                              
  R_DTC -> DTCST_b.DTCST = 0;  
  R_SYSTEM -> OSTDCR_b.OSTDE = 0;                                //oscillation stop detection off
  R_ICU -> WUPEN = 0x00000010;                                   //IRQ4 in WUPEN
   
}

void loop() { 

  R_PFS->PORT[4].PIN[11].PmnPFS = (1 << R_PFS_PORT_PIN_PmnPFS_PCR_Pos) | (1 << R_PFS_PORT_PIN_PmnPFS_ISEL_Pos); // enable interrupt 
  asm volatile("wfi");
  R_ICU->IELSR[5] &= ~(R_ICU_IELSR_IR_Msk);
  
   
  for(int i=0;i<3;i++){                                           
    digitalWrite(21, HIGH);  
    delay(1000);                     
    digitalWrite(21, LOW);   
  delay(1000); 
  }   
}