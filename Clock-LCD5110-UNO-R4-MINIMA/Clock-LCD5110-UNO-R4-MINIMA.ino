/// OK da riga 185

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
// define USESPI in above header for HW SPI version
#include <Wire.h>
#include <RTC.h>
N5110_SPI lcd(9,10,8); // RST,CS,DC
#define VER_NUM        "V1.0"
#define DEC_B          4       // デクリメントボタンPin
#define INC_B          5       // インクリメントボタンPin
#define ENT_B          6       // EnterボタンPin

#if USESPI==1
#include <SPI.h>
#endif

//#include "fonts_all.h"
#include "small5x7bold_font.h"
#include "term8x14_font.h"
#include "digits4x7_font.h"

char   cBuff[50];               // 文字列操作バッファ
String ddmmyy = "dd/mm/yyyy";   // 年月日文字列
String hhmmss = "hh:mm/ss";     // 時分秒文字列
int    dd, mo, yy, hh, mi, ss;  // 時刻の要素

String dispWeekName[] = {  // 曜日の表示文字列(DayOfWeekの略)
  "Domenica", "Lunedi", "Martedi", "Mercoledi", "Giovedi", "Venerdi", "Sabato"
};


volatile bool irqFlag = false;
RTCTime savedTime;    // RTC時刻保存用
RTCTime currentTime;  // 時刻設定用
RTCTime mytime;       // 時刻設定用




void setup() 
{
  Serial.begin(9600);
  delay(2000);
  set48MhzClock();
  rtcSOSC();
  pinMode(DEC_B, INPUT_PULLUP);            // Dec.ボタン
  pinMode(INC_B, INPUT_PULLUP);            // Inc.ボタン、30秒Adjustボタン
  pinMode(ENT_B, INPUT_PULLUP);            // Enterボタン、リセット時の押し続けると時刻セット
  pinMode(21, OUTPUT);                     // 1秒割り込み監視
  lcd.init();
  startScreen();
  Serial.println();                            // 開始位置表示のためにシリアルを1行開ける
  Serial.println("Arduino UNO R4 RTC start");
  
  RTC.begin();
  selectRtcClockSource();///???
  delay(2000); ///???
  RTC.getTime(savedTime);     // 現在時刻をsavedTimeに記録
  Serial.print("t1 = ");      //
  Serial.println(savedTime);
  if (digitalRead(ENT_B) == LOW) {  // ENTボタンがLOWだったら時刻設定
    //RTCTime startTime(2, Month::APRIL, 2024, 17, 8, 0, DayOfWeek::TUESDAY, SaveLight::SAVING_TIME_ACTIVE);  // 決め打ちの時刻をセット
    //RTC.setTime(startTime);
    clockAdjust();  // ボタン操作で時刻を設定

  } else {                   // ENTボタンが押されていなければ通常起動
    adjustSec();             // savedTimeを平均0.5秒進める
    RTC.setTime(savedTime);  // 起動時の時刻を継承(これやらないとRTCが起動しない)
  }
  dispTime();  // 画面表示

  if (!RTC.setPeriodicCallback(periodicCallback, Period::ONCE_EVERY_1_SEC)) {  // 1秒周期で割り込み設定
    Serial.println("ERROR: periodic callback not set");                        // 失敗してたら報告
  }
  Serial.print("Reg. set : ");  // この時点の、
  viewRTCregisters();           // RTC関連レジスタをダンプ
  Serial.println("Clock start");
}



void loop() 
{/*
  lcd.clrScr();
  lcd.setFont(Term8x14PL);
  lcd.printStr(12, 1, "11:11:11");
  lcd.setFont(Small5x7PLBold);
  lcd.setDigitMinWd(10); // distanza tra parole
  lcd.printStr(0, 0, "Prova testo");
  delay(1000);
  */
    String wk;
  while (irqFlag == false) {                // RTCの1秒割り込み待ち
    if (digitalRead(INC_B) == LOW) {        // 30秒調整ボタンが押されていたら
      adjustZeroSec();                      // 秒針合わせ（30秒調整）を実行
      dispTime();                           // 結果を表示に反映
      while (digitalRead(INC_B) == LOW) {}  // ボタンが離されるまで待つ
    }
  }
  irqFlag = false;         // 割り込みが入ったので、
  digitalWrite(21, HIGH);  // LED点灯
  dispTime();              // OLED,シリアルに現在時刻を表示
  digitalWrite(21, LOW);   // LED消灯
}

void startScreen() {         // 開始時のOLED表示
lcd.setFont(Small5x7PLBold);
  lcd.setDigitMinWd(7); // Distanza caratteri
  lcd.clrScr();
  lcd.printStr(0, 0, " UNO R4 clock");
  lcd.printStr(0, 1, "Version #");
  lcd.printStr(55, 1, VER_NUM);
  lcd.printStr(2, 3, "Source :");
  (R_RTC->RCR4_b.RCKSEL == 0) ? lcd.printStr(0, 4, "External SOSC") : lcd.printStr(55, 4, "LOCO");
}

void selectRtcClockSource() {
  R_RTC->RCR2_b.START = 0;            // RTCを一旦停止
  while (R_RTC->RCR2_b.START == 1) {  // RTCが止まるまで待つ
  }
   R_RTC->RCR2_b.START = 1;            // RTCを再起動
  while (R_RTC->RCR2_b.START == 0) {  // RTCが動くまで待つ
  }
  
}

void getDateTime() {  // RTCの値から日時の文字列を作成する
  ddmmyy = "";
  hhmmss = "";  
  dd = savedTime.getDayOfMonth();
  sprintf(cBuff, "%02d", dd);  // / に続けて右詰め2桁
  ddmmyy += cBuff;              // 年月日の文字列完成（ex:2019/02/28)
  mo = Month2int(savedTime.getMonth());
  sprintf(cBuff, "/%02d", mo);  // / に続けて右詰め2桁
  ddmmyy += cBuff;
  yy = savedTime.getYear() - 2000;  // yの値は年の値の下2桁(書き戻す時は+2000)
  sprintf(cBuff, "/20%02d", yy);     // 20に続いて右詰め2桁
  ddmmyy += cBuff;
  hh = savedTime.getHour();
  sprintf(cBuff, "%02d", hh);  // 右詰め2桁
  hhmmss += cBuff;
  mi = savedTime.getMinutes();
  sprintf(cBuff, ":%02d", mi);  // : に続けて右詰め2桁、
  hhmmss += cBuff;
  ss = savedTime.getSeconds();  // 秒
  sprintf(cBuff, ":%02d", ss);  // :に続けて右詰め2桁
  hhmmss += cBuff;              // 時分秒の文字列完成（ex:01:02:03）
}

void clockAdjust() {  // OLEDとボタンスイッチで時刻を合わせる
/*
  lcd.clrScr();
  lcd.setFont(Term8x14PL);
  lcd.printStr(12, 1, "11:11:11");
  lcd.setFont(Small5x7PLBold);
  lcd.setDigitMinWd(10); // distanza tra parole
  lcd.printStr(0, 0, "Prova testo");
  delay(1000);
  */
  lcd.clrScr();
  lcd.setFont(Small5x7PLBold);
  lcd.setDigitMinWd(10); // distanza tra parole
  lcd.printStr(12, 2, "Clock ADJ.");
  while (digitalRead(ENT_B) == LOW) {  // entボタンが離されるまで待つ
  }

  getDateTime();  // 現在時刻を内蔵RTCから取得

  lcd.clrScr();
  lcd.setDigitMinWd(0);

  sprintf(cBuff, "%02d/", dd);  // 2桁で日の文字列作成 +空白
  lcd.printStr(16, 1, cBuff);  

  sprintf(cBuff, "%02d/", mo);  // 2桁で月の文字列作成 +/
  lcd.printStr(33, 1, cBuff);

  lcd.printStr(50, 1, "20"); // Show first 2 digit of year (every 20)
  
  Serial.println(ddmmyy);  // シリアルにも出力 
  
  hhmmss[6] = '-';  // 秒の桁を--で消す
  hhmmss[7] = '-';
  
  lcd.setFont(Term8x14PL);     // Change fonts for clock digits       
  sprintf(cBuff, "%02d:", hh);  // Display hours
  lcd.printStr(15, 3, cBuff); 

  sprintf(cBuff, "%02d:", mi);  // Display minuts
  lcd.printStr(36, 3, cBuff); 
  

  lcd.printStr(57, 3, "--"); // I secondi non si regolano

  Serial.println(hhmmss); // Stampa su seriale

  // Now setup Date and time
  lcd.setFont(Small5x7PLBold); // Set date font

  lcd.fillWin(50,2,22,1,0xa); // Underline yy to setting
  yy = oledRW(62, 1, yy, 1, 20, 60);  // Show last 2 digit of year to set
  lcd.fillWin(50,2,22,1,0); // Switch OFF underline yy
  
  lcd.fillWin(33,2,11,1,0xa);
  mo = oledRW(33, 1, mo, 1, 1, 12);   // Position month to setting
  lcd.fillWin(33,2,11,1,0); // Switch OFF underline month

  lcd.fillWin(16,2,11,1,0xa); // Underline day
  if (mo == 2) {                                                   // 2月で、
    if ((yy % 4) == 0) {                                           // 年が4で割り切れたら
      dd = oledRW(16, 1, dd, 1, 1, 29);                            // 閏年なので29日まで
    } else {                                                       // 閏年では無ければ、
      dd = oledRW(16, 1, dd, 1, 1, 28);                            // 28日まで
    }                                                              //
  } else if ((mo == 4) || (mo == 6) || (mo == 9) || (mo == 11)) {  // 4,6,9,11月なら
    dd = oledRW(16, 1, dd, 1, 1, 30);                              // 30日まで
  } else {                                                         //
    dd = oledRW(16, 1, dd, 1, 1, 31);                              // 上記以外なら31日まで
  }                                                                //
  lcd.fillWin(16,2,11,1,0); // Switch OFF underline day


  lcd.setFont(Term8x14PL); // Set time font
  lcd.fillWin(15,5,17,1,0xa);
  hh = oledRW(15, 3, hh, 1, 0, 23);                               // Set hour
  lcd.fillWin(15,5,17,1,0);
  
  lcd.fillWin(33,2,11,1,0xa);
  mi = oledRW(36, 3, mi, 1, 0, 59);                               // Set minuts
  lcd.fillWin(33,2,11,1,0);

  setDMYHMSW(dd, mo, yy + 2000, hh, mi, 0);  // 設定した時刻をRTCのフォーマットに変換
  RTC.setTime(mytime);                       // mytimeの値をRTCに書く
  adjustZeroSec();                           // ゼロ秒起動させるために30秒調整を実行
}

int oledRW(int x, int y, int d, int stepD, int minD, int maxD) {  // OLEから値を入力
  // OLEDの指定位置に2桁右詰めで変数の値を表示。ボタン操作で値を増減し、
  // Ent入力で値を確定し戻り値として返す。表示位置の左上をx, y 座標で指定
  // 操作位置は下線で表示。値は上下限の範囲でサーキュレート。文字サイズは2倍角(12x16画素）
  // 引数：x座標、ｙ座標、変更したい値、変更ステップ量、下限値、上限値

  oledDisp2Chr(x, y, d);               // 画面の指定位置に初期値を2桁表示（下線付き）
  while (digitalRead(ENT_B) == LOW) {  // enterボタンが押されていたら離されるまで待つ
  }
  delay(30);
  while (digitalRead(ENT_B) == HIGH) {  // enterボタンが押されるまで以下を実行
    if (digitalRead(INC_B) == 0) {      // + ボタンが押されていたら
      d = d + stepD;                    // x を指定ステップ増加
      if (d > maxD) {                   // 上限超えたら下限へサキュレート
        d = minD;
      }
      oledDisp2Chr(x, y, d);             // 画面の指定位置に数値を2桁表示（下線付き）
      while (digitalRead(INC_B) == 0) {  // + ボタンが離されるまで待つ
      }
      delay(30);
    }

    if (digitalRead(DEC_B) == 0) {  // - ボタンが押されていたら
      d = d - stepD;                // x を指定ステップ減らす
      if (d < minD) {               // 下限以下なら上限へサーキュレート
        d = maxD;
      }
      oledDisp2Chr(x, y, d);             // 画面の指定位置に数値を2桁表示（下線付き）
      while (digitalRead(DEC_B) == 0) {  // - ボタンが離されるまで待つ
      }
      delay(30);
    }
  }
  delay(30);
  return d;  // 設定した値を戻り値で返す
}

void oledDisp2Chr(int x, int y, int val) {       // OLEDの指定場所に2桁の値を表示
  sprintf(cBuff, "%02d", val);                   // データーを10進2桁0フィル文字列に変換
  lcd.printStr(x, y, cBuff);
}

// UNO R4 のRTCに時刻を設定
void setDMYHMSW(int dd, int mm, int yy, int th, int tm, int ts) {  // 年月日時分秒の値でRTCを設定（曜日も設定）
  int wkN = 0;                                                     // 曜日の連番、日曜日=0
  mytime.setDayOfMonth(dd);  // 日を設定
  switch (mm) {                                                    // 月を設定
    case 1: mytime.setMonthOfYear(Month::JANUARY); break;          // mmが1ならJANUALY。
    case 2: mytime.setMonthOfYear(Month::FEBRUARY); break;         // 2月
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
  mytime.setYear(yy);                                              // 年(4桁の値）を設定
  mytime.setHour(th);        // 時
  mytime.setMinute(tm);      // 分
  mytime.setSecond(ts);      // 秒

  // ツェラーの式で曜日連番を求める
  if (mm <= 2) {  // 1,2月は、
    yy--;         // 前年の
    mm += 12;     // 13,14月として計算
  }
  wkN = (yy + yy / 4 - yy / 100 + yy / 400 + (13 * mm + 8) / 5 + dd) % 7;
  Serial.print("Week serial num = ");
  Serial.println(wkN);
  switch (wkN) {  // 連番を使って曜日を設定
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

void dispTime() {            // 時刻をシリアルとOLEDに表示
  String wk;                 // 文字列操作用
  RTC.getTime(currentTime);  // 現在時刻をRTCから取得
  
  lcd.clrScr();
  lcd.setFont(Small5x7PLBold);// serve????
  lcd.setDigitMinWd(0);       // serve????
  sprintf(cBuff, "%02d/", currentTime.getDayOfMonth());  // 2桁で日の文字列作成 +空白
  Serial.print(cBuff);
  lcd.printStr(16, 1, cBuff); 

  sprintf(cBuff, "%02d/", Month2int(currentTime.getMonth()));  // 2桁で月の文字列作成 +/
  Serial.print(cBuff); 
  lcd.printStr(33, 1, cBuff); 

  sprintf(cBuff, "%4d ", currentTime.getYear());  // 4桁で年の文字列作成 +/
  Serial.print(cBuff); 
  lcd.printStr(50, 1, cBuff);                             // OLEDに出力

  lcd.setFont(Term8x14PL);          
  sprintf(cBuff, "%02d:", currentTime.getHour());  // 2桁で時 +:
  Serial.print(cBuff);
  lcd.printStr(15, 3, cBuff); 

  sprintf(cBuff, "%02d:", currentTime.getMinutes());  // 2桁で分 +:
  Serial.print(cBuff);
  lcd.printStr(36, 3, cBuff); 

  sprintf(cBuff, "%02d", currentTime.getSeconds());  // 2桁で秒
  Serial.print(cBuff);
  lcd.printStr(57, 3, cBuff); 

  wk = "<"                                                                       // <
       + String(dispWeekName[DayOfWeek2int(currentTime.getDayOfWeek(), 0) % 7])  //  曜日文字生成
       + ">";                                                                    //  　　　　>
  Serial.print(" ");                                                             // シリアルには1文字開けて
  Serial.print(wk);
  lcd.setFont(Small5x7PLBold);
  lcd.setDigitMinWd(0);
  lcd.printStr(ALIGN_CENTER, 0, (char*)wk.c_str());    // 20, 0
  Serial.println();  // シリアルを改行
}

void adjustSec() {  // savedTimeの秒の値を平均で0.5秒進める（やや問題ある？）
  int x;
  x = savedTime.getSeconds();  // savedTimeから秒の値を取り出して
  x = x + ((x + 1) % 2);       // 偶数なら+1、奇数はそのまま。つまり平均+0.5、59秒は補正しないので桁上り無し
  savedTime.setSecond(x);      // 秒の値を書き戻す
}

void adjustZeroSec() {                // RTC時刻を0秒に合わせる（30秒調整）
  R_RTC->RCR2_b.ADJ30 = 1;            // ADJ30に1を書いて30秒調整を実行
  while (R_RTC->RCR2_b.ADJ30 == 1) {  // 調整完了まで待つ
  }
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
  R_SYSTEM->MOSCCR_b.MOSTP = 0;   // Enable XTAL
  R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); // wait for XTAL to stabilise
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
  R_SYSTEM->SOSCCR   = 0x01;       // Stop the sub-clock oscillator
  asm volatile ( "dsb" );          // Data bus Synchronization instruction
  while ( 1 != R_SYSTEM->SOSCCR ) R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
  R_SYSTEM->SOMCR       = 0x00;    // Sub-Clock Oscillator Drive in Normal Mode    
  R_SYSTEM->SOSCCR_b.SOSTP = 0;    // START sub-clock oscillator
  R_RTC->RCR4_b.RCKSEL  = 0;       //Select EXT 32.768 SOSC into RTC module
  R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); // wait for XTAL to stabilise 
  R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_CGC);
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

void periodicCallback() {  // RTC割り込み処理(0.5秒周期でコール）
  irqFlag = true;          // フラグを立ててメインに通知
}



