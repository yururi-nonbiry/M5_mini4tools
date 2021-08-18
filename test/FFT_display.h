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

#define analog_pin 36 // 電圧読み取り用のピン

class FFT_display {
  public:
    FFT_display();
    void begin(void);

    void ad_protting(void);
    void fft_protting(void);
    void fft_drawing(void);
    void ad_calibration(void);

    int current_offset = 0; // 電流補正のオフセット値
    int current_gain = 200; // 電流補正のゲイン値

  private:
    // FFT用
    arduinoFFT FFT = arduinoFFT();

    int samples ; //サンプリング数
    int sampling_frequency = 50000; //サンプリング周期(Hz) max50000Hz

    unsigned int sampling_period_us ;
    unsigned long microseconds ;

    double vReal[SAMPLES_MAX];
    double vImag[SAMPLES_MAX];

    double fft_peak ; //FFT peak saving

    void ad_conversion(void);
    void fft_processing(void);
    //ad_origin = ad_ave
    int ad_origin = 0 ; // AD変換用0ポイント
    int rpm_value = 0 ; // rpm計算用
    double ad_ave = 0; //AD変換平均
    double current_value = 0 ; // 電流計算用


};

#endif
