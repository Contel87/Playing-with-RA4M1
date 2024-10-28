//OOOOOOOOKK

//-------------Menu
#include "setup.h"
variables var; // struct variables
#define START_SCREEN   "Ciao forum"  // Start screen message
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


char   cBuff[50];               // String operation buffer
String ddmmyy = "dd/mm/yyyy";   // day, month, and Year string
String hhmmss = "hh:mm/ss";     // Hour, minute, and second string
int    dd, mo, yy, hh, mi, ss;  // Time element

String dispWeekName[] = {  // Traslate day of the week in ITAlian languge, to other languages just change name of days between ""
  "Domenica", "Lunedi", "Martedi", "Mercoledi", "Giovedi", "Venerdi", "Sabato"
};

volatile bool irqFlag = false;

RTCTime savedTime;    // For saving RTC time
RTCTime currentTime;  // For time setting
RTCTime mytime;       // For time setting


void setup() {
  //pinMode(21, OUTPUT);                     // Set led TX output
  pinMode(encoderPinA, INPUT_PULLUP);      // Buttons are directly connected to GND
  pinMode(encoderPinB, INPUT_PULLUP);      // Buttons are directly connected to GND
  pinMode(DEC_B, INPUT_PULLUP);            // Dec.button
  pinMode(INC_B, INPUT_PULLUP);            // Inc.button and 30 second Adjust
  pinMode(ENT_B, INPUT_PULLUP);            // Ent. button
  attachInterrupt(digitalPinToInterrupt(encoderPinA), doEncoderA, CHANGE);  // Call interrupt when turn encoder CW
  attachInterrupt(digitalPinToInterrupt(encoderPinA), doEncoderB, CHANGE); // Call interrupt when turn encoder CCW
  startScreen();                           // Power ON OLED
  Serial.begin(9600);                      // Initialize Serial communication
  delay(2000);                             // Arduino UNO R4 need delay to open Serial
  Serial.println();Serial.println("Arduino UNO R4 RTC start");
  RTC.begin();     // Start RTC library
  // Uncommet two line below only if board has external crystal resonators like WeAct Studio RA4M1.
  // If use Arduino UNO R4 NOT call set48MhzClock() and rtcSOSC() funct.
  set48MhzClock();                         // Set 48Mhz main clock with external 16Mhz chrystal osc and PLL
  rtcSOSC();                               // Select External 32.768Khz quartz
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
   Serial.println("Clock start");
}


void loop() {
switch(var.current){     // Select properly menu page with switch - case conditions ...
    case st_Menu0:       // If enum stati current == stMenu0
      myMenu0(&var);       // Enter inside Menu0 page
      checkEnc();        // Check if encoder rotate
    break;

    case st_Menu1:      // If enum stati current == stMenu1
      myMenu1(&var);
      checkEnc();
    break;

    case st_Menu2:       // If enum stati current == stMenu2
      myMenu2(&var);
      checkEnc();
    break;
 
  }
}

static void myMenu0(struct variables *var){ // Clock
if(var->first){ ////////////////////////////////////
 Serial.print("Menu ");Serial.println(var->current);
 var->first = false; ///////////////////////////////
 }
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

void periodicCallback() {
  irqFlag = true;
}


void myMenu1(struct variables *var){
if(var->first){
 Serial.print("Menu ");Serial.println(var->current);
 var->deviceID = 1234;                                   // Example to use a member of struct variables
 Serial.print("devID: ");Serial.println (var->deviceID); // If compiler return error change Serial.println(st->deviceid) with Serial.println(String(st->deviceid))
  oled.clearDisplay();
  oled.setCursor(0, 5);
  oled.setTextSize(2);
  oled.print("Menu ");oled.println(var->current);
  oled.display();
 var->first = false;
 }
 encButton();
}


void myMenu2(struct variables *var){
if(var->first){
 Serial.print("Menu ");Serial.println(var->current);
  oled.clearDisplay();
  oled.setCursor(0, 5);
  oled.setTextSize(2);
  oled.print("Menu ");oled.println(var->current);
  oled.display();
 var->first = false;
 }
 encButton();
}


void startScreen(){                        // Start screen displayed on OLED
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // OLED initialization
  oled.clearDisplay();                     // Clear display
  oled.setTextSize(2);                     // Use double-width characters
  oled.setTextColor(WHITE);                // color is white
  oled.setCursor(0, 20);                    //Set OLED X Y coordinate
  oled.println(START_SCREEN);
  oled.display();                         // Send message on OLED
  delay(3000);
}


void checkEnc(){ // Check encoder rotations and move inside menu pages
  static int8_t value, oldvalue = 0;
  int8_t maxValue = stati::elementsCounter -1; // Number of elements inside enum stati
  int8_t minValue = 0; // Min value
  static uint32_t pm = millis();
  uint32_t cm = millis();
  if (cm - pm >= 200){ // debouncing time
    pm = cm;
  if (var.pos == 1) {
    ++value;
  }
  if (var.pos == -1) {
    --value;
  }
  if (value != oldvalue) {
    if (value >= maxValue) value = maxValue;
    if (value <= minValue) value = minValue;
    if (value != oldvalue){
    var.first = true;
    var.current = static_cast<menu_num_t>(value);
    }
  }
  oldvalue = value;
  var.pos = 0;
  }  
}


void doEncoderA() {
  // look for a low-to-high on channel A
  if (digitalRead(encoderPinA) == HIGH) {
    // check channel B to see which way encoder is turning
    if (digitalRead(encoderPinB) == LOW) {
      var.pos = 1;
    } else {
      var.pos = -1;
    }
  } else {// look for a high-to-low on channel A
    if (digitalRead(encoderPinB) == HIGH) {// check channel B to see which way encoder is turning
      var.pos = 1;
    } else {
      var.pos = -1;
    }
  }  
}

void doEncoderB() {
  // look for a low-to-high on channel B
  if (digitalRead(encoderPinB) == HIGH) {
    // check channel A to see which way encoder is turning
    if (digitalRead(encoderPinA) == HIGH) {
      var.pos = 1;
    } else {
      var.pos = -1;
    }
  } else { // Look for a high-to-low on channel B
    // check channel B to see which way encoder is turning
    if (digitalRead(encoderPinA) == LOW) {
      var.pos = 1;
    } else {
      var.pos = -1;
    }
  }
}
