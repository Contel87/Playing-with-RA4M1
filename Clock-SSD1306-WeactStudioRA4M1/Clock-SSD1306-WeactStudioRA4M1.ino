/* Arduino UNO R4 で作るOLED時計 20240408_UnoR4RtcClockV220.ino
  時刻設定機能、曜日自動設定
  2024/4/7 ラジオペンチ http://radiopench.blog96.fc2.com/

  12/10/2024 Updated by Luigi Conte (Contel87):
  - Traslate name of days in ITAlian language
  - Changed data string from YYYY/MO/DD to DD/MO/YYYY
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTC.h>

#define SCREEN_WIDTH   128     // OLED X
#define SCREEN_HEIGHT  64      // OLED Y
#define OLED_RESET     -1      // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C    // OLED I2C address
#define VER_NUM        "V220"  // Release version number
#define DEC_B          4       // Decrement button Pin
#define INC_B          5       // Increment button Pin
#define ENT_B          6       // Enter button Pin

//----------------PRINT_REG command is useful to check RA4M1 register status
#define PRINT_REG(BUCKET, REGISTER) \
  do { \
    uint32_t t = BUCKET->REGISTER; \
    Serial.print(#REGISTER " : b"); \
    Serial.println(t, BIN); \
  } while (false)

char   cBuff[50];               // String operation buffer
String ddmmyy = "dd/mm/yyyy";   // day, month, and Year string
String hhmmss = "hh:mm/ss";     // Hour, minute, and second string
int    dd, mo, yy, hh, mi, ss;  // Time element

String dispWeekName[] = {  // Traslate day of the week in ITAlian languge, to other languages just change name of days between ""
  "Domenica", "Lunedi", "Martedi", "Mercoledi", "Giovedi", "Venerdi", "Sabato"
};

volatile bool irqFlag = false;

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

RTCTime savedTime;    // For saving RTC time
RTCTime currentTime;  // For time setting
RTCTime mytime;       // For time setting

void setup() {
  pinMode(DEC_B, INPUT_PULLUP);            // Dec.button
  pinMode(INC_B, INPUT_PULLUP);            // Inc.button and 30 second Adjust
  pinMode(ENT_B, INPUT_PULLUP);            // Enter button, press and hold during reset to set time
  //pinMode(21, OUTPUT);                     // Set led TX output
  Serial.begin(115200);                    // Serial start
  //delay(2000);                             // UNO R4 Serial initialization
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // OLED initialization
  oled.clearDisplay();                     // OLED buffer clearing
  oled.setTextSize(2);                     // Use double-width characters
  oled.setTextColor(WHITE);                // color is white
  oled.display();                          // Buffer transfer (screen erase)
  startScreen();                           // Start screen displayed on OLED
  delay(3000);
  Serial.println();Serial.println("Arduino UNO R4 RTC start");
  Serial.print("Reg. was : ");
  viewRTCregisters();
  RTC.begin();     // Start RTC library
  set48MhzClock(); // Set external MOSC + PLL (48Mhz)
  rtcSOSC();       // Set external SOSC (32.768 Khz)
  oled.display();
  RTC.getTime(savedTime);     // Record the current time in savedTime
  Serial.print("t1 = ");Serial.println(savedTime);  // Output for debugging (here it was the latest setting time)

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
  viewRTCregisters();           // Dump RTC related registers
  Serial.println("Clock start");
}

void loop() {
    String wk;
  while (irqFlag == false) {                // Wait for RTC 1 second interrupt
    if (digitalRead(INC_B) == LOW) {        // If the 30 seconds adjustment button is pressed
      adjustZeroSec();                      // Execute second hand adjustment (30 second adjustment)
      dispTime();                           // Reflect the result on the display
      while (digitalRead(INC_B) == LOW) {}  // Wait until button is released
    }
  }
  irqFlag = false;         // An interrupt has occurred, so
  //digitalWrite(21, HIGH);  // LED lights up
  dispTime();              // Display current time on LCD
  //digitalWrite(21, LOW);   // LED off
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


void rtcSOSC() { // Swith to external 32.768Khz Sub-OScillator
  R_RTC->RCR4_b.RCKSEL = 0; //Select SOSC
  delay(10);
  R_RTC->RCR2_b.START = 0;            // Temporarily stop RTC
  while (R_RTC->RCR2_b.START == 1) {}  // Wait until RTC stops
  (R_RTC->RCR4_b.RCKSEL)? Serial.println("Unable to switch external SOSC") : Serial.println("RTC use SOSC");
  R_RTC->RCR2_b.CNTMD = 0;          // 0=Calendar count mode 1=Binary count mode
  while (R_RTC->RCR2_b.CNTMD == 1) {}  // Confirm CNTMD status
  R_RTC->RCR2_b.RESET = 1;            // Restart RTC
  while (R_RTC->RCR2_b.RESET == 1) {  // Wait until RTC restarts
  }
  
}

void clkExtXtalTrim() {      // RTCのクロックを外部水晶に設定、補正値を設定（電源切った時に時刻が狂うことがある?）
  oled.print("Xtal+Trim");   // OLEDにクロックソースを表示
  R_RTC->RCR4_b.RCKSEL = 0;  // RTCクロックソースに外部水晶を選択
  delay(10);                 // 切り替え完了までちょっと待機
  R_RTC->RADJ         = 0;   // 補正量をクリア
  R_RTC->RCR2_b.AADJE = 0;   // 補正を停止
  delay(10);
  R_RTC->RCR2_b.AADJP = 1;  // 補正周期10秒 （0:1分、1:10秒）※要調整
  delay(10);
  R_RTC->RCR2_b.AADJE = 1;     // 補正実行(RCR2設定完了）
  R_RTC->RADJ         = 0x96;  // PMADJ=0b10, ADJ=22を一括書き込み（※要調整）
  // ADJの値は 1/(32768*10) = 3.052ppm / LSB　(10秒周期の場合）
  // この例では67.6ppmの誤差があったので補正量=22
  // ADJの値の上限は63（6ビット）
}

void startScreen() {         // LCD display at start
  oled.setCursor(0, 0);      //From the beginning,
  oled.println("R4 clock");  //
  oled.println(VER_NUM);     // Version #
  oled.println("Source =");  // Clock source
  oled.setCursor(0, 48);
  if(R_RTC->RCR4_b.RCKSEL == 0 ) {
    oled.println("Ext SOSC");
  }else{
    oled.println("LOCO");
  }
  oled.display();
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

void clockAdjust() {  // Set the time with OLED and button switch
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.setTextSize(2);
  oled.println("Clock ADJ.");  // Time setting start display
  oled.display();
  while (digitalRead(ENT_B) == LOW) {}  // wait until ent button is released

  // ------- Display on LCD and serial date and time to setting with buttons

  getDateTime();  // Get the current time from the built-in RTC

  oled.clearDisplay();     
  oled.setCursor(0, 0);
  oled.println(ddmmyy);    // Display current date on LCD
  Serial.println(ddmmyy);  // Display current date on serial monitor

  hhmmss[6] = '-';  // Delete the seconds digit with --
  hhmmss[7] = '-';
  oled.setCursor(13, 16);  // In the second line,
  oled.println(hhmmss);    // Initial time display
  Serial.println(hhmmss);
  oled.display();

  // Specify the x, y coordinates, value, step, lower limit, and upper limit, and enter the setting values ​​while looking at the screen.
  yy = oledRW(96, 0, yy, 1, 20, 60);  // Enter the year value (used as the last two digits) ... At X=96 starts 2 yy digits
  mo = oledRW(36, 0, mo, 1, 1, 12);   // Month input ... At X=36 starts 2 mo digits

  if (mo == 2) {                                                   // If month is febrary ...
    if ((yy % 4) == 0) {                                           // ... and the year is divisible by 4
      dd = oledRW(0, 0, dd, 1, 1, 29);                            // It's a leap year, so until the 29th.
    } else {                                                       // If it's not a leap year,
      dd = oledRW(0, 0, dd, 1, 1, 28);                            // Until the 28th
    }                                                              //
  } else if ((mo == 4) || (mo == 6) || (mo == 9) || (mo == 11)) {  // For April, June, September, November
    dd = oledRW(0, 0, dd, 1, 1, 30);                              // Until the 30th
  } else {                                                         //
    dd = oledRW(0, 0, dd, 1, 1, 31);                              // Unless above, until the 31st
  }                                                                //
  hh = oledRW(13, 16, hh, 1, 0, 23);                               // Hours (display position adjustment required)
  mi = oledRW(49, 16, mi, 1, 0, 59);                               // Minutes (display position adjustment required)

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
  oled.drawFastHLine(x - 1, y + 14, 24, BLACK);  // Erase the underline (reflection on screen is at the next digit)
  oled.drawFastHLine(x - 1, y + 15, 24, BLACK);  // remove underline
  delay(30);
  return d;  // Return the set value as the return value
}

void oledDisp2Chr(int x, int y, int val) {       // Displays 2-digit value at specified location on OLED
  oled.fillRect(x, y, 24, 16, BLACK);            // Delete 2 characters from the specified coordinates (black paint)
  oled.drawFastHLine(x - 1, y + 14, 24, WHITE);  // draw an underline
  oled.drawFastHLine(x - 1, y + 15, 24, WHITE);  // Draw an underline (second line)
  sprintf(cBuff, "%02d", val);                   // Convert data to decimal 2-digit 0-fill string
  oled.setCursor(x, y);                          // Place the cursor at the specified position
  oled.print(cBuff);                             // write the number
  oled.display();                                // display on screen
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

  oled.clearDisplay();
  oled.setCursor(0, 0);

  sprintf(cBuff, "%02d/", currentTime.getDayOfMonth());  // Create day string with 2 digits + /
  Serial.print(cBuff);                                   // output to serial
  oled.print(cBuff);                                     // Output to OLED

  sprintf(cBuff, "%02d/", Month2int(currentTime.getMonth()));  // Create month string with 2 digits + /
  Serial.print(cBuff);
  oled.print(cBuff);

  sprintf(cBuff, "%4d ", currentTime.getYear());  // Create year string + blank
  Serial.print(cBuff);
  oled.print(cBuff);

  oled.setCursor(12, 16);                          // On the second line of OLED,
  sprintf(cBuff, "%02d:", currentTime.getHour());  // Hour in 2 digits +:
  Serial.print(cBuff);
  oled.print(cBuff);

  sprintf(cBuff, "%02d:", currentTime.getMinutes());  // Minutes in 2 digits +:
  Serial.print(cBuff);
  oled.print(cBuff);

  sprintf(cBuff, "%02d", currentTime.getSeconds());  // Seconds in 2 digits
  Serial.print(cBuff);
  oled.print(cBuff);

/*
 wk = "<"                                                                       // <
       + String(dispWeekName[DayOfWeek2int(currentTime.getDayOfWeek(), 0) % 7])  // Day of the week character generation
       + ">"; 
*/ 
  wk = String(dispWeekName[DayOfWeek2int(currentTime.getDayOfWeek(), 0) % 7]);
  Serial.print(" ");            // Leave one character open for serial                       
  Serial.print(wk);             // Output the day of the week on serial
  oled.setCursor(7, 32);                                                        // On the 3rd line of OLED
  oled.print(wk);                                                                // Show day of week

  Serial.println();  // Line break serial
  oled.display();    // Transfer to OLED and display
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

void viewRTCregisters() {                                     // リアルタイムクロックのレジスタの状態を出力
  sprintf(cBuff, "RCR1 = 0x%02x", R_RTC->RCR1);               // RCR1を読んで16進2文字に変換
  Serial.print(cBuff);                                        // シリアルに出力
  sprintf(cBuff, ", RCR2 = 0x%02x", R_RTC->RCR2);             // RCR2を読んで16進2文字に変換
  Serial.print(cBuff);                                        // シリアルに出力
  sprintf(cBuff, ", RCR4 = 0x%02x", R_RTC->RCR4);             // RCR4を読んで16進2文字に変換
  Serial.print(cBuff);                                        // シリアルに出力
  sprintf(cBuff, ", RFRH = 0x%04x", R_RTC->RFRH);             // RFRHを読んで16進4文字に変換
  Serial.print(cBuff);                                        // シリアルに出力
  sprintf(cBuff, ", RFRL = 0x%04x", R_RTC->RFRL);             // RFRLを読んで16進4文字に変換
  Serial.print(cBuff);                                        // シリアルに出力
  sprintf(cBuff, ", RADJ = 0x%02x", R_RTC->RADJ);             // RADJを読んで16進2文字に変換
  Serial.print(cBuff);                                        // シリアルに出力
  sprintf(cBuff, ", LOCOUTCR = 0x%02x", R_SYSTEM->LOCOUTCR);  // LOCOUTRを読んで16進2文字に変換
  Serial.print(cBuff);                                        // シリアルに出力
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

void periodicCallback() {  // RTC割り込み処理(0.5秒周期でコール）
  irqFlag = true;          // フラグを立ててメインに通知
}
