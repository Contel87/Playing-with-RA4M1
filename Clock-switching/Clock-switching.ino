/*This program switch ckocks of RA4M1, useful if connected external oscillators*/


void setup() {
  Serial.begin(9600);
  delay(2000);
  set48MhzClock();

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
  R_SYSTEM->SCKSCR = 0x03; //Select Main clock oscillator (MOSC) as system clock
  //R_SYSTEM->PLLCR_b.PLLSTP = 1;   // Disable PLL
  //R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
  R_SYSTEM->MOSCCR_b.MOSTP = 0;   // Enable XTAL
  R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); // wait for XTAL to stabilise
  R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_CGC);
}

static void rtcLOCO ( void ) { //Switch to internal LOCO
  R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_CGC);
  R_SYSTEM->SOSCCR   = 0x01;          /* Stop the sub-clock oscillator */
  asm volatile ( "dsb" );             /* Data bus Synchronization instruction */
  R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
  R_SYSTEM->SOSCCR_b.SOSTP   = 1; //STOP EXT 32.768
  R_RTC->RCR4_b.RCKSEL       =1; //Select LOCO
  R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); /* wait for XTAL to stabilise */
  R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_CGC);
}


static void rtcSOSC ( void ) { // Swith to external 32.768Khz Sub-OScillator
  R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_CGC);
  R_SYSTEM->SOSCCR   = 0x01;          /* Stop the sub-clock oscillator */
  asm volatile ( "dsb" );             /* Data bus Synchronization instruction */
  while ( 1 != R_SYSTEM->SOSCCR ) R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
  R_SYSTEM->SOMCR    = 0x00;          /* Sub-Clock Oscillator Drive in Normal Mode */
  R_SYSTEM->SOSCCR   = 0x00;          /* Operate the sub-clock oscillator */ // OPPOSITE OF R_SYSTEM->HOCOCR R_SYSTEM->MOCOCR (check su OSCSF)
  R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); /* wait for XTAL to stabilise */
  R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_CGC);
}