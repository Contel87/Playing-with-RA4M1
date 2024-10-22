/*
ESEMPIO DI UTILIZZO DI INTERRUPT SPECIFICO SU R4 MINIMA
Esempio di come collegare un interrupt ad una funzione:
Nel setup viene dichiarato l'interrupt da intercettare e la funzione da richiamare in caso di interrupt ... (in questo caso il pin D2 di R4 MINIMA a GND), per la lista completa vedere il file bsp_elc.h nel core di arduino;
Successivamente, sempre nel setup viene configurata la porta P105 (pin D2 di R4 MINIMA) per funzionare da interrupt;
In sostanza ogni volta che il pin D2 e' collegato a GND(ad esempio con un pulsante) viene eseguita la relativa funzione (in questo caso Myirq0_callback())

IMPORTANT notice: "Cortex-M4 Devices Generic user guide" show a table of IRQ priority, user IRQ0 start from 12 :
Priority --> Process
-3             Reset
-2             NMI (Non Mascable Interrupt)
-1             HardFault
 0             MemManage
 1             BusFault
 2             UsageFault
 3             Reserved
 4             Reserved
 5             Reserved
 6             Reserved
 7             SVCall
 8             Reserved
 9             Reserved
 10            PendSV
 11            SysTick
 12            Interrupt(IRQ)  --> User IRQ priority start from here (12)
 N             Interrrupt(IRQ)

* This code is "AS IS" without warranty or liability. 

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

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
GenericIrqCfg_t cfg = {FSP_INVALID_VECTOR, 12, ELC_EVENT_ICU_IRQ0}; // 12 is priority ... seet note1
IRQManager::getInstance().addGenericInterrupt(cfg, Myirq0_callback);
}

void Myirq0_callback() {
  IRQn_Type irq = R_FSP_CurrentIrqGet();
  R_BSP_IrqStatusClear(irq);
  //R_ICU->IELSR[irq] &= ~(R_ICU_IELSR_IR_Msk); // Same to R_BSP_IrqStatusClear(irq);
  
  interruptFired = true;
}