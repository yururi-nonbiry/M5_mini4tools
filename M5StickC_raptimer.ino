//M5StickC
//画面サイズ80*160

// include
#include <M5StickC.h> // M5stick
#include "BluetoothSerial.h" // シリアルBT
#include <EEPROM.h> // eeprom
#include "esp_deep_sleep.h" //スリープ用
#include "esp_sleep.h"

BluetoothSerial SerialBT; // シリアルBT定義

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

unsigned int sampling_period_us;
unsigned long microseconds;


// メインルーチンのメニューカウント用
byte menu_count = 0 ;
byte BtnA_trig = 0; //ボタンを押したかの判定用
byte BtnB_trig = 0; //ボタンを押したかの判定用
unsigned long BtnA_pushed = 0; //ボタンを押した時間
unsigned long BtnB_pushed = 0; //ボタンを押した時間
unsigned long action_time = 0; //ボタンを押した時間
byte draw_trig = 1;

// EEPROMの構造体
struct set_data {
  char SBTDN[21] ; //serial bluetooth device name
  byte RT_WC ; // rap time waiting count
  byte RT_WD ; // rap time waiting distance(mm)
  byte RT_TD ; // rap time trig distance(mm)
  byte RT_IT ; // rap time invalid time(sec)
  byte RT_FF ; // fft frequency(2-16) Type:byte"
  byte RT_FS ; // fft strength(1-70) Type:byte"
};
set_data set_data_buf; // 構造体宣言

// EEPROMリセット
void eeprom_reset() {

  String str = "rap_timer" ; // bluetoothデバイス名
  str.toCharArray(set_data_buf.SBTDN, 10);
  set_data_buf.RT_WC = 50 ; // rap time waiting count　Type:byte
  set_data_buf.RT_WD = 20 ; // rap time waiting distance(mm) Type:byte
  set_data_buf.RT_TD = 10 ; // rap time trig distance(mm) Type:byte
  set_data_buf.RT_IT = 2 ; // rap time invalid time(sec)
  set_data_buf.RT_FF = 10 ; // fft frequency(2-16)
  set_data_buf.RT_FS = 10; // fft strength（0-70）
  eeprom_write(); // EEPROM書き込み
}

// EEPROMへ書き込み
void eeprom_write() {
  EEPROM.put <set_data> (0, set_data_buf); // EEPROMへの書き込み
  EEPROM.commit(); // これが必要らしい
}

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

  // 変数定義
  String read_serial = "";

  setCpuFrequencyMhz(80); //周波数変更(低いと通信できない)
  delay(100);
  SerialBT.begin(set_data_buf.SBTDN); // bluetoothスタート
  SerialBT.setTimeout(100); // タイムアウトまでの時間を設定

  M5.Lcd.fillScreen(BLACK);  // 画面をクリア
  M5.Lcd.setTextSize(2);          // 文字のサイズ
  M5.Lcd.setCursor(10, 15);
  M5.Lcd.print(F("Set by BT"));

  // ここから読取用のルーチン
  while (1) {
    power_supply_draw(); // 電源の状態を更新

    if (SerialBT.available() > 0) { //シリアルに文字が送られてきてるか
      read_serial = SerialBT.readStringUntil(';');
    }
    // スペースと改行を消す
    int search_number = 0 ; // 文字検索用検索
    while (search_number != -1) { // 消したい文字が見つからなくなるまで繰り返す
      search_number = read_serial.lastIndexOf(" ") ; // スペースを検索する
      if (search_number = -1)search_number = read_serial.lastIndexOf("\n") ; // 改行を検索する
      if (search_number != -1)read_serial.remove(search_number) ; // 該当する文字があったら消す
    }

    // 受信したときの処置
    if (read_serial != "") { // 文字が入っていたら実行する
      int comma_position = read_serial.indexOf(",");
      if (read_serial.equalsIgnoreCase("product") == true) {
        SerialBT.println(F("product:rap_timer_1.0.0"));
      } else if (read_serial.equalsIgnoreCase("help") == true) {
        // ここにヘルプの内容を書き込む
        SerialBT.println(F("help:"));
        SerialBT.println(F("product, Product name and version"));
        SerialBT.println(F("SBTDN, Serial bluetooth device name"));
        SerialBT.println(F("RT_WC, rap time waiting count　Type:byte"));
        SerialBT.println(F("RT_WD, rap time waiting distance(mm) Type:byte"));
        SerialBT.println(F("RT_TD, rap time trig distance(mm) Type:byte"));
        SerialBT.println(F("RT_IT, rap time invalid time(sec) Type:byte"));
        SerialBT.println(F("RT_FF, fft frequency(2-16)) Type:byte"));
        SerialBT.println(F("RT_FS, fft strength(0-70) Type:byte"));
        SerialBT.println(F("save; Write the setting data to eeprom"));
        SerialBT.println(F("list; List of setting data"));
      } else if (read_serial.equalsIgnoreCase("list") == true) {
        //ここに設定値のリストを書き込む
        SerialBT.println(F("list:"));
        SerialBT.print(F("SBTDN,")); SerialBT.print(set_data_buf.SBTDN); SerialBT.println(F(";")); // rap time waiting count　Type:byte
        SerialBT.print(F("RT_WC,")); SerialBT.print(set_data_buf.RT_WC); SerialBT.println(F(";")); // rap time waiting count　Type:byte
        SerialBT.print(F("RT_WD,")); SerialBT.print(set_data_buf.RT_WD); SerialBT.println(F(";")); // rap time waiting distance(mm) Type:byte
        SerialBT.print(F("RT_TD,")); SerialBT.print(set_data_buf.RT_TD); SerialBT.println(F(";")); // rap time trig distance(mm) Type:byte
        SerialBT.print(F("RT_IT,")); SerialBT.print(set_data_buf.RT_IT); SerialBT.println(F(";")); // rap time invalid time(sec) Type:byte
        SerialBT.print(F("RT_FF,")); SerialBT.print(set_data_buf.RT_FF); SerialBT.println(F(";")); // fft frequency(2-16) Type:byte
        SerialBT.print(F("RT_FS,")); SerialBT.print(set_data_buf.RT_FS); SerialBT.println(F(";")); // fft strength(0-70) Type:byte
      } else if (read_serial.equalsIgnoreCase("save") == true) {
        SerialBT.println(F("save;Write the setting data to eeprom"));
        eeprom_write();
      } else if (comma_position != -1) { // ","があるか判定する
        String item_str = read_serial.substring(0, comma_position ); // 項目読み取り
        String value_str = read_serial.substring(comma_position + 1, read_serial.length()); // 数値読み取り
        //char SBTN[21] ; //serial bluetooth name
        if (item_str == "SBTDN") {
          value_str.toCharArray(set_data_buf.SBTDN, value_str.length() + 1);
          SerialBT.print(F("SBTDN,")); SerialBT.print(set_data_buf.SBTDN); SerialBT.println(F(";"));
        }
        if (item_str == "RT_WC") {
          set_data_buf.RT_WC = value_str.toInt();
          SerialBT.print(F("RT_WC,")); SerialBT.print(set_data_buf.RT_WC); SerialBT.println(F(";"));
        }
        if (item_str == "RT_WD") {
          set_data_buf.RT_WD = value_str.toInt();
          SerialBT.print(F("RT_WD,")); SerialBT.print(set_data_buf.RT_WD); SerialBT.println(F(";"));
        }
        if (item_str == "RT_TD") {
          set_data_buf.RT_TD = value_str.toInt();
          SerialBT.print(F("RT_TD,")); SerialBT.print(set_data_buf.RT_TD); SerialBT.println(F(";"));
        }
        if (item_str == "RT_IT") {
          set_data_buf.RT_TD = value_str.toInt();
          SerialBT.print(F("RT_IT,")); SerialBT.print(set_data_buf.RT_IT); SerialBT.println(F(";"));
        }
        if (item_str == "RT_FF") {
          set_data_buf.RT_TD = value_str.toInt();
          SerialBT.print(F("RT_FF,")); SerialBT.print(set_data_buf.RT_FF); SerialBT.println(F(";"));
        }
        if (item_str == "RT_FS") {
          set_data_buf.RT_TD = value_str.toInt();
          SerialBT.print(F("RT_FS,")); SerialBT.print(set_data_buf.RT_FS); SerialBT.println(F(";"));
        }
      }
    }
    read_serial = ""; //　クリア
    power_supply_draw();
    //M5.update(); // ボタンの状態を更新
    if (BtnB_trig != 0) {
      BtnB_trig = 0;
      //if (M5.BtnB.wasReleased()) { //ボタンを押してたらループから抜ける
      break;
    }
  }
  SerialBT.disconnect(); // bluetooth ストップ
  delay(100);
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

void start_trig() {
  rap_time_start = millis();
}

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
    attachInterrupt(digitalPinToInterrupt(ir_in), start_trig, CHANGE); // ピンが変化したら割り込みを行う

    while (rap_time_start == 0 ) { //スタート時間が更新されたらスタートする


      power_supply_draw(); // バッテリー表示
      delay(10);
    }
    detachInterrupt(digitalPinToInterrupt(ir_in));

    //M5.Lcd.setTextSize(2);          // 文字のサイズ
    //M5.Lcd.setCursor(10, 15);
    //M5.Lcd.print(F("Go!     "));
    while (rap_time_start + 3000 > millis() ) { // ラップタイムの終わりが来るまでループする

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
    attachInterrupt(digitalPinToInterrupt(ir_in), stop_trig, CHANGE); // ピンが変化したら割り込みを行う
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
    detachInterrupt(digitalPinToInterrupt(ir_in));
    //結果表示
    digitalWrite(ir_out, LOW); // 赤外線センサーの電源をOFF
    rap_time[0] = ( rap_time_end - rap_time_start) ;
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
      //digitalWrite(ir_out, LOW); // 赤外線センサーの電源をOFF
      BtnA_trig = 0;
      menu_lcd_draw(); // メニューを表示する
      return;
    }
    delay(10);
  }
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
  // if (menu_count == 3)M5.Lcd.print(F("Test_tof"));
  // if (menu_count == 4)M5.Lcd.print(F("Test_fft"));
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
  //deep sleep復帰時のコントロールの為、ここには復帰時も実行する物を書く
  Wire.begin(); //i2cスタート
  EEPROM.begin(1024); //EEPROM開始(サイズ指定)
  EEPROM.get <set_data>(0, set_data_buf); // EEPROMを読み込む
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

  //EEPROMリセット
  if (digitalRead(BtnA_pin) == LOW) {
    BtnA_trig = 0;
    //M5.update(); // ボタンの状態を更新
    //if (M5.BtnA.isPressed() == 1) {
    M5.Lcd.print(F("EEPROM Reset"));
    delay(1000);
    eeprom_reset();
    M5.Lcd.fillScreen(BLACK);   // 画面リセット
  }

}

//メインルーチン
void loop()
{

  //画面クリア
  M5.Lcd.setHighlightColor(TFT_BLACK);
  menu_lcd_draw();

  //ここからループ
  while (1) {
    //M5.update(); // ボタンの状態を更新
    //if (M5.BtnB.wasReleased()) {
    if (BtnB_trig != 0) {
      BtnB_trig = 0;
      menu_count ++ ;
      //if (menu_count < 2) menu_count ++ ;
      if (menu_count == 3)menu_count = 0;
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
      //if (menu_count == 3)test_tof();

    }
    power_supply_draw();
    delay(10);
    sleep_start(); //スリープに入る
  }
}
