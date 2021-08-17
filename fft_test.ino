// M5Stack Fast Fourier Transform Analysis
// TFT 320*240

// include file
#include <M5Stack.h>
#include "arduinoFFT.h"

//define list
#define SAMPLES_MAX 2048 // サンプリング数の最大値を入れる(配列の確保の為)
//#define SAMPLING_FREQUENCY 50000 // Hz ADCの速度による max50000Hz

// 各種定数
#define BtnA_pin 39
#define BtnB_pin 38
#define BtnC_pin 37

// 変数定義
byte menu_count = 0;
byte draw_trig = 1;
byte menu_mode = 0;

int ad_origin = 0 ; // AD変換用0ポイント
int rpm_value = 0 ; // rpm計算用
double current_value = 0 ; // 電流計算用
double ad_ave = 0; //AD変換平均

// FFT
arduinoFFT FFT = arduinoFFT(); //クラス宣言

int samples ; //サンプリング数
int sampling_frequency ; //サンプリング周期(Hz) max50000Hz

unsigned int sampling_period_us ;
unsigned long microseconds ;

double vReal[SAMPLES_MAX];
double vImag[SAMPLES_MAX];

double fft_peak ; //FFT peak saving

// /FFT

// 割り込み処理(ボタン関係)
void BtnA_push() {
  if (digitalRead(BtnA_pin) == LOW) {
    if (menu_count != 0)menu_count--;
    draw_trig = 1;
  }
}

void BtnB_push() {
  if (digitalRead(BtnB_pin) == LOW) {
    if (menu_count < 2)menu_count++;
    draw_trig = 1;
  }
}

void BtnC_push() {
  if (digitalRead(BtnC_pin) == LOW) {
    menu_mode++;
    draw_trig = 1;
  }
  if (menu_mode >= 3)menu_mode = 0;
}



// A/D変換のサンプリング
void ad_conversion() {
  double vReal_ave = 0;
  for (int i = 0; i < samples; i++)
  {
    microseconds = micros();    //Overflows after around 70 minutes! 70分でオーバーフローする

    vReal[i] = analogRead(36); //38 = analog4
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
void fft_processing() {
  // ここからFFT解析
  FFT.Windowing(vReal, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, samples, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, samples);
  fft_peak = FFT.MajorPeak(vReal, samples, sampling_frequency);
}

// a/d convert protting
void ad_protting() {

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

// fft protting
void fft_protting() {

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

    //Serial.print((i * 1.0 * SAMPLING_FREQUENCY) / SAMPLES, 1);
    //Serial.print(" ");
    //Serial.println(vReal[i], 1);    //View only this line in serial plotter to visualize the bins
  }
}

void fft_drawing() {
  //M5.Lcd.fillScreen(BLACK);
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
  M5.Lcd.print(current_value / 200);
  Serial.print(fft_peak * 10);
  Serial.print(",");
  Serial.println(current_value / 200);


}

// セットアップファイル
void setup() {

  M5.begin();
  M5.Power.begin();
  M5.Lcd.setBrightness(200);
  M5.Lcd.fillScreen(TFT_BLACK);   //LCD背景色
  M5.Lcd.setTextSize(1);          // 文字のサイズ
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK); // 文字の色
  Serial.begin(115200);

  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK); // 文字の色
  M5.Lcd.print(F("Booting"));

  analogReadResolution(12); // ADCを12bitに設定
  analogSetAttenuation(ADC_11db); // ADCの全チャンネルを -11dBに設定(アッテネータ)

  dacWrite(25, 0); // Speaker OFF

  // ボタンピン割り込み指示
  pinMode(BtnA_pin, INPUT_PULLUP);
  pinMode(BtnB_pin, INPUT_PULLUP);
  pinMode(BtnC_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BtnA_pin), BtnA_push, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BtnB_pin), BtnB_push, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BtnC_pin), BtnC_push, CHANGE);

  // 電流の変転設定用
  M5.Lcd.fillScreen(TFT_BLACK);   //LCD背景色
  M5.Lcd.setTextSize(1);          // 文字のサイズ
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK); // 文字の色
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print(F("Calibration"));
  delay(3000); // A/D変換が落ち着くまで待つ
  samples = 2048 ; //サンプル数
  sampling_frequency = 10000 ; //サンプリング周期
  sampling_period_us = round(1000000 * (1.0 / sampling_frequency)); // サンプリング1回あたりの時間
  ad_conversion(); // A/D読み取り
  ad_origin = ad_ave; //AD変換平均を原点にする

}

void loop() {
  while (1) {
    //M5.update(); // ボタン更新
    //if (M5.BtnA.wasPressed()) menu_count ++ ;
    //if (M5.BtnB.wasPressed()) menu_count -- ;
    //if (menu_count == 0) menu_count = 1 ;
    //if (menu_count >= 4) menu_count = 3 ;

    // menu_count 0:波形表示 1:FFT解析 2:FFT解析表示 4:BT_control
    if (draw_trig == 1) {
      draw_trig = 0;
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setTextSize(2); // 文字のサイズ
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK); // 文字の色
      if (menu_count == 0)M5.Lcd.print(F("A/D protting"));
      if (menu_count == 1)M5.Lcd.print(F("FFT protting"));
      if (menu_count == 2)M5.Lcd.print(F("FFT peak"));
      //外線描画
      if (menu_count == 0 or menu_count == 1) {
        M5.Lcd.drawLine(40, 20, 40, 220, WHITE);
        M5.Lcd.drawLine(40, 220, 296, 220, WHITE);
      }
    }
    if (menu_count == 0) {

      //1回FFT解析をしてモーターの周波数帯を求める
      samples = 2048 ; //サンプル数
      sampling_frequency = 10000 ; //サンプリング周期
      sampling_period_us = round(1000000 * (1.0 / sampling_frequency)); // サンプリング1回あたりの時間
      ad_conversion();
      fft_processing();

      // 1回転分(6波分)の周波数が拾えるサンプリング周期を求める
      // 512サンプリング/6回転
      double fft_peak_calc = fft_peak;
      if (fft_peak_calc > 50000)fft_peak_calc = 50000;
      if (fft_peak_calc < 1000)fft_peak_calc = 1000;
      sampling_period_us = round(1000000 * (1.0 / fft_peak_calc));

      samples = 512 ; //表示は512だけれど、スタート地点をずらすためサンプル数を512にする
      //sampling_frequency = 10000 ; //サンプリング周期
      //sampling_period_us = round(1000000 * (1.0 / sampling_frequency)); // サンプリング1回あたりの時間
      ad_conversion();
      ad_protting();

    } else if (menu_count == 1 ) {
      samples = 512 ; //サンプル数
      sampling_frequency = 10000 ; //サンプリング周期
      sampling_period_us = round(1000000 * (1.0 / sampling_frequency)); // サンプリング1回あたりの時間
      ad_conversion();
      fft_processing();
      fft_protting();
    } else if (menu_count == 2) {

      samples = 2048 ; //サンプル数
      sampling_frequency = 10000 ; //サンプリング周期
      sampling_period_us = round(1000000 * (1.0 / sampling_frequency)); // サンプリング1回あたりの時間
      ad_conversion();
      current_value = ad_ave - ad_origin; //電流を計算する
      fft_processing();
      fft_drawing();
    }

    delay(100);  //Repeat the process every second OR:
  }
}
