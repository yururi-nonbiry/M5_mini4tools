/*
  BT_set.cpp
*/

#include <Arduino.h>
#include "BT_set.h"

BT_set::BT_set() {
  //_pin = 1;
  //pinMode(_pin, OUTPUT);
}


void BT_set::bt_connect() {
  SerialBT.begin(set_data_buf.SBTDN); // bluetoothスタート
  SerialBT.setTimeout(100); // タイムアウトまでの時間を設定
}

void BT_set::bt_disconnect() {
  SerialBT.begin(set_data_buf.SBTDN); // bluetoothスタート
  SerialBT.setTimeout(100); // タイムアウトまでの時間を設定
}

void BT_set::eeprom_begin() {
  EEPROM.begin(1024); //EEPROM開始(サイズ指定)
  EEPROM.get <set_data>(0, set_data_buf); // EEPROMを読み込む
}

// EEPROMリセット
void BT_set::eeprom_reset() {

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
void BT_set::eeprom_write() {
  EEPROM.put <set_data> (0, set_data_buf); // EEPROMへの書き込み
  EEPROM.commit(); // これが必要らしい
}

void BT_set::read() {
  // 変数定義
  String read_serial = "";
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
}
