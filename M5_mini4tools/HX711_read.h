/*
   HX711_read.h
*/

#ifndef INCLUDED_HX771_read_h_
#define INCLUDED_HX771_read_h_
#include <Arduino.h>
#include <M5StickC.h> // M5stickC


#define dat_pin 0 // データ用ピン
#define clk_pin 26 // クロック用ピン


class HX711_read {
  public:
    HX711_read();
    void begin(void);

    float reading(void);
    void raw_send(void);

  private:
    float point_value = 0;
    byte read_count = 64; // 読み取り回数


};

#endif
