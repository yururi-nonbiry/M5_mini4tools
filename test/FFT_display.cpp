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
  //sampling_period_us = round(1000000 * (1.0 / sampling_frequency)); // サンプリング1回あたりの時間
}

// A/D変換のサンプリング
void FFT_display::ad_conversion() {
  sampling_period_us = round(1000000 * (1.0 / sampling_frequency)); // サンプリング1回あたりの時間
  double vReal_ave = 0;
  for (int i = 0; i < samples; i++)
  {
    microseconds = micros();    //Overflows after around 70 minutes! 70分でオーバーフローする

    vReal[i] = analogRead(analog_pin); //38 = analog4
    vImag[i] = 0;

    while (micros() < (microseconds + sampling_period_us)) {
    }
  }
  //　adの平均値を求める
  for (int count = 0; count < samples; count++) {
    vReal_ave += vReal[count];
  }
  ad_ave = vReal_ave / samples;
}


// FFT解析
void FFT_display::fft_processing() {

  FFT.Windowing(vReal, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, samples, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, samples);
  fft_peak = FFT.MajorPeak(vReal, samples, sampling_frequency);

}


// a/d 波形の表示
void FFT_display::ad_protting() {



  //1回FFT解析をしてモーターの周波数帯を求める
  samples = 2048 ; //サンプル数
  sampling_frequency = 10000 ; //サンプリング周期
  sampling_period_us = round(1000000 * (1.0 / sampling_frequency)); // サンプリング1回あたりの時間
  FFT_display::ad_conversion();
  FFT_display::fft_processing();

  // 1回転分(6波分)の周波数が拾えるサンプリング周期を求める
  // 512サンプリング/6回転
  double fft_peak_calc = fft_peak;
  if (fft_peak_calc > 50000)fft_peak_calc = 50000;
  if (fft_peak_calc < 1000)fft_peak_calc = 1000;
  sampling_period_us = round(1000000 * (1.0 / fft_peak_calc));

  samples = 512 ; //表示は512だけれど、スタート地点をずらすためサンプル数を512にする
  FFT_display::ad_conversion();

  int start_point = 0; //グラフのスタートポイント
  double vReal_max = 0; //最大値
  double vReal_min = 4096; //最小値
  double vReal_ave = 0;

  //最大、最小を計算する
  for (int count = 0; count < 512; count++) {
    if (vReal_max < vReal[count])vReal_max = vReal[count];
    if (vReal_min > vReal[count])vReal_min = vReal[count];
    vReal_ave += vReal[count];
  }
  vReal_ave = vReal_ave / 512;


  //スタート地点を求める
  for (int count = 2; count < 512; count++) {
    if (vReal_ave > (vReal[count - 2] + vReal[count - 1] + vReal[count]) / 3) {
      start_point = count;
      break;
    }
  }
  for (int count = start_point; count < 512; count++) {
    if (vReal_ave < (vReal[count - 2] + vReal[count - 1] + vReal[count]) / 3) {
      start_point = count;
      break;
    }
  }

  //もう一回最大、最小を計算する
  vReal_max = 0; //最大値
  vReal_min = 4096; //最小値
  for (int count = start_point ; count < 256 + start_point; count++) {
    if (vReal_max < vReal[count])vReal_max = vReal[count];
    if (vReal_min > vReal[count])vReal_min = vReal[count];
  }



  // 波形を画面に書き出す
  M5.Lcd.drawLine( 41, 19, 41, 219, BLACK);
  M5.Lcd.drawLine( 42, 19, 42, 219, BLACK);
  for (int count = 1; count <= 255; count++) {
    //double calc_value = vReal[count] * 240 / 4096;

    M5.Lcd.drawLine( count - 1 + 41 , round(200 - (vReal[count - 1 + start_point - 1] - vReal_min) * 200 / (vReal_max - vReal_min)) + 19 , count + 41 , round(200 - (vReal[count + start_point - 1] - vReal_min) * 200 / (vReal_max - vReal_min)) + 19 , YELLOW);
    M5.Lcd.drawLine( count + 42, 19, count + 42, 219, BLACK);
    //M5.Lcd.drawPixel( count + 40 , round(200 - vReal[count] * 200 / 4096) + 20  , WHITE);
  }
}



// fft 解析の表示
void FFT_display::fft_protting() {

  samples = 512 ; //サンプル数
  sampling_frequency = 10000 ; //サンプリング周期

  FFT_display::ad_conversion();
  FFT_display::fft_processing();


  double vReal_max = 0;
  //最大値判定
  for (int count = 2; count <= 255; count++) {
    if (vReal[count] > vReal_max)vReal_max = vReal[count];
  }

  //M5.Lcd.fillScreen(BLACK);


  M5.Lcd.drawLine( 41, 19, 41, 219, BLACK);
  M5.Lcd.drawLine( 42, 19, 42, 219, BLACK);

  for (int count = 3; count <= 255; count++) {
    //double calc_value = vReal[count] * 240 / 4096;

    M5.Lcd.drawLine( count - 1 + 41 , round(200 - vReal[count - 1] * 200 / vReal_max ) + 19 , count + 41 , round(200 - vReal[count] * 200 / vReal_max ) + 19 , YELLOW);
    M5.Lcd.drawLine( count + 42, 19, count + 42, 219, BLACK);
    //M5.Lcd.drawPixel( count + 40 , round(200 - vReal[count] * 200 / 4096 ) + 20 , WHITE);
  }

  for (int i = 0; i < (samples / 2); i++)
  {
    /*View all these three lines in serial terminal to see which frequencies has which amplitudes*/

    //Serial.print((i * 1.0 * sampling_frequency) / samples, 1);
    //Serial.print(" ");
    //Serial.println(vReal[i], 1);    //View only this line in serial plotter to visualize the bins
  }
}



// FFT解析の数値データの表示
void FFT_display::fft_drawing() {

  samples = 2048 ; //サンプル数
  sampling_frequency = 10000 ; //サンプリング周期

  FFT_display::ad_conversion();
  current_value = ad_ave - ad_origin; //電流を計算する
  FFT_display::fft_processing();

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2); // 文字のサイズ
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK); // 文字の色
  M5.Lcd.print(F("FFT peak :"));
  float fft_peak_f = fft_peak;
  M5.Lcd.print(fft_peak);
  M5.Lcd.setCursor(0, 16);
  M5.Lcd.print(F("RPM :"));
  M5.Lcd.print(fft_peak * 10);
  M5.Lcd.setCursor(0, 32);
  M5.Lcd.print(F("Current :"));
  M5.Lcd.print(current_value / 200000 * current_gain + current_offset);
  Serial.print(fft_peak * 10);
  Serial.print(",");
  Serial.println(current_value / 200000 * current_gain + current_offset);

}

void FFT_display::ad_calibration() {

  // 電流の変転設定用
  M5.Lcd.fillScreen(TFT_BLACK);   //LCD背景色
  M5.Lcd.setTextSize(1);          // 文字のサイズ
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK); // 文字の色
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print(F("Calibration"));
  delay(1000); // A/D変換が落ち着くまで待つ
  samples = 2048 ; //サンプル数
  sampling_frequency = 10000 ; //サンプリング周期
  //sampling_period_us = round(1000000 * (1.0 / sampling_frequency)); // サンプリング1回あたりの時間
  ad_conversion(); // A/D読み取り
  ad_origin = ad_ave; //AD変換平均を原点にする

}

