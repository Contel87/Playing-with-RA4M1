/*
ESEMPIO DI UTILIZZO DI INTERRUPT SPECIFICO SU R4 MINIMA
Esempio di come collegare un interrupt ad una funzione:
Nel setup viene dichiarato l'interrupt da intercettare e la funzione da richiamare in caso di interrupt ... (in questo caso il pin D2 di R4 MINIMA a GND), per la lista completa vedere il file bsp_elc.h nel core di arduino;
Successivamente, sempre nel setup viene configurata la porta P105 (pin D2 di R4 MINIMA) per funzionare da interrupt;
In sostanza ogni volta che il pin D2 e' collegato a GND(ad esempio con un pulsante) viene eseguita la relativa funzione (in questo caso Myirq0_callback())
*/

#include "IRQManager.h"

volatile bool interruptFired = false;

void setup() {

  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("\n\n *** " __FILE__ " ***\n\n");
  pin2IQR();
  // Enable the interrupt at the peripheral. 
  R_PFS->PORT[1].PIN[5].PmnPFS = (1 << R_PFS_PORT_PIN_PmnPFS_PCR_Pos) | (1 << R_PFS_PORT_PIN_PmnPFS_ISEL_Pos);

}

void loop() {
  Serial.println("Loop Running");
  delay(500);

  if (interruptFired) {
    interruptFired = false;
    Serial.println("Interrupt Fired");
  }
}

void pin2IQR(){
GenericIrqCfg_t cfg = {FSP_INVALID_VECTOR, 12, ELC_EVENT_ICU_IRQ0};
IRQManager::getInstance().addGenericInterrupt(cfg, Myirq0_callback);
}

void Myirq0_callback() {
  IRQn_Type irq = R_FSP_CurrentIrqGet();
  R_BSP_IrqStatusClear(irq);
  
  interruptFired = true;
}