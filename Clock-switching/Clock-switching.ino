/*I clock di sistema (interni: HOCO, MOCO, LOCO ... esterni: MOSC e SOSC ... e PLL) possono essere disattivati SOLTANTO se non sono usati come clock di sistema (nel registro SCKSCR.CKSEL[2:0])
Perciò, in linea generale, per cambiare il clock di sistema occorre innanzitutto assicurarsi che il clock scelto non sia già in uso come clock di sistema (non si deve fermare un clock attualmente in uso nel registro SCKSCR),
poi procedere come segue:
1. -------------------------------- Fermare il clock che si vuole usare al posto di quello attuale
2. -------------------------------- Configurare le varie impostazioni relative al clock scelto
3. -------------------------------- Abilitare il clock che si vuole utilizzare
4. -------------------------------- Attendere il tempo di stabilizzazione necessario
5. -------------------------------- Selezionare il clock appena configurato nel registro SCKSCR
*/


#define pinHOCOCR2 ((volatile unsigned char *)0x4001E000) 
uint8_t reg = *pinHOCOCR2; // Esempio di lettura diretta di un registro copiato nella variabile reg



#define PRINT_REG(BUCKET, REGISTER) \
  do { \
    uint32_t t = BUCKET->REGISTER; \
    Serial.print(#REGISTER " : b"); \
    Serial.println(t, BIN); \
  } while (false)

void setup() {
}

void loop() {
  // put your main code here, to run repeatedly:

}

void set48MhzClock(){ // WITH Externel Quartz of 16Mhz AND PLL
  R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_CGC);
  R_SYSTEM->MOSCCR_b.MOSTP = 1;   // SET MOSTP bit to 1 to stop Main Clock Oscillator
  asm volatile ( "dsb" );         // Data bus Synchronization instruction (?)
  R_SYSTEM->MOMCR_b.MODRV1 = 0;   // MODRV1 = 0 (10 MHz to 20 MHz) becouse external quartz is 16Mhz
  R_SYSTEM->MOSCWTCR       = 0x07;// Set stability timeout period (?)
  R_SYSTEM->MOSCCR_b.MOSTP = 0;   // Enable XTAL
  R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); // wait for XTAL to stabilise
// ----------------------------- and now ... setup PLL Frequency Multiplication to 12x and PLL Frequency Division to /4 (16*12/4 = 48Mhz)
R_SYSTEM->PLLCR_b.PLLSTP = 1;   // Disable PLL
R_SYSTEM->PLLCCR2        = 0x0B; // PLL Frequency Multiplication to 12x
R_SYSTEM->PLLCCR2       |= 0x80; // Output Frequency Division to /4
R_SYSTEM->PLLCR_b.PLLSTP = 0;   // Enable PLL
R_BSP_SoftwareDelay(1000, BSP_DELAY_UNITS_MICROSECONDS);// wait for PLL to stabilise
if (R_SYSTEM->OSCSF == 0b0101001){ // Check for stabile XTAL, before allowing switch
  R_SYSTEM->SCKSCR  = 0x05;          // Select PLL as the system clock
  Serial.println("System clock changed to XTAL with PLL"); 
} else {
  Serial.println("XTAL or PLL not stable, remain on HOCO clock");
}
R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_CGC);
}

void set16MhzClock(){ // WITH Externel Quartz of 16Mhz NO PLL
  R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_CGC);
  R_SYSTEM->MOSCCR_b.MOSTP = 1;   // SET MOSTP bit to 1 to stop Main Clock Oscillator
  asm volatile ( "dsb" );         // Data bus Synchronization instruction (?)
  R_SYSTEM->MOMCR_b.MODRV1 = 0;   // MODRV1 = 0 (10 MHz to 20 MHz) becouse external quartz is 16Mhz
  R_SYSTEM->MOSCWTCR       = 0x07;// Set stability timeout period (?)
  R_SYSTEM->PLLCR_b.PLLSTP = 1;   // Disable PLL
  R_SYSTEM->MOSCCR_b.MOSTP = 0;   // Enable XTAL
  R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); // wait for XTAL to stabilise
  R_SYSTEM->SCKSCR  = 0x03; // Select MOSC as system clock
  R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_CGC);
}

void set48MhzHOCO(){ // Internal 48Mhz as MAIN OSC
  R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_CGC);
  R_SYSTEM->MOSCCR_b.MOSTP = 1;   // SET MOSTP bit to 1 to stop Main Clock Oscillator
  asm volatile ( "dsb" );         // Data bus Synchronization instruction (?)
  R_SYSTEM->MOMCR_b.MODRV1 = 1;   // MODRV1 = 1 (1 MHz to 10 MHz)
  R_SYSTEM->MOSCWTCR       = 0x09;// Set stability timeout period
  R_SYSTEM->SCKSCR = 0;           //Select High-speed on-chip oscillator (HOCO) as system clock
  R_SYSTEM->PLLCR_b.PLLSTP = 1;   // Disable PLL
  R_SYSTEM->MOSCCR_b.MOSTP = 1;   // Disable Main clock oscillator (MOSC) as system clock
  R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); // wait for XTAL to stabilise
  R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_CGC);
}



static void rtcLOCO ( void ) { //Switch to internal LOCO
  R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_CGC);
  R_SYSTEM->SOSCCR   = 0x01;          // Stop the sub-clock oscillator
  asm volatile ( "dsb" );             // Data bus Synchronization instruction
  R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
  R_SYSTEM->SOSCCR_b.SOSTP   = 1;     //STOP EXT 32.768
  R_RTC->RCR4_b.RCKSEL       =1;      //Select LOCO
  R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); // wait for XTAL to stabilise
  R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_CGC);
}


static void rtcSOSC ( void ) { // Swith to external 32.768Khz Sub-OScillator
  R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_CGC);
  R_SYSTEM->SOSCCR   = 0x01;       // Stop the sub-clock oscillator
  asm volatile ( "dsb" );          // Data bus Synchronization instruction
  while ( 1 != R_SYSTEM->SOSCCR ) R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
  R_SYSTEM->SOMCR       = 0x00;    // Sub-Clock Oscillator Drive in Normal Mode    
  R_SYSTEM->SOSCCR_b.SOSTP = 0;    // START sub-clock oscillator
  R_RTC->RCR4_b.RCKSEL  = 1;       //Select EXT 32.768 SOSC into RTC module
  R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); // wait for XTAL to stabilise 
  R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_CGC);
}

void clockstatus(){
// System clocks status
  Serial.println("System Clock Division Control Register");
  PRINT_REG(R_SYSTEM, SCKDIVCR);Serial.println();
  Serial.println("System Clock Source Control Register");
  PRINT_REG(R_SYSTEM, SCKSCR);Serial.println(); 
  Serial.println("PLL Clock Control Register 2");
  PRINT_REG(R_SYSTEM, PLLCCR2);Serial.println();
  Serial.println("PLL Control Register");
  PRINT_REG(R_SYSTEM, PLLCR);Serial.println();
  Serial.println("Memory Wait Cycle Control Register");
  PRINT_REG(R_SYSTEM, MEMWAIT);Serial.println();
  Serial.println("Main Clock Oscillator Control Register");
  PRINT_REG(R_SYSTEM, MOSCCR);Serial.println();
  Serial.println("Sub-Clock Oscillator Control Register");
  PRINT_REG(R_SYSTEM, SOSCCR);Serial.println();
  Serial.println("Low-Speed On-Chip Oscillator Control Register");
  PRINT_REG(R_SYSTEM, LOCOCR);Serial.println();
  Serial.println("High-Speed On-Chip Oscillator Control Register");
  PRINT_REG(R_SYSTEM, HOCOCR);Serial.println();
  //PRINT_REG(R_SYSTEM, HOCOCR2);  // High-Speed On-Chip Oscillator Control Register 2
  Serial.println("Middle-Speed On-Chip Oscillator Control Register");
  PRINT_REG(R_SYSTEM, MOCOCR);Serial.println();
  Serial.println("Oscillation Stabilization Flag Register");
  PRINT_REG(R_SYSTEM, OSCSF);Serial.println();
  Serial.println("Oscillation Stop Detection Control Register");
  PRINT_REG(R_SYSTEM, OSTDCR);Serial.println();
  Serial.println("Oscillation Stop Detection Status Register");
  PRINT_REG(R_SYSTEM, OSTDSR);Serial.println();
  Serial.println("Main Clock Oscillator Wait Control Register");
  PRINT_REG(R_SYSTEM, MOSCWTCR);Serial.println();
  Serial.println("High-Speed On-Chip Oscillator Wait Control Register");
  PRINT_REG(R_SYSTEM, HOCOWTCR);Serial.println();
  Serial.println("Main Clock Oscillator Mode Oscillation Control Register");
  PRINT_REG(R_SYSTEM, MOMCR);Serial.println();
  Serial.println("Sub-Clock Oscillator Mode Control Register (LOW POWER MODE)");
  PRINT_REG(R_SYSTEM, SOMCR);Serial.println();
  Serial.println("Segment LCD Source Clock Control Register");
  PRINT_REG(R_SYSTEM, SLCDSCKCR);Serial.println();
  Serial.println("Clock Out Control Register");   
  PRINT_REG(R_SYSTEM, CKOCR);Serial.println();
  Serial.println("LOCO User Trimming Control Register");
  PRINT_REG(R_SYSTEM, LOCOUTCR);Serial.println();
  Serial.println("MOCO User Trimming Control Register");
  PRINT_REG(R_SYSTEM, MOCOUTCR);Serial.println();
  Serial.println("HOCO User Trimming Control Register");
  PRINT_REG(R_SYSTEM, HOCOUTCR);Serial.println();
  Serial.println("Trace Clock Control Register");
  PRINT_REG(R_SYSTEM, TRCKCR);Serial.println();
  Serial.println("USB Clock Control Register");
  PRINT_REG(R_SYSTEM, USBCKCR);Serial.println();
}