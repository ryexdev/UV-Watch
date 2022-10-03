/*
Lolin WEMOS d1 mini clone
*/

//Increase to burn faster, decrease to be a beautiful tan person
//  -0.1  =  10% not ginger
//  0     =  ginger
//  +0.1  =  10% extra ginger
//y = -267.48x + 3913.6
//Given "x" UV level, "y" returns seconds needed to burn
//We measure time spent in "x" and calc a "% burned"
//(Time Spent at UV level)/(-267.48*(UV level)+3913.6) + ginger index
float GingerIndex = 0;

//Take the time you would normally burn in the sun without protection, 20 minutes will normally produce redness on a light skinned individual.
//Time To Burn
float TTB = 10;

//SPF = burning time X SPF Number
//Example: If you burn in 20 minutes SPF 30 will give you 600 minutes (10 hours) of burning protection from UBV rays.
//input SPF used
float SPFIndex = 30;

float SunScreenTTB = SPFIndex * TTB;
float SunScreenTTBTimer = 3600;
bool SunScreenApplied = true;

const int UVnumReadings = 10;
float UVreadings[UVnumReadings];
int UVreadIndex = 0;
float UVtotal = 0;
float UVaverage = 0;
int SecondsInSun = 0;

float PercentBurned = 0;
float SecondsToBurn = 0;
float PercentAddedToBurn = 0;
float GingerBurn = 0;

#include <Wire.h>
#include "Adafruit_VEML6075.h"
Adafruit_VEML6075 uv = Adafruit_VEML6075();

#include <Adafruit_NeoPixel.h>
#define PIN D4
#define NUMPIXELS 12
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

int LEDBrigthness = 5;

int RedLEDTimer = 0;
int GreenLEDTimer = 0;
float TotalLEDs = 12;

int MeasurementTimer = 0;
int DisplayTimer = 0;
float CurrentReading = 0;

void setup() {
  Serial.begin(115200);
  pixels.begin();
  pixels.setBrightness(LEDBrigthness);
  pixels.show();
  pixels.show();
  delay(100);
  pixels.clear();
  if (!uv.begin()) {
    pixels.setPixelColor(0, pixels.Color(0, 0, 50));
    pixels.setPixelColor(3, pixels.Color(0, 0, 50));
    pixels.setPixelColor(6, pixels.Color(0, 0, 50));
    pixels.setPixelColor(9, pixels.Color(0, 0, 50));
    Serial.println("Failed to communicate with VEML6075 sensor, check wiring?");
    while (1) {
      delay(100);
    }
  }
  Serial.println("Found VEML6075 sensor");
  Serial.println("Program Begin");
  pixels.setPixelColor(0, pixels.Color(0, 50, 0));
  pixels.setPixelColor(3, pixels.Color(0, 50, 0));
  pixels.setPixelColor(6, pixels.Color(0, 50, 0));
  pixels.setPixelColor(9, pixels.Color(0, 50, 0));
  pixels.show();
  delay(1000);
  pixels.clear();
  pixels.show();
}

void loop() {
  MeasurementTimer++;
  DisplayTimer++;

  //-----------------measurement control------------------
  if (MeasurementTimer == 10) {
    CurrentReading = uv.readUVI();
    if (CurrentReading > 12) {
      CurrentReading = 12;
    }
    if (CurrentReading < 0.5) {
      CurrentReading = 0;
    }
    UVtotal = UVtotal - UVreadings[UVreadIndex];
    UVreadings[UVreadIndex] = CurrentReading;
    UVtotal = UVtotal + UVreadings[UVreadIndex];
    UVreadIndex++;
    if (UVreadIndex >= UVnumReadings) {
      UVreadIndex = 0;
    }
    MeasurementTimer = 0;
  }
  //-----------------display control------------------
  if (DisplayTimer == 100) {
    UVaverage = UVtotal / UVnumReadings;
    if (UVaverage < 0.1) {
      UVaverage = 0;
      if (PercentBurned > 0) {
        PercentBurned = PercentBurned - .001;
      } else {
        SecondsInSun = 0;
        pixels.clear();
      }
    } else {
      SecondsInSun++;
      if (SunScreenTTBTimer == 0) {
        SunScreenApplied = false;
      }
      if (SunScreenApplied == true) {
        SunScreenTTBTimer--;
        SecondsToBurn = (-267.48 * UVaverage + 3913.6) * SPFIndex;
      } else {
        SecondsToBurn = -267.48 * UVaverage + 3913.6;
      }
      PercentAddedToBurn = (1 / SecondsToBurn) * 100;
      GingerBurn = SecondsToBurn * GingerIndex;
      PercentBurned = PercentBurned + PercentAddedToBurn + GingerBurn;
    }
    if (PercentBurned > 100) {
      PercentBurned = 100;
    }
    if (PercentBurned < 0) {
      PercentBurned = 0;
    }

    Serial.print("Actual - ");
    Serial.println(CurrentReading);
    Serial.print("Average - ");
    Serial.println(UVaverage);
    Serial.print("% burned - ");
    Serial.println(PercentBurned);
    Serial.print("Seconds in sun - ");
    Serial.println(SecondsInSun);
    Serial.print("Time to Burn (min)- ");
    Serial.println(((100 / PercentBurned) * SecondsInSun) / 60);
    Serial.print("Sunscreen applied? - ");
    Serial.println(SunScreenApplied);
    Serial.print("Sunscreen left - ");
    Serial.println(SunScreenTTBTimer);
    Serial.println("----------------");
    if (PercentBurned < 100) {
      /*Full ring
      for (float PixelLocation = 0; PixelLocation < (TotalLEDs * (PercentBurned / 100)); PixelLocation++) {
        RedLEDTimer = 255 * (PixelLocation / TotalLEDs);
        GreenLEDTimer = 255 - RedLEDTimer;
        pixels.setPixelColor(PixelLocation, pixels.Color(RedLEDTimer, GreenLEDTimer, 0));
        pixels.show();
      }
      for (float PixelLocation = TotalLEDs; PixelLocation >= (TotalLEDs * (PercentBurned / 100)); PixelLocation--) {
        pixels.setPixelColor(PixelLocation, pixels.Color(0, 0, 0));
        pixels.show();
      }
      */
      for (float PixelLocation = 0; PixelLocation < TotalLEDs; PixelLocation++) {
        if (PixelLocation == int(TotalLEDs * (PercentBurned / 100)) && PercentBurned > 0) {
          RedLEDTimer = 255 * (PixelLocation / TotalLEDs);
          GreenLEDTimer = 255 - RedLEDTimer;
          pixels.setPixelColor(PixelLocation, pixels.Color(RedLEDTimer, GreenLEDTimer, 0));
          pixels.show();
        } else {
          pixels.setPixelColor(PixelLocation, pixels.Color(0, 0, 0));
          pixels.show();
        }
      }
    } else {
      pixels.setBrightness(255);
      for (int PixelLocation = 0; PixelLocation < TotalLEDs; PixelLocation++) {
        pixels.setPixelColor(PixelLocation, pixels.Color(255, 0, 0));
        pixels.show();
      }
      pixels.setBrightness(LEDBrigthness);
      pixels.show();
    }
    DisplayTimer = 0;
  }
  delay(10);
}