/*
   FFT_display.h
*/
#ifndef INCLUDED_FFT_display_h_
#define INCLUDED_FFT_display_h_
#include <Arduino.h>
#include "arduinoFFT.h"
#include <M5StickC.h> // M5stickC

#define SAMPLES_MAX 2048 // 2のn乗にする
//#define sampling_frequency 50000 // Hz ADCの速度による max50000Hz

#define analog_pin 32 // 電圧読み取り用のピン
//#define analog_pin 36 // 電圧読み取り用のピン

#define lcd_vertical 80

class FFT_display {
  public:
    FFT_display();
    void begin(void);

    void ad_protting(void);
    void fft_protting(void);
    void fft_drawing(void);
    void ad_calibration(void);
    void raw_send(void);

    int current_offset = 0; // 電流補正のオフセット値
    int current_gain = 100; // 電流補正のゲイン値

  private:
    // FFT用
    arduinoFFT FFT = arduinoFFT();

    int samples ; //サンプリング数
    int sampling_frequency = 50000; //サンプリング周期(Hz) max50000Hz

    unsigned int sampling_period_us ;
    unsigned long microseconds ;

    double vReal[SAMPLES_MAX];
    double vImag[SAMPLES_MAX];

    float fft_peak ; //FFT peak saving

    void ad_conversion(void);
    void fft_processing(void);
    //ad_origin = ad_ave
    float ad_origin = 0 ; // AD変換用0ポイント
    float rpm_value = 0 ; // rpm計算用
    float ad_ave = 0; //AD変換平均
    float current_value = 0 ; // 電流計算用


};

#endif
