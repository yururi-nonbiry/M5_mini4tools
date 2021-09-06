/*
   HX711_read.cpp
*/

#include <Arduino.h>
#include "HX711_read.h"

#define dat_pin 26 // データ用ピン
#define clk_pin 0 // クロック用ピン

HX711_read::HX711_read() {
  //_pin = 1;
  //pinMode(_pin, OUTPUT);

}

void HX711_read::begin() {

  pinMode(clk_pin, OUTPUT);
  pinMode(dat_pin, INPUT);
  point_value = 0 ;
  point_value = HX711_read::reading();

}

float HX711_read::reading(void) {
  long sum = 0;
  for (int i = 0; i < 30; i++) {
    long data = 0;
    while (digitalRead(dat_pin) != 0);
    for (char i = 0; i < 24; i++) {
      digitalWrite(clk_pin, HIGH);
      delayMicroseconds(1);
      digitalWrite(clk_pin, LOW);
      delayMicroseconds(1);
      data = (data << 1) | (digitalRead(dat_pin));
    }
    digitalWrite(clk_pin, HIGH); //gain=128
    delayMicroseconds(1);
    digitalWrite(clk_pin, LOW);
    delayMicroseconds(1);
    data = data ^ 0x800000;
    //return data;
    sum += data;
  }

  float data = sum / 30;

  data /= 6840;
  data -= point_value ;
  return data ;
  
}


void HX711_read::raw_send(void) {
  uint64_t raw_data[read_count] ;
  for (int i = 0; i < read_count; i++) {
    long data = 0;
    while (digitalRead(dat_pin) != 0);
    for (char i = 0; i < 24; i++) {
      digitalWrite(clk_pin, HIGH);
      delayMicroseconds(1);
      digitalWrite(clk_pin, LOW);
      delayMicroseconds(1);
      data = (data << 1) | (digitalRead(dat_pin));
    }
    digitalWrite(clk_pin, HIGH); //gain=128
    delayMicroseconds(1);
    digitalWrite(clk_pin, LOW);
    delayMicroseconds(1);
    data = data ^ 0x800000;
    //return data;
    raw_data[i] = data;
  }

  //float data = sum / 30;

  //data /= 6840;
  //data -= point_value ;
  //return data ;

  // ここから送信
  Serial.write(0xff); //header
  Serial.write(0xfd); // 荷重センサーはヘッダがFFFE
  for (int i = 0; i < read_count; i++)
  {
    //Serial.println(vReal[i],0); //38 = analog4
    int _data =  raw_data[i];
    uint8_t dataHH = (uint8_t)((_data & 0xFF000000) >>  24);
    uint8_t dataHL = (uint8_t)((_data & 0x00FF0000) >>  16);
    uint8_t dataLH = (uint8_t)((_data & 0x0000FF00) >>  8);
    uint8_t dataLL = (uint8_t)((_data & 0x000000FF) >>  0);
    Serial.write(dataHH);
    Serial.write(dataHL);
    Serial.write(dataLH);
    Serial.write(dataLL);
  }
  
}
