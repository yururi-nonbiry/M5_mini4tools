/*
   FFT_display.h
*/
#ifndef INCLUDED_FFT_display_h_
#define INCLUDED_FFT_display_h_
#include <Arduino.h>
#include "arduinoFFT.h"
#include <M5StickC.h> // M5stickC

#define SAMPLES 2048 // 2のn乗にする
#define SAMPLING_FREQUENCY 50000 // Hz ADCの速度による max50000Hz

#define analog_pin 36 // 電圧読み取り用のピン

class FFT_display {
  public:
    FFT_display();
    void begin(void);

    void ad_protting(void);
    void fft_protting(void);
    void fft_drawing(void);

  private:
    // FFT用
    arduinoFFT FFT = arduinoFFT();
    unsigned int sampling_period_us ;
    unsigned long microseconds ;

    double vReal[SAMPLES];
    double vImag[SAMPLES];

    double fft_peak ; //FFT peak saving

    void ad_conversion(void);
    void fft_processing(void);
};

#endif
