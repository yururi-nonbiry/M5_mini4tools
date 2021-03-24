// include
#include <M5StickC.h> // M5stick
#include <Wire.h> // I2C
#include <VL53L0X.h> //ToFセンサー
#include "BluetoothSerial.h" // シリアルBT
#include <EEPROM.h> // eeprom
#include "esp_deep_sleep.h" //スリープ用
#include "esp_sleep.h"

//距離センサー定義
VL53L0X sensor; // 距離センサー定義
BluetoothSerial SerialBT; // シリアルBT定義

// 各種定数
#define BtnA_pin 37
#define BtnB_pin 39

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

//byte menu_count = 0;

// メインルーチンのメニューカウント用
byte menu_count = 0 ;
byte BtnA_trig = 0; //ボタンを押したかの判定用
byte BtnB_trig = 0; //ボタンを押したかの判定用
volatile unsigned long BtnA_pushed = 0; //ボタンを押した時間
volatile unsigned long BtnB_pushed = 0; //ボタンを押した時間
byte draw_trig = 1;

// EEPROMの構造体
struct set_data {
  char SBTDN[21] ; //serial bluetooth device name
  byte RT_WC ; // rap time waiting count
  byte RT_WD ; // rap time waiting distance(mm)
  byte RT_TD ; // rap time trig distance(mm)
  byte RT_IT ; // rap time invalid time(sec)
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
  eeprom_write(); // EEPROM書き込み
}

// EEPROMへ書き込み
void eeprom_write() {
  EEPROM.put <set_data> (0, set_data_buf); // EEPROMへの書き込み
  EEPROM.commit(); // これが必要らしい
}


// 割り込み処理(ボタン関係)
void BtnA_push() {
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

  /*  //sleep設定用
    esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON); // Deep Sleep中にPull Up を保持するため
    // Deep Sleep中にメモリを保持するため
    esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
    esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_ON);
    esp_deep_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);
  */
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

  //esp_deep_sleep_enable_timer_wakeup(3 * 1000 * 1000);//3秒ごとに復帰(バッテリー確認の為)
  //esp_sleep_enable_timer_wakeup(1000000LL * 3);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_37, 0);//37番ピンでスリープ復帰

  delay(100);
  //esp_deep_sleep_start(); //スリープスタート(復帰は先頭から)
  esp_light_sleep_start();

}

//マルチタスク　core0
//距離センサ関係
void sub_task(void* param) {

  sensor.init();

  delay(100);
  // ここから繰り返し
  while (1) {

    if (sub_task_status == 0) { //何もしないモード

      delay(100);

    } else if (sub_task_status == 1) { //距離測定モード

      sensor.setTimeout(1000);
      sensor.startContinuous(0);//連続読み取りモード
      sensor.setMeasurementTimingBudget(20000);
      sensor.startContinuous(); //連続読取スタート

      while (sub_task_status == 1) {
        time_count = millis(); // 1ループのタイム計測用
        distance_read = sensor.readRangeContinuousMillimeters(); //距離読取(連続測定モード)

        if (sensor.timeoutOccurred()) { //センサータイムアウト時の処理()
          // ここの処理はしなくても大丈夫
        }
        process_time = millis() - time_count ; // 1ループの時間計測
        delay(5);
      }
      sensor.stopContinuous(); //測定停止

    } else if (sub_task_status == 2) { // ラップタイム用

      sensor.setTimeout(1000);
      sensor.startContinuous(0);//連続読み取りモード
      sensor.setMeasurementTimingBudget(20000);
      sensor.startContinuous(); //連続読取スタート

      rap_time_start = 0 ; // スタート時間初期化
      rap_time_end =  0 ; // ストップ時間初期化

      int distance_average ; // 距離の平均値を入れる変数

      while (sub_task_status == 2) { // センサーの距離が安定するまで待つモード
        int distance_count[255]; // 距離を格納するための変数
        int distance_max = 0 ; // 最大値を初期化
        int distance_min = 5000 ; // 最小値を初期化
        int distance_read ;
        for (int count = 0; count < set_data_buf.RT_WC; count++) { // 距離を繰り返し測定する
          distance_read = sensor.readRangeContinuousMillimeters(); // 距離測定
          if ( distance_max < distance_read )distance_max = distance_read; // 最大値の更新
          if ( distance_min > distance_read )distance_min = distance_read; // 最小値の更新
          delay(5); // wdtリセット用
        }

        if (distance_max - distance_min < set_data_buf.RT_WD) { // 距離の変動が一定以下なら抜ける
          distance_average = (distance_max + distance_min ) / 2; //最大値と最小値の中心値を入れる(これが判定の基準値となる)
          sub_task_status = 3; // ループを抜ける為、モードを3にする
        }
      }

      while (sub_task_status == 3 ) { // スタート待ち
        if ( distance_average - sensor.readRangeContinuousMillimeters() > set_data_buf.RT_TD) { // 基準値から一定の距離近づいたらスタートする
          rap_time_start = millis(); // スタートの時間を記録する
          break; // whileを抜ける
        }
        delay(5); // wdtリセット用
      }
      delay(1000 * set_data_buf.RT_IT); // 即停止しないように待つ
      while (sub_task_status == 3) { // ストップ待ち
        if (distance_average - sensor.readRangeContinuousMillimeters() > set_data_buf.RT_TD) { // 基準値から一定の距離近づいたらストップする
          rap_time_end = millis(); // ストップの時間を記録する
          break; // whileを抜ける
        }
        delay(5); // wdtリセット用
      }
      sensor.stopContinuous(); //測定停止
    }
    delay(10); // wdtリセット用


  }
}

//テスト用ルーチン(距離の測定とそれにかかる時間の確認用)
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
    //M5.update(); // ボタンの状態を更新
    //if (M5.BtnB.wasReleased()) {
    if (BtnB_trig != 0) {
      BtnB_trig = 0;
      break;
    }
    delay(100);
  }
  sub_task_status = 0; // サブタスクを待機に設定
  M5.Lcd.fillScreen(BLACK);  // 画面をクリア
  menu_lcd_draw(); // メニュー用のlcd表示
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
  M5.Lcd.setCursor(5, 10);
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
  M5.Lcd.setCursor(5, 10);
  if (rap_hour < 10)M5.Lcd.print(F("0"));
  M5.Lcd.print(rap_hour);
  M5.Lcd.print(F(":"));
  if (rap_minute < 10)M5.Lcd.print(F("0"));
  M5.Lcd.print(rap_minute);
  M5.Lcd.print(F("\'"));

  //タイム表示拡大
  M5.Lcd.setTextSize(4);          // 文字のサイズ
  M5.Lcd.setCursor(10, 30);
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
      //M5.update(); // ボタンの状態を更新
      //if (M5.BtnB.wasReleased()) { //ボタンを押したら戻る
      if (BtnA_trig != 0) {
        BtnA_trig = 0;
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
      //M5.update(); // ボタンの状態を更新
      //if (M5.BtnB.wasReleased()) { //ボタンを押したら戻る
      if (BtnA_trig != 0) {
        BtnA_trig = 0;
        sub_task_status = 0; // サブタスクを待機にする
        menu_lcd_draw(); // メニューを表示する
        return;
      }
      power_supply_draw(); // バッテリー表示
      delay(10);
    }
    while (rap_time_end == 0 ) { // ラップタイムの終わりが来るまでループする

      //途中で抜ける用
      // M5.update(); // ボタンの状態を更新
      //if (M5.BtnB.wasReleased()) { //ボタンを押したら戻る
      if (BtnA_trig != 0) {
        BtnA_trig = 0;
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

    sleep_start();

    while (1) {
      //途中で抜ける用
      //M5.update(); // ボタンの状態を更新
      //if (M5.BtnB.wasReleased()) { //Bボタンを押したら戻る
      if (BtnB_trig != 0) {
        BtnB_trig = 0;
        sub_task_status = 0; // サブタスクを待機にする
        menu_lcd_draw(); // メニューを表示する
        return;
      } else if (BtnA_trig != 0) {
        BtnA_trig = 0;
        //} else if (M5.BtnA.wasReleased()) { // 再スタート用
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


//セットアップ カスタム
void setup_c() {
  //ここにsetupを移行(deep sleep 対応の為)

  //Serial.begin(115200);

}

//セットアップ
void setup()
{
  //deep sleep復帰時のコントロールの為、ここには復帰時も実行する物を書く
  Wire.begin(); //i2cスタート
  EEPROM.begin(1024); //EEPROM開始(サイズ指定)
  EEPROM.get <set_data>(0, set_data_buf); // EEPROMを読み込む
  setCpuFrequencyMhz(20);//CPU周波数変更

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


  //マルチタスク用の宣言
  xTaskCreatePinnedToCore(sub_task, "sub_task", 8192, NULL, 1, NULL, 0);

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
  /*
    //ここでスリープからの復帰なのかの判断をする
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_EXT0) {
    //トリガーで戻ったときの処理
    } else if (cause == ESP_SLEEP_WAKEUP_TIMER) {
    //タイマーで戻ったときの処理
    } else {
    //通常起動時の処理
    setup_c();
    }
  */
  //画面クリア
  M5.Lcd.setHighlightColor(TFT_BLACK);
  menu_lcd_draw();

  M5.Lcd.setTextSize(2);          // 文字のサイズ
  M5.Lcd.setCursor(5, 10);
  M5.Lcd.print(F("senser connect"));


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
      if (menu_count == 1)setting();
      if (menu_count == 2)test();
    }
    power_supply_draw();
    delay(10);
  }
}
