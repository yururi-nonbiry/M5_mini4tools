// インクルードファイル
#include <M5StickC.h> // M5stick
#include <Wire.h> // I2C
#include <VL53L0X.h> //ToFセンサー
#include "BluetoothSerial.h" // シリアルBT
#include <EEPROM.h> // eeprom

//距離センサー定義
VL53L0X sensor; // 距離センサー定義
BluetoothSerial SerialBT; // シリアルBT定義

// グローバル変数定義
// サブタスク用
volatile byte sub_task_status = 0 ; //サブタスクの処理指示用
volatile int distance_read ; // 距離読取用
volatile int process_time = 0 ; // 処理時間
volatile unsigned long time_count ; // スタート時間記録用
volatile float read_voltage ;//電圧読取用
volatile float read_current ;//電流読取用
volatile unsigned long rap_time_start ; // スタート時間記録用
volatile unsigned long rap_time_end ; // ストップ時間記録用
volatile unsigned long rap_time ; // ラップタイム計算用

//メインルーチン用
byte menu_count = 0 ;

// EEPROMの構造体宣言
struct set_data {
  char SBTDN[21] ; //serial bluetooth device name
  byte RT_WC ; // rap time waiting count
  byte RT_WD ; // rap time waiting distance(mm)
  byte RT_TD ; // rap time trig distance(mm)
};
set_data set_data_buf; // 構造体宣言


// EEPROMリセット
void eeprom_reset() {

  String str = "rap_timer" ;
  str.toCharArray(set_data_buf.SBTDN, 10); // bluetoothデバイス名
  set_data_buf.RT_WC = 50 ; // rap time waiting count　Type:byte
  set_data_buf.RT_WD = 20 ; // rap time waiting distance(mm) Type:byte
  set_data_buf.RT_TD = 30 ; // rap time trig distance(mm) Type:byte
  eeprom_write();
}

// EEPROMへ書き込み
void eeprom_write() {
  EEPROM.put <set_data> (0, set_data_buf);
  EEPROM.commit();
}


//起動時の表示
void write_display()
{
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print(F("ToF Sensor"));
}

//マルチタスク用のルーチン
//距離センサ関係
void sub_task(void* param) {

  while (1) {

    time_count = millis();

    if (sub_task_status == 0) { //何もしないモード
      delay(100);
    } else if (sub_task_status == 1) { //距離測定モード

      distance_read = sensor.readRangeContinuousMillimeters(); //距離測定

      if (sensor.timeoutOccurred()) {
        //M5.Lcd.printf(" TIMEOUT");
      }
    } else if (sub_task_status == 2) { // ラップタイム用
      rap_time_start = 0 ; // スタート時間初期化
      rap_time_end =  0 ; // ストップ時間初期化
      int distance_average;
      while (sub_task_status == 2) {
        int distance_count[50];
        int distance_max = 0 ;
        int distance_min = 5000 ;
        int distance_read ;
        for (int count = 0; count < set_data_buf.RT_WC; count++) {
          distance_read = sensor.readRangeContinuousMillimeters(); //距離測定
          if ( distance_max < distance_read )distance_max = distance_read;
          if ( distance_min > distance_read )distance_min = distance_read;
          delay(1);
        }

        if (distance_max - distance_min < set_data_buf.RT_WD) { // 距離の変動が一定以下なら抜ける
          distance_average = (distance_max + distance_min ) / 2;
          sub_task_status = 3;
        }
      }
      while (sub_task_status == 3 ) {
        if ( distance_average - sensor.readRangeContinuousMillimeters() > set_data_buf.RT_TD) {
          rap_time_start = millis();
          break;
        }
        delay(1);
      }
      delay(5000);
      while (sub_task_status == 3) {
        if (distance_average - sensor.readRangeContinuousMillimeters() > set_data_buf.RT_TD) {
          rap_time_end = millis();
          break;
        }
        delay(1);
      }
    }
    delay(10);
    process_time = millis() - time_count ;

  }
}

//テスト用ルーチン
void test() {
  sub_task_status = 1; // サブタスクをテスト用に設定
  M5.Lcd.fillScreen(BLACK);  // 画面をクリア
  while (1) {
    power_supply_draw();
    M5.Lcd.setTextSize(2);          // 文字のサイズ
    M5.Lcd.setCursor(5, 10);
    M5.Lcd.print(F("Dist:"));
    if (distance_read < 1000)M5.Lcd.print(F(" "));
    if (distance_read < 100)M5.Lcd.print(F(" "));
    if (distance_read < 10)M5.Lcd.print(F(" "));
    M5.Lcd.print(distance_read);
    M5.Lcd.print(F("mm"));
    M5.Lcd.setCursor(5, 30);
    M5.Lcd.print(F("Time:"));
    if (process_time < 100)M5.Lcd.print(F(" "));
    if (process_time < 10)M5.Lcd.print(F(" "));
    M5.Lcd.print(process_time);
    M5.Lcd.print(F("ms"));
    //M5.Lcd.endWrite();
    M5.update(); // ボタンの状態を更新
    if (M5.BtnB.wasReleased()) {
      break;
    }
    delay(100);
  }
  sub_task_status = 0; // サブタスクを待機に設定
  M5.Lcd.fillScreen(BLACK);  // 画面をクリア
  menu_lcd_draw(); // メニュー用のlcd表示
}

// セッティング用サブルーチン
void setting() {

  // 変数定義
  String read_serial = "";

  setCpuFrequencyMhz(80); //周波数変更
  delay(100);
  SerialBT.begin("rap_time"); // bluetoothスタート
  SerialBT.setTimeout(100); // タイムアウトまでの時間を設定

  M5.Lcd.fillScreen(BLACK);  // 画面をクリア
  M5.Lcd.setTextSize(2);          // 文字のサイズ
  M5.Lcd.setCursor(5, 10);
  M5.Lcd.print(F("Set by BT"));

  // ここから読取用のルーチン
  while (1) {
    power_supply_draw(); // 電源の状態だけ更新する

    if (SerialBT.available() > 0) { //シリアルに文字が送られてきてるか
      read_serial = SerialBT.readStringUntil(';');
      //SerialBT.print(F("Return:"));
      //SerialBT.println(read_serial);
    }
    // スペースと改行を消す
    int search_number = 0; // 文字検索用検索
    while (search_number != -1) { // 消したい文字が見つからなくなるまで繰り返す
      search_number = read_serial.lastIndexOf(" "); // スペースを検索する
      if (search_number = -1)search_number = read_serial.lastIndexOf("\n"); // 改行を検索する
      if (search_number != -1)read_serial.remove(search_number);
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
        SerialBT.println(F("save; Write the setting data to eeprom"));
        SerialBT.println(F("list; List of setting data"));
      } else if (read_serial.equalsIgnoreCase("list") == true) {
        //ここに設定値のリストを書き込む
        SerialBT.println(F("list:"));
        SerialBT.print(F("SBTDN,")); SerialBT.print(set_data_buf.SBTDN); SerialBT.println(F(";")); // rap time waiting count　Type:byte
        SerialBT.print(F("RT_WC,")); SerialBT.print(set_data_buf.RT_WC); SerialBT.println(F(";")); // rap time waiting count　Type:byte
        SerialBT.print(F("RT_WD,")); SerialBT.print(set_data_buf.RT_WD); SerialBT.println(F(";")); // rap time waiting distance(mm) Type:byte
        SerialBT.print(F("RT_TD,")); SerialBT.print(set_data_buf.RT_TD); SerialBT.println(F(";")); // rap time trig distance(mm) Type:byte
      } else if (read_serial.equalsIgnoreCase("save") == true) {
        SerialBT.println(F("save;Write the setting data to eeprom"));
        eeprom_write();
      } else if (comma_position != -1) { // ","があるか判定する
        String item_str = read_serial.substring(0, comma_position ); // 項目読み取り
        String value_str = read_serial.substring(comma_position + 1, read_serial.length()); // 数値読み取り
        //char SBTN[21] ; //serial bluetooth name
        if (item_str == "SBTDN") {
          value_str.toCharArray(set_data_buf.SBTDN, value_str.length() + 1);
          SerialBT.print(F("SBTDN,")); SerialBT.print(set_data_buf.SBTDN); SerialBT.println(F(";")); // rap time waiting count　Type:byte
        }
        if (item_str == "RT_WC") {
          set_data_buf.RT_WC = value_str.toInt();
          SerialBT.print(F("RT_WC,")); SerialBT.print(set_data_buf.RT_WC); SerialBT.println(F(";")); // rap time waiting count　Type:byte
        }
        if (item_str == "RT_WD") {
          set_data_buf.RT_WD = value_str.toInt();
          SerialBT.print(F("RT_WD,")); SerialBT.print(set_data_buf.RT_WD); SerialBT.println(F(";")); // rap time waiting count　Type:byte
        }
        if (item_str == "RT_TD") {
          set_data_buf.RT_TD = value_str.toInt();
          SerialBT.print(F("RT_TD,")); SerialBT.print(set_data_buf.RT_TD); SerialBT.println(F(";")); // rap time waiting count　Type:byte
        }
      }
    }
    read_serial = ""; //　クリア
    power_supply_draw();
    M5.update(); // ボタンの状態を更新
    if (M5.BtnB.wasReleased()) { //ボタンを押してたらループから抜ける
      break;
    }
  }
  SerialBT.disconnect(); // bluetooth ストップ
  delay(100);
  setCpuFrequencyMhz(80); //周波数変更
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
  M5.Lcd.setCursor(5, 10);
  if (rap_hour < 10)M5.Lcd.print(F("0"));
  M5.Lcd.print(rap_hour);
  M5.Lcd.print(F(":"));
  if (rap_minute < 10)M5.Lcd.print(F("0"));
  M5.Lcd.print(rap_minute);
  M5.Lcd.print(F("\'"));
  if (rap_second < 10)M5.Lcd.print(F("0"));
  M5.Lcd.print(rap_second);
  M5.Lcd.print(F("\""));
  if (rap_10msec < 10)M5.Lcd.print(F("0"));
  M5.Lcd.print(rap_10msec);

}

// ラップタイマー用
void rap_timer() {

  while (1) {
    // 変数初期化(修正)
    rap_time_start = 0;
    rap_time_end = 0;

    //センサーの状態チェック
    M5.Lcd.fillScreen(BLACK);  // 画面をクリア
    M5.Lcd.setTextSize(2);          // 文字のサイズ
    M5.Lcd.setCursor(5, 10);
    M5.Lcd.print(F("Waiting"));

    sub_task_status = 2; // サブタスクに距離センサーの変動が落ち着くかの確認をする
    while (sub_task_status == 2 ) { //サブタスクの準備完了まで待つ
      M5.update(); // ボタンの状態を更新
      if (M5.BtnB.wasReleased()) { //ボタンを押したら戻る
        sub_task_status = 0; // サブタスクを待機にする
        menu_lcd_draw(); // メニューを表示する
        return;
      }
      power_supply_draw(); // バッテリー表示
      delay(10);
    }
    // スタート待ち
    M5.Lcd.fillScreen(BLACK);  // 画面をクリア
    M5.Lcd.setTextSize(2);          // 文字のサイズ
    M5.Lcd.setCursor(5, 10);
    M5.Lcd.print(F("Ready..."));

    while (rap_time_start == 0 ) { //スタート時間が更新されたらスタートする
      M5.update(); // ボタンの状態を更新
      if (M5.BtnB.wasReleased()) { //ボタンを押したら戻る
        sub_task_status = 0; // サブタスクを待機にする
        menu_lcd_draw(); // メニューを表示する
        return;
      }
      power_supply_draw(); // バッテリー表示
      delay(10);
    }
    while (rap_time_end == 0 ) { // ラップタイムの終わりが来るまでループする

      //途中で抜ける用
      M5.update(); // ボタンの状態を更新
      if (M5.BtnB.wasReleased()) { //ボタンを押したら戻る
        sub_task_status = 0; // サブタスクを待機にする
        menu_lcd_draw(); // メニューを表示する
        return;
      }
      // 画面更新用
      rap_time = (millis() - rap_time_start)  ;
      rap_draw(rap_time);
    }
    //結果表示
    rap_time = ( rap_time_end - rap_time_start) ;
    rap_draw(rap_time);

    while (1) {
      //途中で抜ける用
      M5.update(); // ボタンの状態を更新
      if (M5.BtnB.wasReleased()) { //Bボタンを押したら戻る
        sub_task_status = 0; // サブタスクを待機にする
        menu_lcd_draw(); // メニューを表示する
        return;
      } else if (M5.BtnA.wasReleased()) { // 再スタート用
        sub_task_status = 0; // サブタスクを待機にする
        menu_lcd_draw(); // メニューを表示する
        break;
      }
    }
  }
}

// メニュー表示用
void menu_lcd_draw() {
  M5.Lcd.fillScreen(BLACK);  // 画面をクリア
  power_supply_draw(); // バッテリー表示
  M5.Lcd.setTextSize(2);          // 文字のサイズ
  M5.Lcd.setCursor(5, 10);
  if (menu_count == 0)M5.Lcd.print(F("Rap Timer"));
  if (menu_count == 1)M5.Lcd.print(F("Setting"));
  if (menu_count == 2)M5.Lcd.print(F("Test"));
}

// 電源関係表示(電圧と電流を表示)
void power_supply_draw() {
  read_voltage = M5.Axp.GetBatVoltage();
  read_current = M5.Axp.GetBatCurrent();
  M5.Lcd.setTextSize(1);          // 文字のサイズ
  M5.Lcd.setCursor(5, 0);
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
  Wire.begin();
  //Serial.begin(115200);

  EEPROM.begin(2); //EEPROM開始(サイズ指定)
  EEPROM.get <set_data>(0, set_data_buf); // EEPROMを読み込む

  //CPU周波数変更
  setCpuFrequencyMhz(20);

  //マルチタスク用の宣言
  xTaskCreatePinnedToCore(sub_task, "sub_task", 8192, NULL, 1, NULL, 0);

}

//メインルーチン
void loop()
{
  //画面初期化
  M5.begin();
  M5.Axp.ScreenBreath(8);         // バックライトの明るさ(7-15)
  M5.Lcd.setRotation(1);          // 表示の向き
  M5.Lcd.fillScreen(BLACK);   //LCD背景色
  M5.Lcd.setTextFont(1);
  M5.Lcd.setTextSize(2);          // 文字のサイズ
  M5.Lcd.setTextColor(WHITE, BLACK); // 文字の色

  //EEPROMリセット
  M5.update(); // ボタンの状態を更新
  if (M5.BtnA.isPressed() == 1) {
    M5.Lcd.print(F("EEPROM Reset"));
    delay(1000);
    eeprom_reset();
    M5.Lcd.fillScreen(BLACK);   // 画面リセット
  }

  //画面クリア
  M5.Lcd.setHighlightColor(TFT_BLACK);
  menu_lcd_draw();

  M5.Lcd.setTextSize(2);          // 文字のサイズ
  M5.Lcd.setCursor(5, 10);
  M5.Lcd.print(F("senser connect"));

  // ToFセンサー設定
  sensor.init();
  sensor.setTimeout(1000);
  sensor.startContinuous(0);
  sensor.setMeasurementTimingBudget(20000);
  sensor.startContinuous();

  //画面クリア
  M5.Lcd.setHighlightColor(TFT_BLACK);
  menu_lcd_draw();

  //ここからループ
  while (1) {
    M5.update(); // ボタンの状態を更新
    if (M5.BtnB.wasReleased()) {
      if (menu_count < 2) menu_count ++ ;
      menu_lcd_draw();
    } else if (M5.Axp.GetBtnPress() == 2) {
      if (menu_count > 0) menu_count --;
      menu_lcd_draw();
    } else if (M5.BtnA.wasReleased()) {
      if (menu_count == 0)rap_timer();
      if (menu_count == 1)setting();
      if (menu_count == 2)test();
    }
    power_supply_draw();
    delay(10);
  }
}
