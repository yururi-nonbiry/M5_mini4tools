// FFT解析 ノイズ対策用サンプルプログラム

#include <Arduino.h>
#include "arduinoFFT.h"
#include <M5Atom.h>

#define analog_pin 26 // 電圧読み取り用のピン

const int peak_cut = 0;               // ピークカット(0は無効)
const int moving_average = 5;         // 移動平均を行う数
const int SAMPLES = 2048;             // サンプル数
const int sampling_frequency = 50000; // サンプリング周期(Hz) Max50000Hz(ADC)
const int overlap = 3;                // オーバーラップを行う回数
const float overlat_ratio = 0.5;      // オーバーラップでかぶせる割合
const int limit_upper = 4000;         // peakの周波数の上限設定
const int limit_lower = 500;          // peakの周波数の下限設定

//const int SAMPLES_MAX = 2048;
const int SAMPLES_MAX = round(SAMPLES * ((overlap - 1) * overlat_ratio + 1)) + moving_average; // 計算で使用するサンプル数の計算
const int RESULT = round(SAMPLES / 2);                                                         // FFTの結果保存用の変数

double sample[SAMPLES_MAX]; // サンプルを保存する配列
double vReal[SAMPLES];      // FFT計算用の配列
double vImag[SAMPLES];      // FFT計算用の配列
double result_fft[RESULT];  // FFT結果保存用の配列

arduinoFFT FFT = arduinoFFT(); // FFT用

// FFT解析用
void customFFT()
{
  // ピークカット
  // 極端なノイズカット用
  if (peak_cut != 0)
  {
    for (int i = 0; i < SAMPLES_MAX; i++)
    {
      if (sample[i] > peak_cut)
        sample[i] = peak_cut;
    }
  }

  // 移動平均
  // ノイズを均して解析結果を安定させる用（多分）
  // ローパスフィルター
  // ※peakの周波数の上限設定があるから必要ないかも
  for (int i = 0; i < SAMPLES_MAX - moving_average; i++)
  {
    double sample_sum = 0;
    for (int j = 0; j < moving_average; j++)
    {
      sample_sum += sample[i + j];
    }
    sample[i] = sample_sum / moving_average;
  }

  // 配列リセット
  // 結果保存用の配列をリセットする
  for (int i = 0; i < RESULT; i++)
  {
    result_fft[i] = 0;
  }

  // オーバーラップ
  // 複数回解析して安定させるが、サンプルの一部を再利用することにより
  // サンプリングするデータを減らすことが出来る
  for (int i = 0; i < overlap; i++)
  {
    for (int j = 0; j < SAMPLES; j++)
    {
      int sample_count = j + round(SAMPLES * overlat_ratio * i);
      vReal[j] = sample[sample_count];
      vImag[j] = 0;
    }

    /*FFT*/
    // FFT解析(窓関数は元々使っていました)
    FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD); // 窓関数
    FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);                 // FFT解析
    FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);                   // 複素数 to 実数
    //double peak = FFT.MajorPeak(vReal, SAMPLES, sampling_frequency);
    //Serial.println(peak);

    // FFTの解析を結果を加算
    for (int j = 0; j < RESULT; j++)
    {
      result_fft[j] += vReal[j];
    }
  }

  // 結果を平均にする
  for (int i = 0; i < RESULT; i++)
  {
    result_fft[i] /= overlap;
  }
}

// peakの周波数を計算する
float peakFFT()
{
  double peak_value = 0;
  float peak_frequency = 0;
  for (int i = 0; i < RESULT; i++)
  {
    float frequency = (i * 1.0 * sampling_frequency) / SAMPLES;
    if (frequency > limit_lower && frequency < limit_upper)
    {
      if (peak_value < result_fft[i])
      {
        peak_value = result_fft[i];
        peak_frequency = frequency;
      }
    }
  }

  return peak_frequency; // 計算結果を戻す
}

// サンプリング
void sampling()
{

  unsigned long sampling_period_us = round(1000000 * (1.0 / sampling_frequency)); // サンプリング周期(時間)
  unsigned long start_time = micros();                                            // スタート時間保存

  for (int i = 0; i < SAMPLES_MAX; i++)
  {

    while (micros() - start_time < sampling_period_us * i) // オーバーフロー・サンプリング周期ずれ　対策済み
      ;
    sample[i] = analogRead(analog_pin); // データ読取
  }
  unsigned long end_time = micros(); // 終了時間保存

  // サンプリング周期確認用(ESP32でADCが遅くなる場合が有ったので確認用)
  Serial.print(F("Sampling frequency : "));
  Serial.println(1000000.0 / float(end_time - start_time) * SAMPLES_MAX);
}

void setup()
{
  // いつものやつ
  M5.begin();
  pinMode(analog_pin, ANALOG);
  analogSetAttenuation(ADC_11db);
}

void loop()
{
  sampling();                 // サンプリングして
  customFFT();                // FFT解析して
  float peak = peakFFT();     //ピークの周波数を求めて
  Serial.print(F("Peak : ")); //表示する
  Serial.println(peak);
  delay(1000);
}
