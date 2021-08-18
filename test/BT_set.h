/*
   BT_set.h
*/
#ifndef INCLUDED_BT_set_h_
#define INCLUDED_BT_set_h_
#include <Arduino.h>
#include <M5StickC.h> // M5stickC
#include "BluetoothSerial.h" // シリアルBT
#include <EEPROM.h> // eeprom


class BT_set {
  public:


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

    BT_set();
    void eeprom_begin(void);
    void bt_connect(void);
    void bt_disconnect(void);
    void eeprom_reset();
    void eeprom_write();
    void read();

  private:
    BluetoothSerial SerialBT; // シリアルBT定義
};

#endif
