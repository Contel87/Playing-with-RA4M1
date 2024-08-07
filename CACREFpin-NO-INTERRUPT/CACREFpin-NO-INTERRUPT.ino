/* Programma NON TESTATO - attenzione ai livelli logici ... se in ingresso ci sono 5V, bisogna usare una scheda a 5V oppure una da 3.3V con adattatore logico!!!!!!
Questo programma prende in ingresso dal pin (P204) un segnale di clock esterno e lo compara con un segnale di clock interno, il risultato viene visualizzato su un lcd NOKIA 5110
La differenza di tick tra il segnale in ingresso da confrontare (configurato nel CACCR2) e il segnale di clock interno (configurato nel CACCR1) viene
visualizzata nel registro CACNTBR ... Se si confronta un segnale di quarzetto esterno a 32.768Khz con i 48Mhz del processore si avra' 48,000,000 / 32,768 == 1464.84375
Puo' essere utile per calibrare una frequenza esterna prendendo come riferimento uno dei clock dell' RA4M1.
*/


// (C)2016-18 Pawel A. Hernik
// N5110 LCD pinout from left:
// #1 RST      - Pin  9 or any digital
// #2 CS/CE    - Pin 10 or any digital
// #3 DC       - Pin  8 or any digital
// #4 MOSI/DIN - Pin 11 / SPI
// #5 SCK/CLK  - Pin 13 / SPI
// #6 VCC 3.3V
// #7 LIGHT (200ohm to GND)
// #8 GND

#include "N5110_SPI.h"
N5110_SPI lcd(9,10,8); // RST,CS,DC
#include <SPI.h>


//#include "fonts_all.h"c:\Users\conte\Documents\Arduino\LCD5110\LCD5110.ino
#include "small5x7bold_font.h"
//#include "term8x14_font.h"
//#include "digits4x7_font.h"

#define PERIOD 1000 // lcd update time

char cBuff[10]; // Buffer to store register debug

void setup() 
{
  Serial.begin(9600);
  delay(2000);
  set48MhzClock(); // Select external MOSC 48Mhz
  rtcSOSC(); // Select external SOSC
  setCACREFpin(); // Set pin P204 as input pin (CACREF)
  configCAC(); // Configure CAC module
  startCAC(); // Start freq match
  lcd.init(); // Initialize lcd
  lcd.clrScr(); // Clear lcd screen
  lcd.setFont(Small5x7PLBold); // Select lcd fonts
  lcd.setDigitMinWd(0); // Distance between words on lcd
}


void loop() 
{
  showCAC();
}

void showCAC(){
  static uint32_t last = millis();
  uint32_t now = millis();
  // Print frequency diff every PERIOD. 
  if(now - last >= PERIOD){
    last += PERIOD;
    lcd.printStr(15, 1, "CACNTBR :");
    Serial.print("MENDIE: "); Serial.println(R_CAC->CASTR_b.MENDF );  // Confronto al cambio di stato dei 2 segnali avvenuto
    Serial.print("FERRIE: "); Serial.println(R_CAC->CASTR_b.FERRF );  // Il valore misurato e' fuori dalle soglie MIN e MAX
    Serial.print("OVERFLOW: "); Serial.println(R_CAC->CAICR_b.OVFIE); // Overflow
    viewCACNTBR(); // Mostra il valore dei 2 segnali messi a confronto ... il valore teorico dovrebbe essere 1465 (48MHz / 32.768Khz)
    //stopCAC();
      }

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

static void rtcSOSC ( void ) { // Swith to external 32.768Khz Sub-OScillator
  R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_CGC);
  R_SYSTEM->SOSCCR   = 0x01;       // Stop the sub-clock oscillator
  asm volatile ( "dsb" );          // Data bus Synchronization instruction
  while ( 1 != R_SYSTEM->SOSCCR ) R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
  R_SYSTEM->SOMCR       = 0x00;    // Sub-Clock Oscillator Drive in Normal Mode    
  R_SYSTEM->SOSCCR_b.SOSTP = 0;    // START sub-clock oscillator
  R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); // wait for XTAL to stabilise 
  R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_CGC);
  
  //----------------------And now config RTC register
  R_RTC->RCR4_b.RCKSEL       =0; //Select SOSC
  delay(10);
  R_RTC->RCR2_b.START = 0;            // Temporarily stop RTC
  while (R_RTC->RCR2_b.START == 1) {  // Wait until RTC stops
  }
  R_RTC->RCR2_b.CNTMD  = 0;          // 0=Calendar count mode 1=Binary count mode
  while (R_RTC->RCR2_b.CNTMD == 1) {  // Confirm CNTMD status
  }
  R_RTC->RCR2_b.RESET = 1;            // Temporarily stop RTC
  while (R_RTC->RCR2_b.RESET == 1) {  // Wait until RTC stops
  }
  //----------------------And now adjust SOSC error (RADJ register is cleared to 00h by an RTC software reset)
  R_RTC->RCR2_b.AADJE = 0;   // Stop correction
  delay(10);
  R_RTC->RCR2_b.AADJP = 1;  // Correction period 10 seconds (0:1 minute, 1:10 second) *Adjustment required
  delay(10);
  R_RTC->RCR2_b.AADJE = 1;     // Execute correction (RCR2 setting complete)
  //R_RTC->RADJ         = 0x96;  // Batch write PMADJ=0b10, ADJ=22 (*Adjustment required)
  R_RTC->RADJ_b.PMADJ = B10; // 10 = Adjustment is performed by the subtraction from the prescaler, 01 = Adjustment is performed by the addition to the prescaler
  delay(10);
  R_RTC->RADJ_b.ADJ = B10110; // 22 clock cycle every second
  delay(10);
  // ADJ value is 1/(32768*10) = 3.052ppm / LSB (for 10 second period)
  // In this example, there was an error of 67.6ppm, so the correction amount = 22
  // Upper limit of ADJ value is 63 (6 bits)
}

void setCACREFpin(){  // pin P204 (UNO R4 MINIMA LOVE) - si puo' usare anche il P400
  R_PMISC->PWPR_b.B0WI = 0;                       // 書き込みプロテクトを、
  R_PMISC->PWPR_b.PFSWE = 1;                      // 外す
  R_PFS->PORT[2].PIN[4].PmnPFS = 0;               // 念のために設定をリセット
  R_PFS->PORT[2].PIN[4].PmnPFS_b.PDR = 0;         // 0: Input (functions as an input pin)
  R_PFS->PORT[2].PIN[4].PmnPFS_b.PSEL = 0b01010;  // pin as CACREF
  R_PFS->PORT[2].PIN[4].PmnPFS_b.PMR = 1;         // 1: Used as an I/O port for peripheral functions - 0: Used as a general I/O pin
  R_PMISC->PWPR_b.PFSWE = 0;                      // 書き込みプロテクトを、
  R_PMISC->PWPR_b.B0WI = 1;                       // 掛ける
  viewP204();                                      // ちゃんと設定出来たか確認
}

void configCAC(){
  R_MSTP->MSTPCRC &= ~0x1UL; // Permette di scrivere sui registri CALLVR e CAULVR
  R_CAC->CALLVR = 1455;      // Soglia MIN
  R_CAC->CAULVR = 1475;      // Soglia MAX
  //R_CAC->CACR1 = 0x34;       // Imposta l'oscillatore interno HOCO (48Mhz) come frequenza Targhet
  R_CAC->CACR1 = 0x30;       // Imposta l'oscillatore esterno MOSC (48Mhz) come frequenza Targhet (con ratio 1/32)
  // Digital filter in CACR2??
  //R_CAC->CACR2 = 0x09;     // Imposta l'oscillatore interno LOCO (32.768Khz) con ratio 1/32 come segnale da confrontare (48,000,000 / 32,768 == 1464.84375)
  R_CAC->CACR2 = 0;          // CACREF pin input (puo' essere P204 oppure P400)     
  R_CAC->CAICR = 0x70;       // Disabilita l'uso degli interrupt globali
}

void setCACREFpin(){
// Set CACREFPIN as input reference (P204 or P400)
  R_PMISC->PWPR_b.B0WI = 0;                       // Writing to the PFSWE bit enabled
  R_PMISC->PWPR_b.PFSWE = 1;                      // Writing to the PmnPFS register enabled.
  R_PFS->PORT[2].PIN[4].PmnPFS = 0;               // Reset settings
  R_PFS->PORT[2].PIN[4].PmnPFS_b.PDR = 0;         // 0: Input (functions as an input pin) - 1: Output (functions as an output pin)
  R_PFS->PORT[2].PIN[4].PmnPFS_b.PSEL = 0b01010;  // CACREF pin
  R_PFS->PORT[2].PIN[4].PmnPFS_b.PMR = 1;         // 0: Used as a general I/O pin - 1: Used as an I/O port for peripheral functions
  R_PMISC->PWPR_b.PFSWE = 0;                      // Writing to the PmnPFS register disabled
  R_PMISC->PWPR_b.B0WI = 1;                       // Writing to the PFSWE bit disabled
  //viewP204();
  //viewP400();
}

void startCAC(){
  R_CAC->CACR0_b.CFME = 1; //Enable CAC counter
}

void stopCAC(){
  R_CAC->CACR0_b.CFME = 0; //Disable CAC counter
}

void viewCACNTBR(){
  sprintf(cBuff, "CACNTBR = %04d", R_CAC->CACNTBR);
  lcd.printStr(15, 2, cBuff);
  Serial.println(cBuff);
}

void viewP204(){
  sprintf(cBuff, "P204 = 0x%08lx", R_PFS->PORT[2].PIN[4].PmnPFS);
  Serial.println(cBuff);
}

