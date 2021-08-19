// M5StickC
// 画面サイズ80*160

// インクルードファイル
#include <M5StickC.h> // M5stickC
//#include "BluetoothSerial.h" // シリアルBT
//#include <EEPROM.h> // eeprom
#include "esp_deep_sleep.h" //スリープ用
#include "esp_sleep.h"
//#include "arduinoFFT.h"
#include "FFT_display.h"
#include "BT_set.h"

FFT_display fft_dis = FFT_display();
BT_set bt_set = BT_set();

//BluetoothSerial SerialBT; // シリアルBT定義

// 各種定数
#define BtnA_pin 37
#define BtnB_pin 39

#define sleep_time 5000 //(ms)経過したらsleepする
#define ir_out 26
#define ir_in 36

// グローバル変数定義
// サブタスク用
volatile int process_time = 0 ; // 処理時間
volatile unsigned long time_count ; // スタート時間記録用
volatile float read_voltage ;//電圧読取用
volatile float read_current ;//電流読取用
volatile unsigned long rap_time_start ; // スタート時間記録用
volatile unsigned long rap_time_end ; // ストップ時間記録用
volatile unsigned long rap_time[10] ; // ラップタイム計算用
volatile byte raptime_count = 0;

// メインルーチンのメニューカウント用
byte menu_count = 0 ;
byte BtnA_trig = 0; //ボタンを押したかの判定用
byte BtnB_trig = 0; //ボタンを押したかの判定用
unsigned long BtnA_pushed = 0; //ボタンを押した時間
unsigned long BtnB_pushed = 0; //ボタンを押した時間
unsigned long action_time = 0; //ボタンを押した時間
byte draw_trig = 1;




// 割り込み処理(ボタン関係)
void BtnA_push() {
  action_time = millis();//操作した時間を更新する
  delayMicroseconds(3000);
  if (digitalRead(BtnA_pin) == LOW) {
    //if (menu_count != 0)menu_count--;
    BtnA_trig = 1;
    BtnB_trig = 0;
    BtnA_pushed = millis();
    draw_trig = 1;
  } else {
    if (BtnA_trig == 1)BtnA_trig = 2;
  }
}

void BtnB_push() {
  action_time = millis();//操作した時間を更新する
  delayMicroseconds(3000);
  if (digitalRead(BtnB_pin) == LOW) {
    //if (menu_count < 2)menu_count++;
    BtnA_trig = 0;
    BtnB_trig = 1;
    BtnB_pushed = millis();
    draw_trig = 1;
  } else {
    if (BtnB_trig == 1)BtnB_trig = 2;
  }
}

//スリープ用
void sleep_start() {

  if (millis() - action_time > sleep_time) { //操作してからある程度時間が空いたらスリープに入る

    //スリープ時にSを表示
    M5.Lcd.setTextSize(1);          // 文字のサイズ
    M5.Lcd.setCursor(5, 5);
    M5.Lcd.print(F("S"));

    /*  //sleep設定用
      esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON); // Deep Sleep中にPull Up を保持するため
      // Deep Sleep中にメモリを保持するため
      esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
      esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_ON);
      esp_deep_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);
    */
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

    esp_deep_sleep_enable_timer_wakeup(3 * 1000 * 1000);//3秒ごとに復帰(バッテリー確認の為)
    //esp_sleep_enable_timer_wakeup(1000000LL * 3);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_37, 0);//37番ピンでスリープ復帰

    delay(100);
    //esp_deep_sleep_start(); //スリープスタート(復帰は先頭から)
    esp_light_sleep_start();

    //復帰した後の処理
    delay(10);

    //"Sを消す"
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(5, 5);
    M5.Lcd.print(F(" "));

    power_supply_draw(); //バッテリー表示
  }
}


// セッティング用サブルーチン (bluetooth serialで設定値を受信するモード)
void setting() {



  setCpuFrequencyMhz(80); //周波数変更(低いと通信できない)
  delay(100);
  //SerialBT.begin(set_data_buf.SBTDN); // bluetoothスタート
  //SerialBT.setTimeout(100); // タイムアウトまでの時間を設定

  bt_set.bt_connect();

  M5.Lcd.fillScreen(BLACK);  // 画面をクリア
  M5.Lcd.setTextSize(2);          // 文字のサイズ
  M5.Lcd.setCursor(10, 15);
  M5.Lcd.print(F("Set by BT"));

  // ここから読取用のルーチン
  while (1) {
    power_supply_draw(); // 電源の状態を更新

    bt_set.read();

    if (BtnB_trig != 0) {
      BtnB_trig = 0;
      //if (M5.BtnB.wasReleased()) { //ボタンを押してたらループから抜ける
      break;
    }
  }
  bt_set.bt_disconnect();
  //SerialBT.disconnect(); // bluetooth ストップ
  //delay(100);
  setCpuFrequencyMhz(20); //周波数変更
  delay(100);
  M5.Lcd.fillScreen(BLACK); // 画面をクリア
  menu_lcd_draw(); // メニューのLCD表示

}


//ラップ表示用
void rap_draw(unsigned long rap) {
  // 画面更新用
  // 変数定義
  byte rap_hour;
  byte rap_minute;
  byte rap_second;
  byte rap_10msec;

  // 時間計算
  rap_hour = rap / (3600 * 1000);
  rap -= rap_hour * 3600 * 1000;
  rap_minute = rap / (60 * 1000);
  rap -= rap_minute * 60 * 1000;
  rap_second = rap / 1000;
  rap -= rap_second * 1000;
  rap_10msec = rap / 10;

  // 画面更新用
  power_supply_draw();
  M5.Lcd.setTextSize(2);          // 文字のサイズ
  M5.Lcd.setCursor(10, 15);
  if (rap_hour < 10)M5.Lcd.print(F("0"));
  M5.Lcd.print(rap_hour);
  M5.Lcd.print(F(":"));
  if (rap_minute < 10)M5.Lcd.print(F("0"));
  M5.Lcd.print(rap_minute);
  M5.Lcd.print(F("\'"));

  //タイム表示拡大
  M5.Lcd.setTextSize(4);          // 文字のサイズ
  M5.Lcd.setCursor(15, 35);
  if (rap_second < 10)M5.Lcd.print(F("0"));
  M5.Lcd.print(rap_second);
  M5.Lcd.print(F("\""));
  if (rap_10msec < 10)M5.Lcd.print(F("0"));
  M5.Lcd.print(rap_10msec);

}

// スタートのトリガがかかった時の処理
void start_trig() {
  rap_time_start = millis();
}

// ストップのトリガがかかったときの処理
void stop_trig() {
  rap_time_end = millis();
}

// ラップタイマー用
void rap_timer() {
  digitalWrite(ir_out, HIGH); // 赤外線センサーの電源をON

  while (1) {
    // 変数初期化(修正)
    rap_time_start = 0;
    rap_time_end = 0;
    //setCpuFrequencyMhz(240); //周波数変更(低いと読み取り速度が足りない)
    //センサーの状態チェック
    M5.Lcd.fillScreen(BLACK);  // 画面をクリア
    M5.Lcd.setTextSize(2);          // 文字のサイズ
    M5.Lcd.setCursor(10, 15);
    M5.Lcd.print(F("Waiting"));

    // 赤外線センサーの状態を確認する
    byte high_count = 0;
    byte low_count = 0;

    while (high_count < 100 && low_count < 100 ) {
      if (digitalRead(ir_in) == HIGH) {
        high_count ++;
        low_count = 0;
      } else {
        low_count ++;
        high_count = 0;
      }
      if (BtnA_trig != 0) { //ボタンを押したら戻る
        digitalWrite(ir_out, LOW); // 赤外線センサーの電源をOFF
        BtnA_trig = 0;
        menu_lcd_draw(); // メニューを表示する
        return;
      }
      delay(10);
    }

    // スタート待ち
    M5.Lcd.fillScreen(BLACK);  // 画面をクリア
    M5.Lcd.setTextSize(2);          // 文字のサイズ
    M5.Lcd.setCursor(10, 15);
    M5.Lcd.print(F("Ready..."));
    attachInterrupt(digitalPinToInterrupt(ir_in), start_trig, CHANGE); // irのピンが変化したら割り込みを行う(start)

    while (rap_time_start == 0 ) { //スタート時間が更新されたらスタートする
      power_supply_draw(); // バッテリー表示
      delay(10);
    }
    detachInterrupt(digitalPinToInterrupt(ir_in)); // irピンの割り込み中止

    // チャタリング防止用の待ち時間
    while (rap_time_start + 3000 > millis() ) { // ラップタイムの終わりが来るまでループする

      //途中で抜ける用
      if (BtnA_trig != 0) {
        digitalWrite(ir_out, LOW); // 赤外線センサーの電源をOFF
        BtnA_trig = 0;
        menu_lcd_draw(); // メニューを表示する
        return;
      }
      // 画面更新用
      rap_time[0] = (millis() - rap_time_start);
      rap_draw(rap_time[0]);
    }

    attachInterrupt(digitalPinToInterrupt(ir_in), stop_trig, CHANGE); // ピンが変化したら割り込みを行う(stop)
    M5.Lcd.fillScreen(BLACK);  // スタートしたら一回画面をクリアする
    power_supply_draw(); // バッテリー表示
    while (rap_time_end == 0 ) { // ラップタイムの終わりが来るまでループする
      //途中で抜ける用
      // M5.update(); // ボタンの状態を更新
      //if (M5.BtnB.wasReleased()) { //ボタンを押したら戻る
      if (BtnA_trig != 0) {
        digitalWrite(ir_out, LOW); // 赤外線センサーの電源をOFF
        BtnA_trig = 0;
        menu_lcd_draw(); // メニューを表示する
        return;
      }
      // 画面更新用
      rap_time[0] = (millis() - rap_time_start)  ;
      rap_draw(rap_time[0]);
    }
    detachInterrupt(digitalPinToInterrupt(ir_in)); // irピンの割り込み中止

    //結果表示
    digitalWrite(ir_out, LOW); // 赤外線センサーの電源をOFF
    rap_time[0] = (rap_time_end - rap_time_start) ;
    rap_draw(rap_time[0]);


    while (1) {
      digitalWrite(ir_out, LOW); // 赤外線センサーの電源をOFF
      sleep_start(); //スリープに入る

      //途中で抜ける用
      if (BtnB_trig != 0) {
        digitalWrite(ir_out, LOW); // 赤外線センサーの電源をOFF
        BtnB_trig = 0;
        menu_lcd_draw(); // メニューを表示する
        return;
      } else if (BtnA_trig != 0) {
        BtnA_trig = 0;
        digitalWrite(ir_out, HIGH); // 赤外線センサーの電源をON
        menu_lcd_draw(); // メニューを表示する
        break;
      }
    }
  }
}


// 赤外線センサーのチェック用
void test() {
  digitalWrite(ir_out, HIGH); // 赤外線センサーの電源をON
  M5.Lcd.fillScreen(BLACK);  // 画面をクリア

  while (1) {

    //センサーの状態チェック
    M5.Lcd.setTextSize(2);          // 文字のサイズ
    M5.Lcd.setCursor(10, 15);
    if (digitalRead(ir_in) == HIGH) {
      M5.Lcd.print(F("HIGH"));
    } else {
      M5.Lcd.print(F("LOW "));
    }

    power_supply_draw(); // バッテリー表示

    //途中で抜ける用
    if (BtnA_trig != 0) {
      digitalWrite(ir_out, LOW); // 赤外線センサーの電源をOFF
      BtnA_trig = 0;
      menu_lcd_draw(); // メニューを表示する
      return;
    }
    delay(10);
  }
}

// 波形用
void waveform_display() {
  setCpuFrequencyMhz(240);//CPU周波数変更
  delay(100);
  fft_dis.ad_calibration();
  while (1) {

    //センサーの状態チェック
    M5.Lcd.setTextSize(2);          // 文字のサイズ
    M5.Lcd.setCursor(10, 15);
    //fft_dis.ad_conversion();
    fft_dis.ad_protting();

    power_supply_draw(); // バッテリー表示

    //途中で抜ける用
    if (BtnA_trig != 0) {
      digitalWrite(ir_out, LOW); // 赤外線センサーの電源をOFF
      BtnA_trig = 0;
      menu_lcd_draw(); // メニューを表示する
      return;
    }
    delay(10);
  }
  setCpuFrequencyMhz(20);//CPU周波数変更
  delay(100);
}


// fft用
void fft_display() {
  setCpuFrequencyMhz(240);//CPU周波数変更
  delay(100);
  fft_dis.ad_calibration();
  while (1) {

    //センサーの状態チェック
    M5.Lcd.setTextSize(2);          // 文字のサイズ
    M5.Lcd.setCursor(10, 15);
    //fft_dis.ad_conversion();
    //fft_dis.fft_processing();
    fft_dis.fft_protting();

    power_supply_draw(); // バッテリー表示

    //途中で抜ける用
    if (BtnA_trig != 0) {
      digitalWrite(ir_out, LOW); // 赤外線センサーの電源をOFF
      BtnA_trig = 0;
      menu_lcd_draw(); // メニューを表示する
      return;
    }
    delay(10);
  }
  setCpuFrequencyMhz(20);//CPU周波数変更
  delay(100);
}


// 解析用
void analytic_display() {
  setCpuFrequencyMhz(240);//CPU周波数変更
  delay(100);
  fft_dis.ad_calibration();
  while (1) {

    //センサーの状態チェック
    M5.Lcd.setTextSize(2);          // 文字のサイズ
    M5.Lcd.setCursor(10, 15);

    //fft_dis.ad_conversion();
    //fft_dis.fft_processing();
    fft_dis.fft_drawing();

    power_supply_draw(); // バッテリー表示

    //途中で抜ける用
    if (BtnA_trig != 0) {
      digitalWrite(ir_out, LOW); // 赤外線センサーの電源をOFF
      BtnA_trig = 0;
      menu_lcd_draw(); // メニューを表示する
      return;
    }
    delay(10);
  }
  setCpuFrequencyMhz(20);//CPU周波数変更
  delay(100);
}


// メニュー表示用
void menu_lcd_draw() {
  M5.Lcd.fillScreen(BLACK);  // 画面をクリア
  power_supply_draw(); // バッテリー表示
  M5.Lcd.setTextSize(2);          // 文字のサイズ
  M5.Lcd.setCursor(10, 15);
  if (menu_count == 0)M5.Lcd.print(F("Rap Timer"));
  if (menu_count == 1)M5.Lcd.print(F("test"));
  if (menu_count == 2)M5.Lcd.print(F("Setting"));
  if (menu_count == 3)M5.Lcd.print(F("waveform_display"));
  if (menu_count == 4)M5.Lcd.print(F("fft_display"));
  if (menu_count == 5)M5.Lcd.print(F("analytic_display"));
  if (menu_count == 6)M5.Lcd.print(F("eeprom_reset"));
}

// 電源関係表示(電圧と電流を表示)
void power_supply_draw() {
  read_voltage = M5.Axp.GetBatVoltage();
  read_current = M5.Axp.GetBatCurrent();
  M5.Lcd.setTextSize(1);          // 文字のサイズ
  M5.Lcd.setCursor(10, 5);
  M5.Lcd.print(F("V:"));
  M5.Lcd.print(String(read_voltage, 2));
  M5.Lcd.print(F(" I:"));
  if (read_current > 0) {
    M5.Lcd.print(F(" "));
    if (read_current < 10)   M5.Lcd.print(F(" "));
    if (read_current < 100)  M5.Lcd.print(F(" "));
  } else {
    if (read_current > -100) M5.Lcd.print(F(" "));
    if (read_current > -10)  M5.Lcd.print(F(" "));
  }
  M5.Lcd.print(String(read_current, 1));
}


//セットアップ
void setup()
{
  //Wire.begin(); //i2cスタート

  fft_dis.begin();
  bt_set.eeprom_begin();
  Serial.begin(115200);
  setCpuFrequencyMhz(20);//CPU周波数変更

  // ピンモード
  digitalWrite(ir_out, LOW);
  pinMode(ir_out, OUTPUT);
  pinMode(ir_in, INPUT);
  //digitalWrite(ir_out, HIGH);

  // ボタンピン割り込み指示
  pinMode(BtnA_pin, INPUT_PULLUP);
  pinMode(BtnB_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BtnA_pin), BtnA_push, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BtnB_pin), BtnB_push, CHANGE);

  //画面は初期化されているのでこっち
  //画面初期化
  M5.begin();
  M5.Axp.ScreenBreath(8);         // バックライトの明るさ(7-15)
  M5.Lcd.setRotation(1);          // 表示の向き
  M5.Lcd.fillScreen(BLACK);   //LCD背景色
  M5.Lcd.setTextFont(1);
  M5.Lcd.setTextSize(2);          // 文字のサイズ
  M5.Lcd.setTextColor(WHITE, BLACK); // 文字の色


}

void eeprom_reset() {
  //EEPROMリセット
  BtnA_trig = 0;
  //M5.update(); // ボタンの状態を更新
  //if (M5.BtnA.isPressed() == 1) {
  M5.Lcd.print(F("EEPROM Reset"));
  delay(1000);
  bt_set.eeprom_reset();
  //eeprom_reset();
  M5.Lcd.fillScreen(BLACK);   // 画面リセット
  menu_lcd_draw(); // メニューを表示する
}

//メインルーチン
void loop()
{

  //画面クリア
  M5.Lcd.setHighlightColor(TFT_BLACK);
  menu_lcd_draw();

  //ここからループ
  while (1) {

    if (BtnB_trig != 0) {
      BtnB_trig = 0;
      menu_count ++ ;
      //if (menu_count < 2) menu_count ++ ;
      if (menu_count == 7)menu_count = 0;
      menu_lcd_draw();
    } else if (M5.Axp.GetBtnPress() == 2) {
      if (menu_count > 0) menu_count --;
      menu_lcd_draw();
      //} else if (M5.BtnA.wasReleased()) {
    } else if (BtnA_trig != 0) {
      BtnA_trig = 0;
      if (menu_count == 0)rap_timer();
      if (menu_count == 1)test();
      if (menu_count == 2)setting();
      if (menu_count == 3)waveform_display();
      if (menu_count == 4)fft_display();
      if (menu_count == 5)analytic_display();
      if (menu_count == 6)eeprom_reset();

    }
    power_supply_draw();
    delay(10);
    sleep_start(); //スリープに入る
  }
}
