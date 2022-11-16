#include <tinyNeoPixel.h>
#include "TinyI2CMaster.h"

//------------------Watch Variables-------------
//y = -267.48x + 3913.6
//Given "x" UV level, "y" returns seconds needed to burn, we measure time spent in "x" and calc a "% burned"
//(Time Spent at UV level)/(-267.48*(UV level)+3913.6) + ((Time Spent at UV level)/(-267.48*(UV level)+3913.6))*ginger index
//1 = ginger. Increase to burn faster, decrease to be a beautiful tan person
//default is 1
float GingerIndex = 1;
//Input SPF used
//default is 30
float SPFIndex = 30;
//How long you expect your sunscreen to last in seconds
//default is 3600
int SunScreenDurationSeconds = 3600;
//How fast you want to recover from sunburn
//Percent decay per second, as a decimal
//Default = 0.005
float BurnDecay = 0.005;
//Mode control, what we want to start on
//default is 1
int WatchModeSelect = 1;
//Sub-Mode control, what we want to start on
//default is 1
int WatchSubModeSelect = 1;
//LED Brightness
int LEDBrigthness = 5;
int adjustableLEDBrigthness = LEDBrigthness;

//------------------SunBurn Algo Setup-------------
//we assume the person isn't wearing sunscreen (BOOOOO!)
bool SunScreenApplied = false;
//Variables to help calculate
int SunScreenTTBTimer = SunScreenDurationSeconds;
float PercentBurned = 0;
float DisplayPercentBurned = 0;
float SecondsToBurn = 0;
float PercentAddedToBurn = 0;
//Average UV stuffs.
const int UVnumReadings = 10;
float UVreadings[UVnumReadings];
int UVreadIndex = 0;
float UVtotal = 0;
float UVaverage = 0;
int SecondsInSun = 0;
float CurrentReading = 0;
float PreviousReading = 0;

//------------------UV Sensor Setup-------------
// Adafruit says voltage/0.1V = UV index
// GUVA-S12SD sensor attached to this pin - voltage is 4.3 x diode photocurrent
#define UV_PIN PIN_PA2
float counts;
float UVSensorVoltage;
float voltageoffset = 0.33;

//------------------NeoPixels Setup----------------
// NeoPixel Pin
#define NEO_PIN PIN_PA3
//Count of NeoPixels
#define NUMPIXELS 12
//Setup Neopixels
tinyNeoPixel pixels = tinyNeoPixel(NUMPIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);
int RedLEDTimer = 0;
int GreenLEDTimer = 0;
int BlueLEDTimer = 0;
float TotalLEDs = NUMPIXELS;
int NeoPixelArray[NUMPIXELS][3];

//------------------Accel Setup-------------
//Initialize accel, setting up
extern int8_t accelInit();
//Run in accel.ino, not used here
extern void accelRead();
//changes global variable ZeroAtTwelve BOOL (int) 0-1
extern bool orientDisplay();
//Setup comms
#define I2C_WRITE 0
// Accel values
uint16_t aOrient;
//Check if accel is available
uint8_t gotAccel = 0;
uint16_t aShake;
uint16_t notMoving = 0;
uint8_t ZeroAtTwelve = 0;

//------------------Button Setup-------------
// Buttons - use "INPUT_PULLUP" - pins will be low when pressed
#define UPPER_BUTTON PIN_PA4
#define LOWER_BUTTON PIN_PA7
bool UpperButtonPressed = false;
bool LowerButtonPressed = false;
int UpperButtonPressedTimer = 0;
int LowerButtonPressedTimer = 0;

//------------------Voltage Setup-------------
#define BATT_READ PIN_PA1
float BatteryVoltage;

//------------------Vibrator Setup-------------
#define VIBE_LED PIN_PA5

void setup() {
  Serial.begin(9600);
  Serial.println();
  Serial.println();
  Serial.println("UV Watch");
  Serial.println();

  TinyI2C.init();

  pixels.begin();
  pixels.setBrightness(LEDBrigthness);

  gotAccel = accelInit();

  pinMode(UPPER_BUTTON, INPUT_PULLUP);
  pinMode(LOWER_BUTTON, INPUT_PULLUP);

  pinMode(VIBE_LED, OUTPUT);
  digitalWrite(VIBE_LED, 1);
  delay(500);
  digitalWrite(VIBE_LED, 0);
}

//MS to loop for delay.
//**Do not edit (dire consequences)**
int LoopDelay = 10;

int SensorCaptureTimer = 0;
int SensorCaptureTrigger = 10;

int DisplayTimer = 0;
int DisplayTimerTrigger = 10;

int InputTimer = 0;
int InputTimerTrigger = 1;

void loop() {
  SensorCaptureTimer++;
  DisplayTimer++;
  InputTimer++;

  //-----------------------------INPUT CAPTURE---------------------------------------
  if (InputTimer >= InputTimerTrigger) {
    /*
    orientDisplay();
    if (!ZeroAtTwelve) {
      LowerButtonPressedTimer++;
    } else {
      digitalWrite(VIBE_LED, 0);
      if (LowerButtonPressedTimer > 0 && LowerButtonPressedTimer < 1000) {
        digitalWrite(VIBE_LED, 1);
        WatchModeSelect++;
      }
      LowerButtonPressedTimer = 0;
    }
    */
    if (digitalRead(UPPER_BUTTON)) {
      UpperButtonPressedTimer++;
    } else {
      digitalWrite(VIBE_LED, 0);
      if (UpperButtonPressedTimer > 0 && UpperButtonPressedTimer < 1000) {
        digitalWrite(VIBE_LED, 1);
        WatchModeSelect++;
        WatchSubModeSelect = 1;
      }
      UpperButtonPressedTimer = 0;
    }

    if (digitalRead(LOWER_BUTTON)) {
      LowerButtonPressedTimer++;
    } else {
      digitalWrite(VIBE_LED, 0);
      if (LowerButtonPressedTimer > 0 && LowerButtonPressedTimer < 1000) {
        digitalWrite(VIBE_LED, 1);
        WatchSubModeSelect++;
      }
      LowerButtonPressedTimer = 0;
    }
    //----DO NOT REMOVE----
    InputTimer = 0;
    //----DO NOT REMOVE----
  }
  //----------------------------------------------------------------------------------



  //-----------------------------SENSOR CAPTURE---------------------------------------
  if (SensorCaptureTimer >= SensorCaptureTrigger) {
    counts = analogRead(UV_PIN);
    UVSensorVoltage = ((counts * 3300 * 5) / 1024) * voltageoffset;
    CurrentReading = ((UVSensorVoltage - 108) / 97);
    if (CurrentReading > 12) {
      CurrentReading = 12;
    }
    if (CurrentReading < 0.2) {
      CurrentReading = 0;
    }
    //Giving momentum to decreasing readings. Prevents shade bias. Slows down decay.
    if (PreviousReading > CurrentReading) {
      CurrentReading = PreviousReading - ((PreviousReading - CurrentReading) / 10);
    }

    UVtotal = UVtotal - UVreadings[UVreadIndex];
    UVreadings[UVreadIndex] = CurrentReading;
    UVtotal = UVtotal + UVreadings[UVreadIndex];
    UVreadIndex++;
    if (UVreadIndex >= UVnumReadings) {
      UVreadIndex = 0;
    }
    adjustableLEDBrigthness = (LEDBrigthness * (CurrentReading * 2)) + LEDBrigthness;
    pixels.setBrightness(adjustableLEDBrigthness);
    PreviousReading = CurrentReading;

    //BatteryVoltage = analogRead(BATT_READ) * (5.0 / 1023.0);
    BatteryVoltage = map(analogRead(BATT_READ), 0, 1023, 0, 11);
    //----DO NOT REMOVE----
    SensorCaptureTimer = 0;
    //----DO NOT REMOVE----
  }
  //--------------------------------------------------------------------



  //-------------------------------DISPLAY-------------------------------------
  if (DisplayTimer >= DisplayTimerTrigger) {
    if (SunScreenApplied == true) {
      SunScreenTTBTimer--;
      if (SunScreenTTBTimer == 0) {
        SunScreenApplied = false;
      }
    }
    UVaverage = UVtotal / UVnumReadings;
    if (UVaverage < 0.1) {
      //If we see no UV
      UVaverage = 0;
      if (PercentBurned > 0) {
        //Decay percent burned by .1%/Second when no UV
        PercentBurned = PercentBurned - BurnDecay;
      } else {
        //When back to zero, reset all. No burn.
        SecondsInSun = 0;
        pixels.clear();
      }
    } else {
      //If we see UV, start counting time in sun
      SecondsInSun++;
      //If sunscreen applied. Start counting down and dividing time to burn by SPFIndex
      if (SunScreenApplied == true) {
        //Math with sunscreen
        SecondsToBurn = (-267.48 * UVaverage + 3913.6) * SPFIndex;
      } else {
        //Math without sunscreen
        SecondsToBurn = -267.48 * UVaverage + 3913.6;
      }
      //Building the new "% burned".
      PercentAddedToBurn = (1 / SecondsToBurn) * 100;
      PercentBurned = PercentBurned + (PercentAddedToBurn * GingerIndex);
    }

    //Value catching. Dealing with float. Makes 0- = 0
    if (PercentBurned < 0) {
      PercentBurned = 0;
    }
    if (PercentBurned > 200) {
      PercentBurned = 200;
    }
    //Testing/monitoring
    Serial.print("Average UV - ");
    Serial.println(UVaverage);
    Serial.print("% burned - ");
    Serial.println(PercentBurned);
    Serial.print("Seconds in sun - ");
    Serial.println(SecondsInSun);
    if (CurrentReading != 0) {
      Serial.print("Time to Burn (min)- ");
      Serial.println(((100 / PercentBurned) * SecondsInSun) / 60);
    }
    Serial.print("Sunscreen applied? - ");
    Serial.println(SunScreenApplied ? "Applied" : "None");
    if (SunScreenApplied) {
      Serial.print("Sunscreen left - ");
      Serial.println(SunScreenTTBTimer);
    }
    Serial.println("----------------");
    switch (WatchModeSelect) {
      case 1:
        switch (WatchSubModeSelect) {
          case 1:
            //Percent burned display
            for (float PixelLocation = 0; PixelLocation < TotalLEDs; PixelLocation++) {
              //Check if we are over 100% burned
              if (PercentBurned < 100) {
                DisplayPercentBurned = PercentBurned;
              } else {
                //if we are, then subtract 100 to display correctly
                DisplayPercentBurned = PercentBurned - 100;
              }
              //If we've reached 2x burned, pray for God
              if (PercentBurned == 200) {
                NeoPixelArray[0][0] = 255;
                NeoPixelArray[3][0] = 255;
                NeoPixelArray[6][0] = 255;
                NeoPixelArray[9][0] = 255;
              }
              //only enter here if the current pixel we care about is active in the loop
              if (PixelLocation == int(TotalLEDs * (DisplayPercentBurned / 100)) && DisplayPercentBurned > 0) {
                if (PercentBurned < 100) {
                  //If we aren't over burned, show normally.
                  RedLEDTimer = 255 * (PixelLocation / TotalLEDs);
                  GreenLEDTimer = 255 - RedLEDTimer;
                  BlueLEDTimer = 0;
                } else {
                  //If we are over burned then show red
                  RedLEDTimer = 255;
                  GreenLEDTimer = 0;
                  BlueLEDTimer = 0;
                  NeoPixelArray[int(PixelLocation - 1)][0] = RedLEDTimer;
                  NeoPixelArray[int(PixelLocation - 1)][1] = GreenLEDTimer + 40;
                  NeoPixelArray[int(PixelLocation - 1)][2] = BlueLEDTimer;
                  NeoPixelArray[int(PixelLocation - 2)][0] = RedLEDTimer;
                  NeoPixelArray[int(PixelLocation - 2)][1] = GreenLEDTimer + 80;
                  NeoPixelArray[int(PixelLocation - 2)][2] = BlueLEDTimer;
                }
                NeoPixelArray[int(PixelLocation)][0] = RedLEDTimer;
                NeoPixelArray[int(PixelLocation)][1] = GreenLEDTimer;
                NeoPixelArray[int(PixelLocation)][2] = BlueLEDTimer;
                if (SunScreenApplied) {
                  for (int i = 0; i < (SunScreenTTBTimer / 900) + 1; i++) {
                    PixelLocation++;
                    if (PixelLocation < 12) {
                      NeoPixelArray[int(PixelLocation)][0] = 0;
                      NeoPixelArray[int(PixelLocation)][1] = 0;
                      NeoPixelArray[int(PixelLocation)][2] = 255;
                    } else {
                      NeoPixelArray[int(PixelLocation - 12)][0] = 0;
                      NeoPixelArray[int(PixelLocation - 12)][1] = 0;
                      NeoPixelArray[int(PixelLocation - 12)][2] = 255;
                    }
                  }
                }
              }
            }
            break;  //----DO NOT REMOVE----
          case 2:
            //UV level display
            for (float PixelLocation = 0; PixelLocation < TotalLEDs; PixelLocation++) {
              if (PixelLocation <= UVaverage && UVaverage != 0) {
                RedLEDTimer = 255 * (PixelLocation / TotalLEDs);
                GreenLEDTimer = 0;
                BlueLEDTimer = 255 - RedLEDTimer;
                NeoPixelArray[int(PixelLocation)][0] = RedLEDTimer;
                NeoPixelArray[int(PixelLocation)][1] = GreenLEDTimer;
                NeoPixelArray[int(PixelLocation)][2] = BlueLEDTimer;
              }
            }
            break;  //----DO NOT REMOVE----
          default:
            WatchSubModeSelect = 1;
            break;  //----DO NOT REMOVE----
        }
        break;  //----DO NOT REMOVE----
      case 2:
        switch (WatchSubModeSelect) {
          case 1:
            //Battery voltage
            for (float PixelLocation = 0; PixelLocation < TotalLEDs; PixelLocation++) {
              if (int(PixelLocation) <= int(BatteryVoltage)) {
                NeoPixelArray[int(PixelLocation)][0] = 255;
                NeoPixelArray[int(PixelLocation)][1] = 255;
                NeoPixelArray[int(PixelLocation)][2] = 255;
              }
            }
            break;  //----DO NOT REMOVE----
          case 2:
            //Red flashlight
            for (float PixelLocation = 0; PixelLocation < TotalLEDs; PixelLocation++) {
              NeoPixelArray[int(PixelLocation)][0] = 255;
              NeoPixelArray[int(PixelLocation)][1] = 0;
              NeoPixelArray[int(PixelLocation)][2] = 0;
            }
            pixels.setBrightness(255);
            break;  //----DO NOT REMOVE----
          case 3:
            //White-ish flashlight
            for (float PixelLocation = 0; PixelLocation < TotalLEDs; PixelLocation++) {
              NeoPixelArray[int(PixelLocation)][0] = 175;
              NeoPixelArray[int(PixelLocation)][1] = 255;
              NeoPixelArray[int(PixelLocation)][2] = 255;
            }
            pixels.setBrightness(255);
            break;  //----DO NOT REMOVE----
          case 4:
            //Party time, excellent
            RedLEDTimer = random(0, 200);
            GreenLEDTimer = random(0, 200);
            BlueLEDTimer = random(0, 200);
            for (float PixelLocation = 0; PixelLocation < TotalLEDs; PixelLocation++) {
              NeoPixelArray[int(PixelLocation)][0] = RedLEDTimer;
              NeoPixelArray[int(PixelLocation)][1] = GreenLEDTimer;
              NeoPixelArray[int(PixelLocation)][2] = BlueLEDTimer;
            }
            pixels.setBrightness(random(0, 255));
            break;  //----DO NOT REMOVE----
          default:
            WatchSubModeSelect = 1;
            break;  //----DO NOT REMOVE----
        }
        break;  //----DO NOT REMOVE----
      default:
        WatchModeSelect = 1;
        break;  //----DO NOT REMOVE----
    }
    //Loop through all color selections and display
    for (int PixelLocation = 0; PixelLocation < TotalLEDs; PixelLocation++) {
      pixels.setPixelColor(PixelLocation, pixels.Color(NeoPixelArray[PixelLocation][0], NeoPixelArray[PixelLocation][1], NeoPixelArray[PixelLocation][2]));
      NeoPixelArray[PixelLocation][0] = 0;
      NeoPixelArray[PixelLocation][1] = 0;
      NeoPixelArray[PixelLocation][2] = 0;
      delay(1);
    }
    pixels.show();
    //----DO NOT REMOVE----
    DisplayTimer = 0;
    //----DO NOT REMOVE----
  }
  //--------------------------------------------------------------------

  delay(LoopDelay);
}