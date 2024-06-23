/*
This program is free software: you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation, either version 3 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program.  If not, see <http://www.gnu.org/licenses/>.

Questo scketch mostra il valore del sensore CTSU compreso tra 0 e 65535 (poggiare il dito sul cuore di UNO R4).
Inserire un condensatore da 10nF tra il pin 10 e GND di arduino MINIMA.
Riferimenti:
https://www.renesas.com/jp/ja/document/apn/capacitive-sensor-mcu-qe-capacitive-touch-advanced-mode-parameter-guide?language=en&r=1054146
https://www.renesas.com/jp/ja/document/apn/capacitive-sensor-microcontrollers-ctsu-capacitive-touch-introduction-guide?language=en&r=1054146
https://www.renesas.com/us/en/products/microcontrollers-microprocessors/ra-cortex-m-mcus/ra4m1-32-bit-microcontrollers-48mhz-arm-cortex-m4-and-lcd-controller-and-cap-touch-hmi
https://forum.arduino.cc/t/lets-play-with-the-ctsu/1187758;

The following sequence describes the operation shown in Figure 41.17:
1. After initial settings are made, operation is started by writing 1 to the CTSUCR0.CTSUSTRT bit.
2. After the channel to be measured is determined according to the preset conditions, a request to set the channel
(CTSU_CTSUWR) is output.
3. On completion of writing the measurement channel settings (CTSUSSC, CTSUSO0, and CTSUSO1 registers), the
sensor drive pulse is output and the sensor ICO clock and the reference ICO clock operate. At the same time, a
rising edge pulse is output to the transmit pin on the measurement channel for the high-level period of the sensor
drive pulse.
4. After the sensor stabilization wait time and the measurement time elapse and measurement stops, a measurement
result read request (CTSU_CTSURD) is output.
5. The same channel, measurement is performed by outputting a pulse that becomes a falling edge during the highlevel
period of the sensor drive pulse.
6. After the same channel is measured twice, the channel to be measured next is determined and measured in the same
way.
7. On completion of all measurement channels, a measurement end interrupt (CTSU_CTSUFN) is output and
measurement stops (transition to Status 0).
The CTSU Mutual Capacitance Status Flag (CTSUST.CTSUPS bit) changes when Status 5 transitions to Status 1.
Table 41.8 lists the touch pin states in mutual capacitance full scan mode.

*/


// Per collegare il modulo DTC rimuovere i commenti ai 3 passaggi
//#include "r_dtc.h" // !!! PASSAGGIO 1/3 !!!
#include "IRQManager.h"

int ctsurdEventLinkIndex = 0;
int ctsuwrEventLinkIndex = 0;

// !!! PASSAGGIO 2/3 !!!
//dtc_extended_cfg_t rd_ext;
//dtc_extended_cfg_t wr_ext;

//volatile bool ctsu_done = true;
volatile boolean newData = false;
volatile uint16_t sCounter = 0; // CTSU Sensor Counter
volatile uint16_t rCounter = 0; // CTSU Reference Counter

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
    
  Serial.println("\n\n\n*** Starting R4_CTSU_Test.ino***\n\n\n");
  initialCTSUsetup();
}

void loop() {
  if (newData) {
    newData = false;
    noInterrupts();
    uint32_t s = sCounter;
    uint32_t r = rCounter;
    interrupts();
    Serial.print("Result : ");
    Serial.print(s);
    Serial.print(" : ");
    Serial.print(r);
    Serial.println();
  }
}

// Follow the flow in Figure 41.11 for reading the sensor

// Write Request Interrupt.  
void CTSUWR_handler() {
  // This is called when Status Counter is 1

  //resetEventLink(ctsuwrEventLinkIndex);
  IRQn_Type irq = R_FSP_CurrentIrqGet(); // Reset IRQ
  R_BSP_IrqStatusClear(irq);             // Reset IRQ
  
  // Set the measurement channel to move to Status 2
  R_CTSU->CTSUMCH0 = 0;
       
  // Write to CTSUSO1 to select the clock divider to move to status 3
  R_CTSU->CTSUSO1 = 0x0F00;
  
}

// Data Ready Interrupt
void CTSURD_handler() {
  // This will be called at Status Counter 5 when measurement is ready to read
  //resetEventLink(ctsurdEventLinkIndex);

  IRQn_Type irq = R_FSP_CurrentIrqGet();
  R_BSP_IrqStatusClear(irq);
  
  // Read the results from the counter registers.
  sCounter = R_CTSU->CTSUSC;
  rCounter = R_CTSU->CTSURC;
  
  
  
  
  
}

void CTSUFN_handler()
{
  IRQn_Type irq = R_FSP_CurrentIrqGet();
  R_BSP_IrqStatusClear(irq);
  //ctsu_done = true;
  // Start the measurement.
  //startCTSUmeasure();
  if (((R_CTSU->CTSUST & 7) == 0))
  {
    newData = true;
    startCTSUmeasure();
  }
}


void startCTSUmeasure() {
  //ctsu_done = false;
  // There has to be at least one measurement channel to software start
  R_CTSU->CTSUMCH0 = 0;

  // Software Start in CTSUCR0 register  
  R_CTSU->CTSUCR0 = 1;
}

void stopTouchMeasurement()
{
  R_CTSU->CTSUCR0 = 0x10;
}

void initialCTSUsetup() {
  // Follow the flow chart Fig 41.9
  // Step 1: Discharge LPF (set TSCAP as OUTPUT LOW.)
  R_PFS->PORT[1].PIN[12].PmnPFS = (1 << R_PFS_PORT_PIN_PmnPFS_PDR_Pos);
  delay(1000);

  // Step 2: Setup I/O port PmnPFR registers
      // TSCAP Pin  
  R_PFS->PORT[1].PIN[12].PmnPFS = (1 << R_PFS_PORT_PIN_PmnPFS_PMR_Pos) | (0x0C << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos);

      // Sensor Pin
  R_PFS->PORT[2].PIN[4].PmnPFS = (1 << R_PFS_PORT_PIN_PmnPFS_PMR_Pos) | (0x0C << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos);

  // Step 3: Enable CTSU in MSTPCRC bit MSTPC3 to 0
  R_MSTP->MSTPCRC &= ~(1 << R_MSTP_MSTPCRC_MSTPC3_Pos);

  // Step 4: Set CTSU Power Supply (CTSUCR1 register)  
  R_CTSU->CTSUCR1 = 0;

  // Step 5: Set CTSU Base Clock (CTSUCR1 and CTSUSO1 registers)
  R_CTSU->CTSUSO1 = 0x0F00;

  // Step 6: Power On CTSU (set bits CTSUPON and CTSUCSW in CTSUCR1 at the same time)
  R_CTSU->CTSUCR1 |= 0x03;

  // Step 7: Wait for stabilization (Whatever that means...)
  delay(1000);

  // Continued from flow chart Fig 41.12

  // Step 8: Other Registers
      // CTSUCR1   // Select operating clock

      // CTSUSDPRS  // Noise Filter
  R_CTSU->CTSUSDPRS = 0x63;

      // CTSUSST   // Stabilization Time
  R_CTSU->CTSUSST = 0x10;

      // CTSUCHAC0-4  Select the channel for measurement
  R_CTSU->CTSUCHAC[0] = 1;
  R_CTSU->CTSUMCH0 = 0;

      // CTSUCR0  // Control Start Signals  

      // CTSUDCLKC  ???  
  R_CTSU->CTSUDCLKC = 0x30;

  // Step 9:   
      // Enable the interrupts
  // CTSUWR is event 0x42
  // CTSURD is event 0x43
  // CTSUFN is event 0x44
  //ctsurdEventLinkIndex = attachEventLinkInterrupt(0x43, CTSURD_handler);
  //ctsuwrEventLinkIndex = attachEventLinkInterrupt(0x42, CTSUWR_handler);
  // CTSUWR is event 0x42
    // CTSURD is event 0x43
    // CTSUFN is event 0x44
    GenericIrqCfg_t rd_int_cfg = {FSP_INVALID_VECTOR, 12, ELC_EVENT_CTSU_READ};
    GenericIrqCfg_t wr_int_cfg = {FSP_INVALID_VECTOR, 12, ELC_EVENT_CTSU_WRITE};
    GenericIrqCfg_t fn_int_cfg = {FSP_INVALID_VECTOR, 12, ELC_EVENT_CTSU_END};
    IRQManager::getInstance().addGenericInterrupt(rd_int_cfg, CTSURD_handler);
    //rd_ext.activation_source = rd_int_cfg.irq; /// !!! PASSAGGIO 3/3 !!!
    IRQManager::getInstance().addGenericInterrupt(wr_int_cfg, CTSUWR_handler);
    //wr_ext.activation_source = wr_int_cfg.irq; /// !!! PASSAGGIO 3/3 !!!
    IRQManager::getInstance().addGenericInterrupt(fn_int_cfg, CTSUFN_handler);

  // Step 10: Start Measurement
  startCTSUmeasure();

}
