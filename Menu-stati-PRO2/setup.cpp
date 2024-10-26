#include "setup.h"

void myMenu0(struct variables *var){
if(var->first){
 Serial.print("Menu ");Serial.println(var->current);
 var->first = false;
 }
 encButton();
}


void myMenu1(struct variables *var){
if(var->first){
 Serial.print("Menu ");Serial.println(var->current);
 var->deviceID = 1234;                                   // Example to use a member of struct variables
 Serial.print("devID: ");Serial.println (var->deviceID); // If compiler return error change Serial.println(st->deviceid) with Serial.println(String(st->deviceid))
 var->first = false;
 }
 encButton();
}

void myMenu2(struct variables *var){
if(var->first){
 Serial.print("Menu ");Serial.println(var->current);
  var->first = false;
 }
 encButton();
}


void encButton(){ // Check state of encoder button
static uint8_t pvbt;
uint8_t bt = !digitalRead(ENT_B); // invertito perchè in pull down (collegato a gnd)
if (bt && !pvbt){
  Serial.println("Fronte di salita");
  delay(200);
}
if (bt && pvbt){
  Serial.println(".");
  delay(200);
}
if (!bt && pvbt){
  Serial.println("Fronte di discesa");
  delay(200);
}
pvbt = bt;
}

//-------------This function to set 48Mhz with external quartz of RA4M1 (only if your board have external 16Mhz OSC, in this case I use Weact Studio RA4M1 board)
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

void rtcSOSC() { // Swith to external 32.768Khz Sub-OScillator (24.3.3 User Manual)
  R_RTC->RCR4_b.RCKSEL = 0; //Select SOSC
  delay(10);
  R_RTC->RCR2_b.START = 0;            // Temporarily stop RTC
  while (R_RTC->RCR2_b.START == 1) {}  // Wait until RTC stops
  R_RTC->RCR2_b.RESET = 1;            // Restart RTC
  while (R_RTC->RCR2_b.RESET == 1) {  // Wait until RTC restarts
  //adj60SEC();
  }
}

void adj1SEC(){ // Adjust crystal ticks every 1 sec.
R_RTC->RADJ_b.PMADJ = 0;
R_RTC->RCR2_b.AADJE = 0;
R_RTC->RADJ_b.PMADJ = 0b10; // 10: subtraction - 01: addition
R_RTC->RADJ_b.ADJ = 22; // tick of crystal (Max 63)
}

void adj10SEC(){ // Adjust crystal ticks every 10 sec.
 R_RTC->RADJ_b.PMADJ = 0;
 R_RTC->RCR2_b.AADJE = 1;
 R_RTC->RCR2_b.AADJP = 1; // 1: 10sec.  0: 60sec.
 R_RTC->RADJ_b.PMADJ = 0b10; // 10: subtraction - 01: addition
 R_RTC->RADJ_b.ADJ = 22; // tick of crystal (Max 63)
}

void adj60SEC(){ // Adjust crystal ticks every 1 min.
 R_RTC->RADJ_b.PMADJ = 0;
 R_RTC->RCR2_b.AADJE = 1;
 R_RTC->RCR2_b.AADJP = 0; // 1: 10sec.  0: 60sec.
 R_RTC->RADJ_b.PMADJ = 0b01; // 10: subtraction - 01: addition
 R_RTC->RADJ_b.ADJ = 45; // tick of crystal (Max 63)
}
