/* Original project from http://radiopench.blog96.fc2.com/blog-date-20240408.html
This project use a LCD Keypad Shield on board of Arduino Uno R4 Minima*/



#include <Wire.h>
#include <RTC.h>
#include <LiquidCrystal.h>

#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

#define BACKLIGHT 10


#define btnRIGHT  ((analogRead(0) >= 0) && (analogRead(0) <= 5))
#define btnUP     ((analogRead(0) >= 100) && (analogRead(0) <= 105))
#define btnDOWN   ((analogRead(0) >= 255) && (analogRead(0) <= 260))
#define btnLEFT   ((analogRead(0) >= 406) && (analogRead(0) <= 411))
#define btnSELECT ((analogRead(0) >= 636) && (analogRead(0) <= 641))
#define btnNONE   ((analogRead(0) >= 1007) && (analogRead(0) <= 1012))


char   cBuff[50];               // 文字列操作バッファ
String ddmmyy = "dd/mm/yyyy";   // 年月日文字列
String hhmmss = "hh:mm/ss";     // 時分秒文字列
int    dd, mo, yy, hh, mi, ss;  // 時刻の要素

String dispWeekName[] = {  // 曜日の表示文字列(DayOfWeekの略)
  "Dom", "Lun", "Mar", "Mer", "Gio", "Ven", "Sab"
};

volatile bool irqFlag = false;

RTCTime savedTime;    // RTC時刻保存用
RTCTime currentTime;  // 時刻設定用
RTCTime mytime;       // 時刻設定用






void setup() {
  Serial.begin(9600);// Inizializza seriale
  delay(2000);

 pinMode(13, OUTPUT);            //Imposta pin 13 come OUTPUT
 pinMode( BACKLIGHT,OUTPUT );    // Imposta il pin 10 come OUTPUT
 digitalWrite( BACKLIGHT,HIGH ); // Accende illuminazione Display

 lcd.begin(16, 2);               // Avvia il Display
 lcd.clear();                    // Ripulisce il Display

  Serial.println();               // Come inizio stampa una riga di seriale vuota
  Serial.println("Arduino UNO R4 RTC start");  // Messaggio di avvio (conferma comunicazione seriale ok)
  Serial.print("Reg. was : ");   // A questo punto
  viewRTCregisters();            // Scarica configurazione iniziale registri RTC
 
 RTC.begin();                    // Avvia la libreria RTC
 selectRtcClockSource();         // Seleziona la sorgente di Clock dell'orologio
 delay(2000);  // Attende completamento commutazione??

  RTC.getTime(savedTime);     // Aggiorna data e ora RTC da savedTime
  Serial.print("t1 = ");      //
  Serial.println(savedTime);  // Output su seriale di savedTime
  
  
  if (btnSELECT) {  // Se il pulsante SELECT e' premuto durante l'avvio
    clockAdjust();           // Imposta l'ora utilizzando i pulsanti UP e DOWN
} else {                     // Se all'avvio non e' premuto il pulsante SELECT si avvia normalmente
    adjustSec();             // avanza il savedTime con una media di 0,5 secondi
    RTC.setTime(savedTime);  // Avviare il modulo RTC
  }
  
  
  
  //dispTime();  // Visualizza data e ora sullo schermo
 if (!RTC.setPeriodicCallback(periodicCallback, Period::ONCE_EVERY_1_SEC)) {  // Richiama la funzione periodicCallback ogni secondo
    Serial.println("ERROR: periodic callback not set");                        // Errore in caso di errato avvio
  }
  Serial.print("Reg. set : ");  // A questo punto
  viewRTCregisters();           // Scarica la nuova configurazione dei registri RTC
  Serial.println("Clock start");
}

void loop() {
  String wk;
  while (!irqFlag) {
    if (btnUP) {           // Se il pulsante UP viene premuto per 30 secondi
      adjustZeroSec();             // Esegue la regolazione dei secondi
      dispTime();                  // Aggiorna il risultato sul Display
      while (btnUP) {}  // Attende il rilascio del pulsante UP
  }
  }
  irqFlag = false;         // Quando periodicCallback() nega il valore di irqFlag
  digitalWrite(13, HIGH);  // Si accende il led sul pin 13
  dispTime();              // Visualizza l'ora corrente
  digitalWrite(13, LOW);   // Spegne il led sul pin 13

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

void selectRtcClockSource() {
  R_RTC->RCR2_b.START = 0;            // RTCを一旦停止
  while (R_RTC->RCR2_b.START == 1) {  // RTCが止まるまで待つ
  }
  // クロックソースを下記4種から一つをコメントアウトで指定
  clkNormal();  // 内部CRオシレーター(LOCO)
  //clkNormalTrim(-2, 16);  // 内部CRオシレーター(LOCO)補正あり※要調整
  //clkExtXtal();  // 外部水晶
  //clkExtXtalTrim();  //外部水晶、補正あり。電源切ると大きく遅れることがある）

  R_RTC->RCR2_b.START = 1;            // RTCを再起動
  while (R_RTC->RCR2_b.START == 0) {  // RTCが動くまで待つ
  }
  //oled.display();  // クロックソースを画面に表示
}

void clkNormal() {                // クロックソースに内蔵CRオシレーターで調整無し状態に設定
  lcd.print("RC OSC");           // OLEDにクロックソースを表示
  R_RTC->RCR4_b.RCKSEL = 1;       // RTCクロックソースに内部オシレータ（LOCO)を選択
  R_SYSTEM->PRCR       = 0xA501;  // レジスタプロテクト解除
  R_SYSTEM->LOCOUTCR   = 0;       // LOCOの周波数トリム量=0
  R_SYSTEM->PRCR       = 0xA500;  // レジスタを再プロテクト
  R_RTC->RFRH          = 0;       // 分周比設定レジスタ上位の設定（ここは常にゼロ）
  R_RTC->RFRL          = 0xff;    // RTCクロック分周比に  0xff(デフォルト値)を設定
}

void clkNormalTrim(int a, int b) {  // RTCの内蔵CRオシレーターの周波数を調整
  // a: LOCOトリム量、b:分周比調整量（いずれも値が増えると時間が長くなる）
  int8_t   v1;                     // 符号付き8ビット
  uint16_t v2;                     // 符号無し10ビット
  lcd.print("RC OSC+Tm");         // OLEDにクロックソースを表示
  v1                   = a;        // LOCOの周波数トリム量
  v2                   = 255 - b;  // 分周比調整量
  R_RTC->RCR4_b.RCKSEL = 1;        // RTCクロックソースに内部オシレータ（LOCO)を選択
  R_SYSTEM->PRCR       = 0xA501;   // レジスタプロテクト解除
  R_SYSTEM->LOCOUTCR   = v1;       // LOCOの周波数トリム量設定、1LSB=3.7%
  R_SYSTEM->PRCR       = 0xA500;   // レジスタを再プロテクト
  R_RTC->RFRH          = 0;        // 分周比設定レジスタ上位の設定（ここは常にゼロ）
  R_RTC->RFRL          = v2;       // RTCクロック分周比設定、1LSB=0.39% (1/256）
}

void clkExtXtal() {          // RTCクロックを外部水晶に設定、補正無し
  lcd.print("Xtal");        // OLEDにクロックソースを表示
  R_RTC->RCR4_b.RCKSEL = 0;  // RTCクロックソースに外部水晶を選択
  delay(10);                 // ちょっと待つ
}

void clkExtXtalTrim() {      // RTCのクロックを外部水晶に設定、補正値を設定（電源切った時に時刻が狂うことがある?）
  lcd.print("Xtal+Trim");   // OLEDにクロックソースを表示
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
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Clock ADJ.");  // 時刻合わせ開始表示
  while (btnSELECT) {}

  getDateTime();  // 現在時刻を内蔵RTCから取得

  lcd.clear();     // 画面を消して
  lcd.setCursor(0, 0);    // 左上から
  lcd.print(ddmmyy);    // 現在の年月日を表示
  Serial.println(ddmmyy);  // シリアルにも出力

  hhmmss[6] = '-';  // 秒の桁を--で消す
  hhmmss[7] = '-';
  lcd.setCursor(0, 1);  // 2行目に、
  lcd.print(hhmmss);    // 初期時刻表示
  Serial.println(hhmmss);
  //oled.display();
  // x, y座標, 値, ステップ, 下限, 上限を指定し、画面を見ながら設定値を入力
  yy = oledRW(8, 0, yy, 1, 20, 60);  // 年の値を入力(下2桁で扱う）
  mo = oledRW(3, 0, mo, 1, 1, 12);   // 月の入力

  if (mo == 2) {                                                   // 2月で、
    if ((yy % 4) == 0) {                                           // 年が4で割り切れたら
      dd = oledRW(0, 0, dd, 1, 1, 29);                            // 閏年なので29日まで
    } else {                                                       // 閏年では無ければ、
      dd = oledRW(0, 0, dd, 1, 1, 28);                            // 28日まで
    }                                                              //
  } else if ((mo == 4) || (mo == 6) || (mo == 9) || (mo == 11)) {  // 4,6,9,11月なら
    dd = oledRW(0, 0, dd, 1, 1, 30);                              // 30日まで
  } else {                                                         //
    dd = oledRW(0, 0, dd, 1, 1, 31);                              // 上記以外なら31日まで
  }                                                                //
  hh = oledRW(0, 1, hh, 1, 0, 23);                               // 時 （表示位置要調整）
  mi = oledRW(3, 1, mi, 1, 0, 59);                               // 分 （表示位置要調整）

  setDMYHMSW(dd, mo, yy + 2000, hh, mi, 0);  // Converte l'ora nel formato RTC
  RTC.setTime(mytime);                       // mytimeの値をRTCに書く
  adjustZeroSec();                           // ゼロ秒起動させるために30秒調整を実行
}

int oledRW(int x, int y, int d, int stepD, int minD, int maxD) {  // OLEから値を入力
  // OLEDの指定位置に2桁右詰めで変数の値を表示。ボタン操作で値を増減し、
  // Ent入力で値を確定し戻り値として返す。表示位置の左上をx, y 座標で指定
  // 操作位置は下線で表示。値は上下限の範囲でサーキュレート。文字サイズは2倍角(12x16画素）
  // 引数：x座標、ｙ座標、変更したい値、変更ステップ量、下限値、上限値

  oledDisp2Chr(x, y, d);               // 画面の指定位置に初期値を2桁表示（下線付き）
  while (btnSELECT) {  // enterボタンが押されていたら離されるまで待つ
  }
  delay(30);  

    do {  // Ciclo do while per modificare i dati ad ogni pressione del tasto SELECT
          if (btnUP) {      // + ボタンが押されていたら
          d = d + stepD;                    // x を指定ステップ増加
          if (d > maxD) {                   // 上限超えたら下限へサキュレート
          d = minD;
          }
      oledDisp2Chr(x, y, d);             // 画面の指定位置に数値を2桁表示（下線付き）
         while (btnUP) {  // + ボタンが離されるまで待つ
                       }
      delay(30);
    }

    if (btnDOWN) {  // - ボタンが押されていたら
      d = d - stepD;                // x を指定ステップ減らす
      if (d < minD) {               // 下限以下なら上限へサーキュレート
        d = maxD;
      }
      oledDisp2Chr(x, y, d);             // 画面の指定位置に数値を2桁表示（下線付き）
      while (btnDOWN) {  // - ボタンが離されるまで待つ
      }
      delay(30);
    }

    if(btnSELECT) break;
    }while(1);

  //lcd.noBlink();
  lcd.noCursor();
  delay(30);
  return d;  // 設定した値を戻り値で返す
}

void oledDisp2Chr(int x, int y, int val) {       // OLEDの指定場所に2桁の値を表示
  sprintf(cBuff, "%02d", val);                   // データーを10進2桁0フィル文字列に変換
  lcd.setCursor(x, y);                          // カーソルを指定位置に合わせて
  lcd.print(cBuff);                             // 数値を書き込み
  lcd.setCursor(x+1,y); // Cursore sotto la seconda cifra da impostare
  lcd.cursor();
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

  lcd.clear();   // 画面消去
  lcd.setCursor(0, 0);  // カーソルを1行目の先頭に移動

  sprintf(cBuff, "%02d/", currentTime.getDayOfMonth());  // 2桁で日の文字列作成 +空白
  Serial.print(cBuff); 
  lcd.print(cBuff);

  sprintf(cBuff, "%02d/", Month2int(currentTime.getMonth()));  // 2桁で月の文字列作成 +/
  Serial.print(cBuff); 
  lcd.print(cBuff);

  sprintf(cBuff, "%4d ", currentTime.getYear());  // 4桁で年の文字列作成 +/
  Serial.print(cBuff); 
  lcd.print(cBuff);                              // OLEDに出力

    lcd.setCursor(0, 1);                          // OLEDの2行目に、
  sprintf(cBuff, "%02d:", currentTime.getHour());  // 2桁で時 +:
  Serial.print(cBuff); 
  lcd.print(cBuff);

  sprintf(cBuff, "%02d:", currentTime.getMinutes());  // 2桁で分 +:
  Serial.print(cBuff); 
  lcd.print(cBuff);

  sprintf(cBuff, "%02d", currentTime.getSeconds());  // 2桁で秒
  Serial.print(cBuff); 
  lcd.print(cBuff);

  wk = "<"                                                                       // <
       + String(dispWeekName[DayOfWeek2int(currentTime.getDayOfWeek(), 0) % 7])  //  曜日文字生成
       + ">";                                                                    //  　　　　>
  Serial.print(" ");                                                             // シリアルには1文字開けて
  Serial.print(wk);                                                              // 曜日を出力
  lcd.setCursor(9, 1);
  lcd.print(wk);
  Serial.println();  // シリアルを改行
}

void periodicCallback() {  // RTC割り込み処理(0.5秒周期でコール）
  irqFlag = true;          // フラグを立ててメインに通知
}