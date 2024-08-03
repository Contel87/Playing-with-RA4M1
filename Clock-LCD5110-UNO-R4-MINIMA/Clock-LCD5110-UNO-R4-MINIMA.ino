/*Questo programma utilizza un display LCD di un vecchio Nokia 5110 collegato ad una board "WEACT STUDIO RA4M1"
Sulla board è stato caricato il bootloader della UNO R4 e 3 pulsanti che collegano a GND i pin P103 (D4 arduino UNO), P102 (D5 arduino UNO) e P106 (D6 arduino UNO)
Il progetto originale e' tradotto da un blog giapponese: http://radiopench.blog96.fc2.com/blog-date-20240408.html
Per l'LCD è trata utilizzata la libreria N5110_SPI di @cbm80amiga: https://github.com/cbm80amiga/N5110_SPI
*/


// (C)2016-18 Pawel A. Hernik
// N5110 LCD pinout from left:
// #1 RST      - Pin  9 or any digital
// #2 CS/CE    - Pin 10 or any digital
// #3 DC       - Pin  8 or any digital
// #4 MOSI/DIN - Pin 11 / SPI
// #5 SCK/CLK  - Pin 13 / SPI
// #6 VCC 3.3V
// #7 LIGHT (200ohm to GND) - If need power saving don't connect this pin to RA4M1 
// #8 GND


  

#include "N5110_SPI.h" // PCD8544 lcd controller library
#include <RTC.h>       // RTC library
#include <SPI.h>       // SPI library
N5110_SPI lcd(9,10,8); // RST,CS,DC lcd connections pins

//----------------PRINT_REG command is useful to check RA4M1 register status

#define PRINT_REG(BUCKET, REGISTER) \
  do { \
    uint32_t t = BUCKET->REGISTER; \
    Serial.print(#REGISTER " : b"); \
    Serial.println(t, BIN); \
  } while (false)


#define VER_NUM        "V1.0"  // Release version number
#define DEC_B          4       // Decrement button Pin
#define INC_B          5       // Increment button Pin
#define ENT_B          6       // Enter button Pin


#include "small5x7bold_font.h" // Fonts to display date
#include "term8x14_font.h"     // Fonts to display time


char   cBuff[50];               // String operation buffer
String ddmmyy = "dd/mm/yyyy";   // day, month, and Year string
String hhmmss = "hh:mm/ss";     // Hour, minute, and second string
int    dd, mo, yy, hh, mi, ss;  // Time element

String dispWeekName[] = {  // Traslate day of the week in ITAlian languge
  "Domenica", "Lunedi", "Martedi", "Mercoledi", "Giovedi", "Venerdi", "Sabato"
};


volatile bool irqFlag = false; // Flag used into loop to blink led every second
RTCTime savedTime;    // For saving RTC time
RTCTime currentTime;  // For time setting
RTCTime mytime;       // For time setting




void setup() 
{  //----------------Config Serial
  Serial.begin(9600);
  delay(2000);

  Serial.println("Registri accensione:"); // Check RTC register before 
  viewRTCregister2();                    // changing it
  set48MhzClock();                         // Set 48Mhz main clock with external 16Mhz chrystal osc and PLL
  pinMode(DEC_B, INPUT_PULLUP);            // Dec.button
  pinMode(INC_B, INPUT_PULLUP);            // Inc.button and 30 second Adjust
  pinMode(ENT_B, INPUT_PULLUP);            // For time setting
  pinMode(21, OUTPUT);                     // On board led
  // ----------------Power ON LCD
  lcd.init();                              // Initialize LCD
  startScreen();                           // Startup screen
  Serial.println();                        // Open one line of serial to display the starting position
  Serial.println("Arduino UNO R4 RTC start");// Start message (also serves as a serial communication check)
  RTC.begin();               // Start RTC library
  rtcSOSC();                 // Select External 32.768Khz quartz AFTER RTC.begin()!!!
  RTC.getTime(savedTime);    // Try reading the RTC time
  Serial.print("t1 = ");     // Display value serially
  Serial.println(savedTime); // Before RTC starts, 2000/1/1 0:0:0
  
  // ------  If the ENT button is long pressed at startup, set manually date and time with buttons
  if (digitalRead(ENT_B) == LOW) { 
    clockAdjust();  // Change date and time with buttons
  } else {                   // Normal startup if ENT button is not pressed
    adjustSec();          // Advance savedTime by 0.5 seconds on average
    RTC.setTime(savedTime);  // Inherit the startup time (RTC will not start if you do not do this)
  }
  dispTime();               // Screen display

  if (!RTC.setPeriodicCallback(periodicCallback, Period::ONCE_EVERY_1_SEC)) {  // Set interrupt at 1 second period
    Serial.println("ERROR: periodic callback not set");                        // Report if it fails
  }
  Serial.print("Reg. set : ");  // At this point,
  Serial.println("Registri dopo setup:");
  viewRTCregister2();           // Dump RTC related registers
  Serial.println("Clock start");
}



void loop() 
{
    String wk;
  while (irqFlag == false) {                // Wait for RTC 1 second interrupt
    if (digitalRead(INC_B) == LOW) {        // If the 30 seconds adjustment button is pressed
      adjustZeroSec();                      // Execute second hand adjustment (30 second adjustment)
      dispTime();                           // Reflect the result on the display
      while (digitalRead(INC_B) == LOW) {}  // Wait until button is released
    }
  }
  irqFlag = false;         // An interrupt has occurred, so
  digitalWrite(21, HIGH);  // LED lights up
  dispTime();              // Display current time on LCD
  digitalWrite(21, LOW);   // LED off
}

void startScreen() {         // LCD display at start
lcd.setFont(Small5x7PLBold); // Set LCD Fonts
  lcd.setDigitMinWd(7);      // LCD character distance
  lcd.clrScr();              // Clear LCD
  // ---------------Display project info on LCD
  lcd.printStr(0, 0, " UNO R4 clock");
  lcd.printStr(0, 1, "Version #");
  lcd.printStr(55, 1, VER_NUM);
  lcd.printStr(2, 3, "Source :");
  (R_RTC->RCR4_b.RCKSEL == 0) ? lcd.printStr(0, 4, "External SOSC") : lcd.printStr(55, 4, "LOCO");
}




void getDateTime() {  // Create date and time string from RTC value
  ddmmyy = "";
  hhmmss = "";  
  dd = savedTime.getDayOfMonth();
  sprintf(cBuff, "%02d", dd);  // dd is 2 digits value of day
  ddmmyy += cBuff;             
  mo = Month2int(savedTime.getMonth());
  sprintf(cBuff, "/%02d", mo);  // / followed by 2 digits to the right
  ddmmyy += cBuff;
  yy = savedTime.getYear() - 2000;  // / The value of y is the last two digits of the year value (+2000 when writing back)
  sprintf(cBuff, "/20%02d", yy);    // 20 followed by two right-justified digits
  ddmmyy += cBuff;                  // Year, month, and day string completed (ex:2019/02/28)
  hh = savedTime.getHour();
  sprintf(cBuff, "%02d", hh);       // Right justified 2 digits
  hhmmss += cBuff;
  mi = savedTime.getMinutes();
  sprintf(cBuff, ":%02d", mi);  // : followed by two right-justified digits,
  hhmmss += cBuff;
  ss = savedTime.getSeconds();  // seconds
  sprintf(cBuff, ":%02d", ss);  // : followed by 2 digits to the right
  hhmmss += cBuff;              // Complete string of hours, minutes, and seconds (ex:01:02:03)
}

void clockAdjust() {                 // Adjust the time using LCD and button switch
  lcd.clrScr();                      // Clear LCD screen
  lcd.setFont(Small5x7PLBold);       // Set Fonts for date
  lcd.setDigitMinWd(10);             // Set distance between words on LCD
  lcd.printStr(12, 2, "Clock ADJ."); // Time adjustment start display
  while (digitalRead(ENT_B) == LOW) {// Wait until ent button is released
  }
  // ------- Display on LCD and serial date and time to setting with buttons

  getDateTime();  // Get the current time from the built-in RTC

  lcd.clrScr();         // Clear LCD screen
  lcd.setDigitMinWd(0); // Set distance between words on LCD

  sprintf(cBuff, "%02d/", dd);  
  lcd.printStr(16, 1, cBuff);   // Display the current day

  sprintf(cBuff, "%02d/", mo);  
  lcd.printStr(33, 1, cBuff);  // Display the current month

  lcd.printStr(50, 1, "20");   // Display the current date
  
  Serial.println(ddmmyy);     // Also output to serial
  
  hhmmss[6] = '-';  // Delete the seconds digit with --
  hhmmss[7] = '-';
  
  lcd.setFont(Term8x14PL);     // Set Fonts for hour, minuts and seconds    
  sprintf(cBuff, "%02d:", hh);  
  lcd.printStr(15, 3, cBuff);  // Display hours

  sprintf(cBuff, "%02d:", mi);  
  lcd.printStr(36, 3, cBuff);  // Display minuts
  
  lcd.printStr(57, 3, "--"); // Display seconds

  Serial.println(hhmmss); // Also output date and time to serial

  // ------ Now we can change Date and time on display with buttons
  lcd.setFont(Small5x7PLBold); // Set date font

 // ------- Use inc. and dec. button to change year
  lcd.fillWin(50,2,22,1,0xa); // Underline year
  yy = oledRW(62, 1, yy, 1, 20, 60);  // Specify the x, y coordinates of LCD, value of year, step, lower limit, and upper limit, and enter the setting values ​​while looking at the screen
  lcd.fillWin(50,2,22,1,0); // Switch OFF underline yy
  
  // -------After year changed, if button ENT. is pressed, move underline to month
  lcd.fillWin(33,2,11,1,0xa); // Underline month
  mo = oledRW(33, 1, mo, 1, 1, 12);   // Specify the x, y coordinates of LCD, value of month, step, lower limit, and upper limit, and enter the setting values ​​while looking at the screen
  lcd.fillWin(33,2,11,1,0); // Switch OFF underline month

  lcd.fillWin(16,2,11,1,0xa); // Underline day
  if (mo == 2) {                                                   //  In February,
    if ((yy % 4) == 0) {                                           //  If the year is divisible by 4
      dd = oledRW(16, 1, dd, 1, 1, 29);                            //  Until the 29th because it is a leap year
    } else {                                                       //  If it is not a leap year,
      dd = oledRW(16, 1, dd, 1, 1, 28);                            // Until the 28th
    }                                                              //
  } else if ((mo == 4) || (mo == 6) || (mo == 9) || (mo == 11)) {  //  For April, June, September, November
    dd = oledRW(16, 1, dd, 1, 1, 30);                              //  Until the 30th
  } else {                                                         //
    dd = oledRW(16, 1, dd, 1, 1, 31);                              // Unless above, until the 31st
  }                                                                //
  lcd.fillWin(16,2,11,1,0); // Switch OFF underline day

// -------After day changed, if button ENT. is pressed, move underline to hour
  lcd.setFont(Term8x14PL); // Set time font
  lcd.fillWin(15,5,17,1,0xa); //  Underline hour
  hh = oledRW(15, 3, hh, 1, 0, 23);                               // Set hour
  lcd.fillWin(15,5,17,1,0);// Switch OFF underline hour

 // -------After hour changed, if button ENT. is pressed, move underline to minuts
  lcd.fillWin(36,5,17,1,0xa); //  Underline minuts
  mi = oledRW(36, 3, mi, 1, 0, 59);                               // Set minuts
  lcd.fillWin(36,5,17,1,0); // Switch OFF underline minuts

  setDMYHMSW(dd, mo, yy + 2000, hh, mi, 0);  // Convert the set time to RTC format
  RTC.setTime(mytime);                       // Write the value of mytime to RTC
  adjustZeroSec();                           // Execute 30 second adjustment to start in zero seconds
}

int oledRW(int x, int y, int d, int stepD, int minD, int maxD) {  // Enter values ​​from LCD
  ///Display the value of the variable at the specified position on the LCD, right justified by 2 digits. Increase or decrease the value by button operation,
  // Confirm the value with Ent input and return it as the return value. Specify the top left of the display position using x, y coordinates
  // The operation position is underlined. Values ​​circulate within the upper and lower limits.
  // Arguments: x coordinate, y coordinate, value to change, change step amount, lower limit value, upper limit value

  oledDisp2Chr(x, y, d);               // Display the initial value in 2 digits at the specified position on the screen (underlined)
  while (digitalRead(ENT_B) == LOW) {  // If the enter button is pressed, wait until it is released
  }
  delay(30);
  while (digitalRead(ENT_B) == HIGH) {  // Run the following until the enter button is pressed
    if (digitalRead(INC_B) == 0) {      // If the + button is pressed
      d = d + stepD;                    // Increase x by the specified step
      if (d > maxD) {                   // If the upper limit is exceeded, succulate to the lower limit
        d = minD;
      }
      oledDisp2Chr(x, y, d);             // Display a 2-digit number (underlined) at the specified position on the screen
      while (digitalRead(INC_B) == 0) {  // Wait until + button is released
      }
      delay(30);
    }

    if (digitalRead(DEC_B) == 0) {  // - If the button is pressed
      d = d - stepD;                // Decrease x by specified step
      if (d < minD) {               // If below the lower limit, circulate to the upper limit
        d = maxD;
      }
      oledDisp2Chr(x, y, d);             // Display a 2-digit number (underlined) at the specified position on the screen
      while (digitalRead(DEC_B) == 0) {  // - wait until button is released
      }
      delay(30);
    }
  }
  delay(30);
  return d;  // Return the set value as the return value
}

void oledDisp2Chr(int x, int y, int val) {       //Display a 2-digit value at the specified location on the LCD
  sprintf(cBuff, "%02d", val);                   // Convert data to decimal 2-digit 0 fill string
  lcd.printStr(x, y, cBuff);                     // Move the cursor to the specified position and Display the number on screen
}

// Set time to RTC of UNO R4
void setDMYHMSW(int dd, int mm, int yy, int th, int tm, int ts) {  // Set RTC with the year, month, day, hour, minute, and second values ​​(also set the day of the week)
  int wkN = 0;                                                     // Sequential number of days of the week, Sunday = 0
  mytime.setDayOfMonth(dd);  // Set the day
  switch (mm) {                                                     // set month
    case 1: mytime.setMonthOfYear(Month::JANUARY); break;          // JANUALY if mm is 1.
    case 2: mytime.setMonthOfYear(Month::FEBRUARY); break;         // February
    case 3: mytime.setMonthOfYear(Month::MARCH); break;            // ↓
    case 4: mytime.setMonthOfYear(Month::APRIL); break;
    case 5: mytime.setMonthOfYear(Month::MAY); break;
    case 6: mytime.setMonthOfYear(Month::JUNE); break;
    case 7: mytime.setMonthOfYear(Month::JULY); break;
    case 8: mytime.setMonthOfYear(Month::AUGUST); break;
    case 9: mytime.setMonthOfYear(Month::SEPTEMBER); break;
    case 10: mytime.setMonthOfYear(Month::OCTOBER); break;
    case 11: mytime.setMonthOfYear(Month::NOVEMBER); break;
    case 12: mytime.setMonthOfYear(Month::DECEMBER); break;
    default: break;
  }
  mytime.setYear(yy);    // Set the year                                           
  mytime.setHour(th);    // hour
  mytime.setMinute(tm);  // minute
  mytime.setSecond(ts);  // seconds

  // Find the consecutive days of the week using Zeller's formula
  if (mm <= 2) {   // January and February are
    yy--;         // previous year's
    mm += 12;     // Calculated as 13,14 months
  }
  wkN = (yy + yy / 4 - yy / 100 + yy / 400 + (13 * mm + 8) / 5 + dd) % 7;
  Serial.print("Week serial num = ");
  Serial.println(wkN);
  switch (wkN) {  // Set days of the week using sequential numbers
    case 0: mytime.setDayOfWeek(DayOfWeek::SUNDAY); break;
    case 1: mytime.setDayOfWeek(DayOfWeek::MONDAY); break;
    case 2: mytime.setDayOfWeek(DayOfWeek::TUESDAY); break;
    case 3: mytime.setDayOfWeek(DayOfWeek::WEDNESDAY); break;
    case 4: mytime.setDayOfWeek(DayOfWeek::THURSDAY); break;
    case 5: mytime.setDayOfWeek(DayOfWeek::FRIDAY); break;
    case 6: mytime.setDayOfWeek(DayOfWeek::SATURDAY); break;
    default: break;
  }
}

void dispTime() {            // Display time on serial and LCD
  String wk;                 // For string operations
  RTC.getTime(currentTime);  // Get the current time from RTC
  
  lcd.clrScr();               // Clear screen
  lcd.setFont(Small5x7PLBold);// Set data fonts
  lcd.setDigitMinWd(0);       // Set distance between words on LCD
  sprintf(cBuff, "%02d/", currentTime.getDayOfMonth());  // Create day string with 2 digits + /
  Serial.print(cBuff);        // Print day on Serial
  lcd.printStr(16, 1, cBuff); // Print day on LCD

  sprintf(cBuff, "%02d/", Month2int(currentTime.getMonth()));  // Create month string with 2 digits + /
  Serial.print(cBuff);        // Print month on Serial
  lcd.printStr(33, 1, cBuff); // Print month on LCD

  sprintf(cBuff, "%4d ", currentTime.getYear());  // Create year string + blank
  Serial.print(cBuff);        // Print year on Serial
  lcd.printStr(50, 1, cBuff); // Print year on LCD

  lcd.setFont(Term8x14PL);  // Set time font        
  sprintf(cBuff, "%02d:", currentTime.getHour());  // Hour in 2 digits +:
  Serial.print(cBuff);        // Print hour on Serial
  lcd.printStr(15, 3, cBuff); // Print hour on LCD

  sprintf(cBuff, "%02d:", currentTime.getMinutes());  // Minutes in 2 digits +:
  Serial.print(cBuff);        // Print minuts on Serial
  lcd.printStr(36, 3, cBuff); // Print minuts on LCD

  sprintf(cBuff, "%02d", currentTime.getSeconds());  // Seconds in 2 digits
  Serial.print(cBuff);        // Print seconds on Serial
  lcd.printStr(57, 3, cBuff); // Print seconds on LCD

  wk = "<"                                                                       // <
       + String(dispWeekName[DayOfWeek2int(currentTime.getDayOfWeek(), 0) % 7])  // Day of the week character generation
       + ">";                                                                    // >
  Serial.print(" ");            // Leave one character open for serial                       
  Serial.print(wk);             // Output the day of the week on serial
  Serial.println();            // Line break serial
  lcd.setFont(Small5x7PLBold);
  lcd.setDigitMinWd(0);
  lcd.printStr(ALIGN_CENTER, 0, (char*)wk.c_str());// Display the day of the week on LCD
  
}

void adjustSec() {  // Advance the second value of savedTime by 0.5 seconds on average
  int x;
  x = savedTime.getSeconds();  // Get the seconds value from savedTime
  x = x + ((x + 1) % 2);       // +1 for even numbers, leave as is for odd numbers. In other words, the average +0.5 and 59 seconds are not corrected, so there is no increase.
  savedTime.setSecond(x);      // Write back the second value
}

void adjustZeroSec() {                // Adjust RTC time to 0 seconds (30 seconds adjustment)
  R_RTC->RCR2_b.ADJ30 = 1;            // Write 1 to ADJ30 and perform adjustment for 30 seconds
  while (R_RTC->RCR2_b.ADJ30 == 1) {  // Wait until adjustment is complete
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

void viewRTCregisters() {                                     // Output the status of real-time clock registers
  sprintf(cBuff, "RCR1 = 0x%02x", R_RTC->RCR1);               // Read RCR1 and convert it to 2 hexadecimal characters
  Serial.print(cBuff);                                       // Output to serial
  sprintf(cBuff, ", RCR2 = 0x%02x", R_RTC->RCR2);             // Read RCR2 and convert it to 2 hexadecimal characters
  Serial.print(cBuff);                                        // Output to serial
  sprintf(cBuff, ", RCR4 = 0x%02x", R_RTC->RCR4);             // Read RCR4 and convert it to 2 hexadecimal characters
  Serial.print(cBuff);                                        // Output to serial
  sprintf(cBuff, ", RFRH = 0x%04x", R_RTC->RFRH);             // Read RFRH and convert it to 2 hexadecimal characters
  Serial.print(cBuff);                                        // Output to serial
  sprintf(cBuff, ", RFRL = 0x%04x", R_RTC->RFRL);             // Read RFRL and convert it to 2 hexadecimal characters
  Serial.print(cBuff);                                        // Output to serial
  sprintf(cBuff, ", RADJ = 0x%02x", R_RTC->RADJ);             // Read RADJ and convert it to 2 hexadecimal characters
  Serial.print(cBuff);                                        // Output to serial
  sprintf(cBuff, ", LOCOUTCR = 0x%02x", R_SYSTEM->LOCOUTCR);  // Read LOCOUTCR and convert it to 2 hexadecimal characters
  Serial.print(cBuff);                                        // Output to serial
  Serial.println();
}

void viewRTCregister2(){
  Serial.println("RTC Control Register 1");
  PRINT_REG(R_RTC, RCR1);Serial.println();
  Serial.println("RTC Control Register 2");
  PRINT_REG(R_RTC, RCR2);Serial.println();
  Serial.println("RTC Control Register 4");
  PRINT_REG(R_RTC, RCR4);Serial.println();
  Serial.println("Frequency Register RFRH");
  PRINT_REG(R_RTC, RFRH);Serial.println();
  Serial.println("Frequency Register RFRL");
  PRINT_REG(R_RTC, RFRL);Serial.println();
  Serial.println("Time Error Adjustment Register RADJ");
  PRINT_REG(R_RTC, RADJ);Serial.println();
  Serial.println("LOCO User Trimming Control Register");
  PRINT_REG(R_SYSTEM, LOCOUTCR);Serial.println();
  
}

void periodicCallback() {  // RTC interrupt processing (called every 0.5 seconds)
  irqFlag = true;          // Set flag and notify main
}