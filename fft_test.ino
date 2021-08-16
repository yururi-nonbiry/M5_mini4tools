// M5Stack Fast Fourier Transform Analysis
// TFT 320*240

// include file
#include <M5Stack.h>
#include "arduinoFFT.h"

//define list
#define SAMPLES 2048 // 2のn乗にする
#define SAMPLING_FREQUENCY 50000 // Hz ADCの速度による max50000Hz

// 変数定義
byte menu_count = 1;

// FFT宣言
arduinoFFT FFT = arduinoFFT();

unsigned int sampling_period_us ;
unsigned long microseconds ;

double vReal[SAMPLES];
double vImag[SAMPLES];

double fft_peak ; //FFT peak saving

// A/D変換のサンプリング
void ad_conversion() {

  /*SAMPLING*/
  for (int i = 0; i < SAMPLES; i++)
  {
    microseconds = micros();    //Overflows after around 70 minutes! 70分でオーバーフローする

    vReal[i] = analogRead(36); //38 = analog4
    vImag[i] = 0;

    while (micros() < (microseconds + sampling_period_us)) {
    }
  }
}

// FFT解析
void fft_processing() {
  // ここからFFT解析

  /*FFT*/
  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
  fft_peak = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);

  /*PRINT RESULTS*/
  // Serial.println(peak);     //Print out what frequency is the most dominant.

}

// a/d convert protting
void ad_protting() {
  // 波形を画面に書き出す
  M5.Lcd.fillScreen(BLACK);

  for (int count = 0; count <= 360; count++) {
    //double calc_value = vReal[count] * 240 / 4096;
    M5.Lcd.drawPixel( count, (round(240 - vReal[count] * 240 / 4096 - 1) - 120) * 2 + 120 , WHITE);
  }

  for (int i = 0; i < (SAMPLES / 2); i++)
  {
    /*View all these three lines in serial terminal to see which frequencies has which amplitudes*/

    //Serial.print((i * 1.0 * SAMPLING_FREQUENCY) / SAMPLES, 1);
    //Serial.print(" ");
    //Serial.println(vReal[i], 1);    //View only this line in serial plotter to visualize the bins
  }
}

// fft protting
void fft_protting() {

  M5.Lcd.fillScreen(BLACK);

  for (int count = 0; count <= 360; count++) {
    //double calc_value = vReal[count] * 240 / 4096;
    M5.Lcd.drawPixel( count, (round(240 - vReal[count] * 240 / 4096 - 1) - 120) * 2 + 120 , WHITE);
  }

  for (int i = 0; i < (SAMPLES / 2); i++)
  {
    /*View all these three lines in serial terminal to see which frequencies has which amplitudes*/

    //Serial.print((i * 1.0 * SAMPLING_FREQUENCY) / SAMPLES, 1);
    //Serial.print(" ");
    Serial.println(vReal[i], 1);    //View only this line in serial plotter to visualize the bins
  }
}

void fft_drawing(){
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


void setup() {
  M5.begin();
  M5.Power.begin();
  M5.Lcd.setBrightness(200);
  Serial.begin(115200);

  analogReadResolution(12); // ADCを12bitに設定
  analogSetAttenuation(ADC_11db); // ADCの全チャンネルを -11dBに設定(アッテネータ)

  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY)); // サンプリング1回あたりの時間

  dacWrite(25, 0); // Speaker OFF
}

void loop() {

  M5.update(); // ボタン更新
  if (M5.BtnA.wasPressed()); menu_count ++ ;
  if (M5.BtnB.wasPressed()); menu_count -- ;
  if (menu_count = 0); menu_count = 1 ;
  if (menu_count >= 4); menu_count = 3 ;

  // menu_count 1:波形表示 2:FFT解析 3:FFT解析表示 4:BT_control
  if (menu_count == 1) {
    ad_conversion();
    ad_protting();

  } else if (menu_count == 2 ) {
    ad_conversion();
    fft_processing();
    fft_protting();
  } else if (menu_count == 3) {
    ad_conversion();
    fft_processing();
    
  }

  delay(1000);  //Repeat the process every second OR:
  //while(1);       //Run code once

}
