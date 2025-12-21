 /* Intermittent Chaos Method
  * Simula la luce di una fiamma con un led arancione ...
  * Thanks to radiopench http://radiopench.blog96.fc2.com/blog-date-20250908.html
  * Testato su Arduino Nano R4
  * Un idea per l'illuminazione di un presepe usando led WS2812 e controllati con libreria Adafruit_NeoPixel
  * http://radiopench.blog96.fc2.com/blog-date-20251028.html
*/


float xx = 0.7;
float last_xx;
float xxx;
 
void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("Start ");
}
 
void loop() {
  xx = fluctuate(xx);  // カオス法で新しい明るさを決定
  Serial.print(xx);
  Serial.print(", ");
  // コイルに通電して炎の板を揺らす
  xxx = last_xx - xx;
  // LEDの明るさを変える
  for (int i = 1; i <= 10; i++) {               // チラツキ防止のため、10回に分けて、
    xxx = last_xx + (xx - last_xx) * i / 10.0;  // 指定値まで直線補完で、
    analogWrite(LED_BUILTIN, 70 + xxx * 180);       // LEDの明るさを設定
    delay(10);                                  // 補間1ステップ時間（全体ではこの10倍）
  }
  last_xx = xx;
}
 
float fluctuate(float x) {              // 間欠カオス法で 1/f波形を生成
  if (x < 0.5) {                        // 0.5以上なら
    x = x + 2 * x * x;                  // 2x^2で徐々に増加
  } else {                              // 0.5以下なら
    x = x - 2 * (1.0 - x) * (1.0 - x);  // 2(1-x)^2で徐々に減少
  }                                     //
  if (x < 0.05 || x > 0.95) {           // 結果が上下限を外れていたら、
    x = random(100, 900) / 1000.0;      // 乱数で0.1-0.9の範囲に戻す
  }
  return x;
}