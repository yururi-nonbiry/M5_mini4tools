/*
   FFT_display.cpp
*/

#include <Arduino.h>
#include "FFT_display.h"

FFT_display::FFT_display() {
  //_pin = 1;
  //pinMode(_pin, OUTPUT);

}

void FFT_display::begin() {
  // FFT用
  // Serial.begin(115200);
  analogReadResolution(12); // ADCを12bitに設定
  analogSetAttenuation(ADC_11db); // ADCの全チャンネルを -11dBに設定(アッテネータ)
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY)); // サンプリング1回あたりの時間
}

// A/D変換のサンプリング
void FFT_display::ad_conversion() {

  for (int i = 0; i < SAMPLES; i++)
  {
    microseconds = micros();    //Overflows after around 70 minutes! 70分でオーバーフローする

    vReal[i] = analogRead(analog_pin); //38 = analog4
    vImag[i] = 0;

    while (micros() < (microseconds + sampling_period_us)) {
    }
  }
}

// FFT解析
void FFT_display::fft_processing() {

  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
  fft_peak = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);

}

// a/d 波形の表示
void FFT_display::ad_protting() {
  ad_conversion();
  // 波形を画面に書き出す
  M5.Lcd.fillScreen(BLACK);

  for (int count = 0; count <= 80; count++) {
    //double calc_value = vReal[count] * 240 / 4096;
    M5.Lcd.drawPixel( count, (round(80 - vReal[count] * 80 / 4096 - 1) - 120) * 2 + 120 , WHITE);
  }

  for (int i = 0; i < (SAMPLES / 2); i++)
  {
    /*View all these three lines in serial terminal to see which frequencies has which amplitudes*/

    //Serial.print((i * 1.0 * SAMPLING_FREQUENCY) / SAMPLES, 1);
    //Serial.print(" ");
    //Serial.println(vReal[i], 1);    //View only this line in serial plotter to visualize the bins
  }
}

// fft 解析の表示
void FFT_display::fft_protting() {
  FFT_display::ad_conversion();
  FFT_display::fft_processing();

  M5.Lcd.fillScreen(BLACK);

  for (int count = 0; count <= 80; count++) {
    //double calc_value = vReal[count] * 240 / 4096;
    M5.Lcd.drawPixel( count, (round(80 - vReal[count] * 80 / 4096 - 1) - 120) * 2 + 120 , WHITE);
  }

  for (int i = 0; i < (SAMPLES / 2); i++)
  {
    /*View all these three lines in serial terminal to see which frequencies has which amplitudes*/

    //Serial.print((i * 1.0 * SAMPLING_FREQUENCY) / SAMPLES, 1);
    //Serial.print(" ");
    Serial.println(vReal[i], 1);    //View only this line in serial plotter to visualize the bins
  }
}

// FFT解析の数値データの表示
void FFT_display::fft_drawing() {
  FFT_display::ad_conversion();
  FFT_display::fft_processing();

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2); // 文字のサイズ
  //M5.Lcd.fillRect(0, 0, 160, 8, TFT_BLACK);
  //M5.Lcd.setCursor(0, 0);
  //M5.Lcd.printf("V:%6.3f  I:%6.1f\n", read_voltage, read_current);

  //M5.Lcd.fillRect(0, 8, 160, 8, TFT_BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK); // 文字の色
  M5.Lcd.print(F("Date :"));
  float fft_peak_f = fft_peak;
  M5.Lcd.print(fft_peak);

}
