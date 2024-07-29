/* Questo programma fa uscire sul pin P109 (D11 su UNO MINIMA R4) uno dei clock disponibili al processore, in modo da poterne misurare la frequenza esatta con un oscilloscopio
Original project is http://radiopench.blog96.fc2.com/blog-date-20240215.html*/
 
char cbuff[10];  // 文字列操作バッファ
 
void setup() {
  set48MhzClock();
 // rtcSOSC(); // IF you wont check external SOSC frequency with oscilloscope on P109 (D11)
  rtcLOCO(); // If you wont check LOCO frequency with oscilloscope on P109 (D11)
  Serial.begin(9600);
  delay(2000);
  Serial.println();
  Serial.println("Start !");
 
  // CKCORを設定(クロックソースと分周比を設定)
  R_SYSTEM->PRCR = 0xA501;           // レジスタプロテクト解除
  R_SYSTEM->CKOCR = 0;               // CKOCRの全ビットクリア
  R_SYSTEM->CKOCR_b.CKOSEL = 0b010;  // HOCO:000, MOCO:001, LOCO:010, MOSC:011, SOSC:100
  R_SYSTEM->CKOCR_b.CKODIV = 0b000;  // Division:000 x1, 001 /2, 010 /4, 011 /8, 100 /16, 101 /32, 110 /64,111 /128
  R_SYSTEM->CKOCR_b.CKOEN = 1;       // クロックアウト許可
  R_SYSTEM->PRCR = 0xA500;           // レジスタを再プロテクト
  viewCKOCR();
 
  // D11ポートの設定（CKOUTが出力出来るように設定）
  R_PMISC->PWPR_b.B0WI = 0;                       // 書き込みプロテクトを、
  R_PMISC->PWPR_b.PFSWE = 1;                      // 外す
  R_PFS->PORT[1].PIN[9].PmnPFS = 0;               // 念のために設定をリセット
  R_PFS->PORT[1].PIN[9].PmnPFS_b.PDR = 1;         // D11(P109)を出力に設定
  R_PFS->PORT[1].PIN[9].PmnPFS_b.PSEL = 0b01001;  // CLKOUTを選択
  R_PFS->PORT[1].PIN[9].PmnPFS_b.PMR = 1;         // 周辺機能をON
  R_PMISC->PWPR_b.PFSWE = 0;                      // 書き込みプロテクトを、
  R_PMISC->PWPR_b.B0WI = 1;                       // 掛ける
  viewD11();                                      // ちゃんと設定出来たか確認
}
 
void loop() {
}

void set48MhzClock(){ // WITH Externel Quartz of 16Mhz AND PLL
  R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_CGC); // Disable register protection
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
R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_CGC);// Enable register protection
}

void set16MhzClock(){ // WITH Externel Quartz of 16Mhz NO PLL
  R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_CGC);// Disable register protection
  R_SYSTEM->MOSCCR_b.MOSTP = 1;   // SET MOSTP bit to 1 to stop Main Clock Oscillator
  asm volatile ( "dsb" );         // Data bus Synchronization instruction (?)
  R_SYSTEM->MOMCR_b.MODRV1 = 0;   // MODRV1 = 0 (10 MHz to 20 MHz) becouse external quartz is 16Mhz
  R_SYSTEM->MOSCWTCR       = 0x07;// Set stability timeout period (?)
  R_SYSTEM->MOSCCR_b.MOSTP = 0;   // Enable XTAL
  R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); // wait for XTAL to stabilise
  R_SYSTEM->PLLCR_b.PLLSTP = 1;   // Disable PLL
  R_SYSTEM->MOSCCR_b.MOSTP = 0;   // Enable XTAL
  R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); // wait for XTAL to stabilise
  R_SYSTEM->SCKSCR  = 0x03; // Select MOSC as system clock
  R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_CGC);// Enable register protection
}

void set48MhzHOCO(){ // Internal 48Mhz as MAIN OSC
  R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_CGC);// Disable register protection
  R_SYSTEM->MOSCCR_b.MOSTP = 1;   // SET MOSTP bit to 1 to stop Main Clock Oscillator
  asm volatile ( "dsb" );         // Data bus Synchronization instruction (?)
  R_SYSTEM->MOMCR_b.MODRV1 = 1;   // MODRV1 = 1 (1 MHz to 10 MHz)
  R_SYSTEM->MOSCWTCR       = 0x09;// Set stability timeout period
  R_SYSTEM->SCKSCR = 0;           //Select High-speed on-chip oscillator (HOCO) as system clock
  R_SYSTEM->PLLCR_b.PLLSTP = 1;   // Disable PLL
  R_SYSTEM->MOSCCR_b.MOSTP = 1;   // Disable Main clock oscillator (MOSC) as system clock
  R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); // wait for XTAL to stabilise
  R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_CGC);// Enable register protection
}



static void rtcLOCO ( void ) { //Switch to internal LOCO (24.3.2 Clock and Count Mode Setting Procedure)
  //-----use LOCOUTCR and RFRL to calibrate frequency
  R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_CGC);
  R_SYSTEM->SOSCCR   = 0x01;                               // Stop the sub-clock oscillator
  asm volatile ( "dsb" );                                  // Data bus Synchronization instruction
  R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
  R_SYSTEM->SOSCCR_b.SOSTP   = 1;                         //STOP EXT 32.768
  R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); // wait for XTAL to stabilise
  R_SYSTEM->LOCOUTCR   = 0;                               //Trim LOCO clock 1LSB=3.7%
  R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_CGC);
  //----------------------And now config RTC register
  R_RTC->RCR4_b.RCKSEL       =1;      //Select LOCO
  delay(10);
  R_RTC->RCR2_b.START = 0;            // Temporarily stop RTC
  while (R_RTC->RCR2_b.START == 1) {  // Wait until RTC stops
  }
  R_RTC->RFRH         = 0;         // Upper division ratio setting register setting (this is always zero)
  R_RTC->RFRL          = 0xff;      // RTC clock division ratio setting, 1LSB=0.39% (1/256)
  R_RTC->RCR2_b.CNTMD  = 0;         // 0=Calendar count mode 1=Binary count mode
  while (R_RTC->RCR2_b.CNTMD == 1) {  // Confirm CNTMD status
  }
  R_RTC->RCR2_b.RESET = 1;            // Restart RTC
  while (R_RTC->RCR2_b.RESET == 1) {  // Wait until RTC restarts
  }
  
}


static void rtcSOSC ( void ) { // Swith to external 32.768Khz Sub-OScillator
  R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_CGC);
  R_SYSTEM->SOSCCR   = 0x01;       // Stop the sub-clock oscillator
  asm volatile ( "dsb" );          // Data bus Synchronization instruction
  while ( 1 != R_SYSTEM->SOSCCR ) R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
  R_SYSTEM->SOMCR       = 0x00;    // Sub-Clock Oscillator Drive in Normal Mode    
  R_SYSTEM->SOSCCR_b.SOSTP = 0;    // START sub-clock oscillator
  //R_RTC->RCR4_b.RCKSEL  = 0;       //Select EXT 32.768 SOSC into RTC module
  R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); // wait for XTAL to stabilise 
  R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_CGC);
  //----------------------And now config RTC register
  R_RTC->RCR4_b.RCKSEL       =0; //Select LOCO
  delay(10);
  R_RTC->RCR2_b.START = 0;            // Temporarily stop RTC
  while (R_RTC->RCR2_b.START == 1) {  // Wait until RTC stops
  }
  /* RADJ register is cleared to 00h by an RTC software reset.*/
  R_RTC->RADJ         = 0;   // Clear the correction amount
  /*
  R_RTC->RCR2_b.AADJE = 0;   // Stop correction
  delay(10);
  R_RTC->RCR2_b.AADJP = 1;  // Correction period 10 seconds (0:1 minute, 1:10 second) *Adjustment required
  delay(10);
  R_RTC->RCR2_b.AADJE = 1;     // Execute correction (RCR2 setting complete)
  R_RTC->RADJ         = 0x96;  // Batch write PMADJ=0b10, ADJ=22 (*Adjustment required)
  // ADJ value is 1/(32768*10) = 3.052ppm / LSB (for 10 second period)
  // In this example, there was an error of 67.6ppm, so the correction amount = 22
  // Upper limit of ADJ value is 63 (6 bits)
  */
  R_RTC->RCR2_b.CNTMD  = 0;          // 0=Calendar count mode 1=Binary count mode
  while (R_RTC->RCR2_b.CNTMD == 1) {  // Confirm CNTMD status
  }

  R_RTC->RCR2_b.RESET = 1;            // Restart RTC
  while (R_RTC->RCR2_b.RESET == 1) {  // Wait until RTC restarts
  }
  
}
 
void viewCKOCR() {  // CKOCRの内容を出力
  sprintf(cbuff, "CKOCR     = 0x%02x", R_SYSTEM->CKOCR);
  Serial.println(cbuff);
}
 
void viewD11() {  // D11のポート設定を出力
  sprintf(cbuff, "D11(P109) = 0x%08lx", R_PFS->PORT[1].PIN[9].PmnPFS);
  Serial.println(cbuff);
}