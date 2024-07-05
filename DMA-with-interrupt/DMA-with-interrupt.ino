/*
ESEMPIO DI UTILIZZO DEL MODULO DMA SU R4 MINIMA
Il modulo DMA di ARDUINO UNO R4 permette di copiare variabili da una parte ad un altra SENZA l'utilizzo della CPU!!!
In questo esempio un interrupt sul pin D2 oppure D3 di R4 MINIMA (potrebbe essere un encoder rotativo che porta a GND il D2 o il D3) inserisce 
il contenuto dell'array "source0" oppure "source1" nell'array "dest";
*/

#include "IRQManager.h"
#include "r_dtc.h"

//#define ARRAY_SIZE 64

//  These are arrays to play with transferring in and out of
//uint8_t source[ARRAY_SIZE];
//uint8_t dest[ARRAY_SIZE];

char source0[7] = "hello!";
char source1[7] = "byebye";
char dest[7] = "------";

volatile bool interruptFired0 = false;
volatile bool interruptFired1 = false;

// These four structs hold the configuration information needed by the HAL
dtc_instance_ctrl_t IRQ0_ctrl;                      // The HAL will initialize this in Open
transfer_info_t IRQ0_info;                          // This gets built in setup
dtc_extended_cfg_t IRQ0_ext;                        // This gets built in setup
transfer_cfg_t IRQ0_cfg = {&IRQ0_info, &IRQ0_ext};  // This just holds pointers to the previous two.

dtc_instance_ctrl_t IRQ1_ctrl;
transfer_info_t IRQ1_info;
dtc_extended_cfg_t IRQ1_ext;
transfer_cfg_t IRQ1_cfg = {&IRQ1_info, &IRQ1_ext};

void setup() {

  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("\n\n *** " __FILE__ " ***\n\n");
  Serial.print("Original values are: ");
  Serial.print("source0 = ");Serial.println(source0);
  Serial.print("source1 = ");Serial.println(source1);
  Serial.print("dest = ");Serial.println(dest);
  pin2IQR(); // Enable R4 MINIMA pin D2 as interrupt source
  pin3IQR(); // Enable R4 MINIMA pin D3 as interrupt source
  initialize_DTC();
  

}

void loop() {
  Serial.println("Loop Running");
  delay(500);

  if (interruptFired0) {
    interruptFired0 = false;
    Serial.println("Interrupt Fired on D2");
    Serial.print("dest = ");Serial.println(dest);
    // The source and destination addresses can be NULL if you want to leave them where they are.
    // The last argment is the length for the new transfer.  
    // These settings will NOT be copied into the info struct. 
    R_DTC_Reset(&IRQ0_ctrl, source0, dest, 7);
  }

  if (interruptFired1) {
    interruptFired1 = false;
    Serial.println("Interrupt Fired on D3");
    Serial.print("dest = ");Serial.println(dest);
    // The source and destination addresses can be NULL if you want to leave them where they are.
    // The last argment is the length for the new transfer.  
    // These settings will NOT be copied into the info struct. 
    R_DTC_Reset(&IRQ1_ctrl, source1, dest, 7);
  }
}

void pin2IQR(){
GenericIrqCfg_t IRQ0_int_cfg = {FSP_INVALID_VECTOR, 12, ELC_EVENT_ICU_IRQ0};
IRQManager::getInstance().addGenericInterrupt(IRQ0_int_cfg, Myirq0_callback);
IRQ0_ext.activation_source = IRQ0_int_cfg.irq;
// Enable the interrupt at the peripheral. 
  R_PFS->PORT[1].PIN[5].PmnPFS = (1 << R_PFS_PORT_PIN_PmnPFS_PCR_Pos) | (1 << R_PFS_PORT_PIN_PmnPFS_ISEL_Pos);

}


void pin3IQR(){
GenericIrqCfg_t IRQ1_int_cfg = {FSP_INVALID_VECTOR, 12, ELC_EVENT_ICU_IRQ1};
IRQManager::getInstance().addGenericInterrupt(IRQ1_int_cfg, Myirq1_callback);
IRQ1_ext.activation_source = IRQ1_int_cfg.irq;
// Enable the interrupt at the peripheral. 
  R_PFS->PORT[1].PIN[4].PmnPFS = (1 << R_PFS_PORT_PIN_PmnPFS_PCR_Pos) | (1 << R_PFS_PORT_PIN_PmnPFS_ISEL_Pos);

}

void Myirq0_callback() {
  IRQn_Type irq = R_FSP_CurrentIrqGet();
  R_BSP_IrqStatusClear(irq);  
  interruptFired0 = true;
}

void Myirq1_callback() {
  IRQn_Type irq = R_FSP_CurrentIrqGet();
  R_BSP_IrqStatusClear(irq);  
  interruptFired1 = true;
}

static void initialize_DTC()
{
  /* TRANSFER_ADDR_MODE_FIXED - TRANSFER_ADDR_MODE_OFFSET - TRANSFER_ADDR_MODE_INCREMENTED - TRANSFER_ADDR_MODE_DECREMENTED */
  IRQ0_info.transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
  /* TRANSFER_REPEAT_AREA_DESTINATION - TRANSFER_REPEAT_AREA_SOURCE */
  IRQ0_info.transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE;
  /* TRANSFER_IRQ_END - TRANSFER_IRQ_EACH */
  IRQ0_info.transfer_settings_word_b.irq = TRANSFER_IRQ_END;
  /* TRANSFER_CHAIN_MODE_DISABLED - TRANSFER_CHAIN_MODE_EACH - TRANSFER_CHAIN_MODE_END */
  IRQ0_info.transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED; // CHAIN_MODE is DTC ONLY
  /* TRANSFER_ADDR_MODE_FIXED - TRANSFER_ADDR_MODE_OFFSET - TRANSFER_ADDR_MODE_INCREMENTED - TRANSFER_ADDR_MODE_DECREMENTED */
  IRQ0_info.transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
  /* TRANSFER_SIZE_1_BYTE - TRANSFER_SIZE_2_BYTE - TRANSFER_SIZE_4_BYTE */
  IRQ0_info.transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE;
  /* TRANSFER_MODE_NORMAL - TRANSFER_MODE_REPEAT - TRANSFER_MODE_BLOCK - TRANSFER_MODE_REPEAT_BLOCK */
  IRQ0_info.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL;

  IRQ0_info.p_dest = dest; // pointer to where data should go to
  IRQ0_info.p_src = source0;        // pointer to where data should come from
  IRQ0_info.num_blocks = 0;                      // unused in normal mode - number of repeats in repeat mode - number of blocks in block mode
  IRQ0_info.length = 7;                          // number of transfers to make in normal mode - size of repeat area in repeat mode - size of block in block mode

  
  /* TRANSFER_ADDR_MODE_FIXED - TRANSFER_ADDR_MODE_OFFSET - TRANSFER_ADDR_MODE_INCREMENTED - TRANSFER_ADDR_MODE_DECREMENTED */
  IRQ1_info.transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
  /* TRANSFER_REPEAT_AREA_DESTINATION - TRANSFER_REPEAT_AREA_SOURCE */
  IRQ1_info.transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE;
  /* TRANSFER_IRQ_END - TRANSFER_IRQ_EACH */
  IRQ1_info.transfer_settings_word_b.irq = TRANSFER_IRQ_END;
  /* TRANSFER_CHAIN_MODE_DISABLED - TRANSFER_CHAIN_MODE_EACH - TRANSFER_CHAIN_MODE_END */
  IRQ1_info.transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED; // CHAIN_MODE is DTC ONLY
  /* TRANSFER_ADDR_MODE_FIXED - TRANSFER_ADDR_MODE_OFFSET - TRANSFER_ADDR_MODE_INCREMENTED - TRANSFER_ADDR_MODE_DECREMENTED */
  IRQ1_info.transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
  /* TRANSFER_SIZE_1_BYTE - TRANSFER_SIZE_2_BYTE - TRANSFER_SIZE_4_BYTE */
  IRQ1_info.transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE;
  /* TRANSFER_MODE_NORMAL - TRANSFER_MODE_REPEAT - TRANSFER_MODE_BLOCK - TRANSFER_MODE_REPEAT_BLOCK */
  IRQ1_info.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL;

  IRQ1_info.p_dest = dest;         // pointer to where data should go to
  IRQ1_info.p_src =  source1;           // pointer to where data should come from
  IRQ1_info.num_blocks = 0;          // unused in normal mode - number of repeats in repeat mode - number of blocks in block mode
  IRQ1_info.length = 7;              // number of transfers to make in normal mode - size of repeat area in repeat mode - size of block in block mode

/*
//If don't won't use pin2IQR() and pin3IQR() functions, call pinMode(2, INPUT_PULLUP) and pinMode(3, INPUT_PULLUP);
// in to setup block and uncomment the block below:
  attachInterrupt(digitalPinToInterrupt(2), Myirq0_callback, FALLING);
  attachInterrupt(digitalPinToInterrupt(2), Myirq1_callback, FALLING);
  // Find the entry in IELSR for IRQ0 and set as activation source
  for (int i=0; i<32; i++){
    if((R_ICU->IELSR[i] & 0xFF) == ELC_EVENT_ICU_IRQ0) {
      // Set the activation source for the DTC to the interrupt channel we found
      // The interrupt channel will match the vector table index. 
      IRQ0_ext.activation_source = (IRQn_Type)i;
    }
  }

  // Find the entry in IELSR for IRQ1 and set as activation source
  for (int i=0; i<32; i++){
    if((R_ICU->IELSR[i] & 0xFF) == ELC_EVENT_ICU_IRQ1) {
      // Set the activation source for the DTC to the interrupt channel we found
      // The interrupt channel will match the vector table index. 
      IRQ1_ext.activation_source = (IRQn_Type)i;
    }
  }
  */
  
  R_DTC_Open(&IRQ0_ctrl, &IRQ0_cfg);
  R_DTC_Enable(&IRQ0_ctrl);

  R_DTC_Open(&IRQ1_ctrl, &IRQ1_cfg);
  R_DTC_Enable(&IRQ1_ctrl);
}