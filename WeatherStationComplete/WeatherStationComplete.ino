#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "RTClib.h"
#include <LiquidCrystal.h>

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

#define SEALEVELPRESSURE_HPA (1013.25)

RTC_DS1307 rtc;   // initialise the RTC DS1307
Adafruit_BME280 bme;  // initialise the BME280 sensor


unsigned long delayTime;  // refresh rate for the readings
int t_hour = 0;
int t_minute = 0;


int pressureArray[10] = {0};  // here we store the pressure readings 
byte counter = 0;
byte delta_time = 0;
int Z = 0;

char tStr[21];
char pStr[22];
char hStr[20];
char pseaStr[26];
char timeStr[6];
char dateStr[12];
char zambretti[10] = "N/A";
char pressureHistory[57];
char pressureHistory2[57];



int temperature;
int humidity;
int pressure;
int altitude = 5;   // Place real altitude of weather station


void setup() {
  
  lcd.begin(16, 2);

  bool status;
  Serial.begin(57600);   // default settings
  Serial.println("Starting measurements");
  if (! rtc.begin()) {
    lcd.print("Couldn't find RTC");
    while (1);
  }

if (!bme.begin(0x76)) {
    lcd.print("BME Failuire!");
    while (1);
}

rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

delayTime = 20000;
  }

  
    

  
 
  

void loop() {

bool sunny = false;
  bool sunnyCloudy = false;
  bool cloudy = false;
  bool rainy = false;
  bool worsening = false;

  //String trend = "Steady";  // redundant

  // Reading sensor values
  temperature = (int)bme.readTemperature();
  humidity = (int)bme.readHumidity();
  pressure = (int)(bme.readPressure() / 100.0F);
  
  // Calculate sea-level pressure
  int seapressure = station2sealevel(pressure, altitude, temperature);

  DateTime now = rtc.now();
  int t_hour2 = now.hour();
  int t_minute2 = now.minute();

  // Update delta_time and pressure array every minute
  if (t_hour2 != t_hour || t_minute2 != t_minute) {
    delta_time++;
    if (delta_time > 10) {    // every minute we increment delta_time, then every 10 minutes
      delta_time = 0;         // we store the value in the array 

      if (counter == 10) {  // if array is full, shift the array and add the latest value
        for (int i = 0; i < 9; i++) {
          pressureArray[i] = pressureArray[i + 1];
        }
        pressureArray[counter - 1] = seapressure;
      } else {  // populate the array if it's not full
        pressureArray[counter] = seapressure;
        counter++;
      }
    }

    // Calculate averages for trend analysis-redundant
    //int avg_recent = (pressureArray[9] + pressureArray[8] + pressureArray[7]) / 3;
    //int avg_earlier = (pressureArray[0] + pressureArray[1] + pressureArray[2]) / 3;

    // Zambretti forecast based on pressure trends
    Z = calc_zambretti((pressureArray[9] + pressureArray[8] + pressureArray[7]) / 3, (pressureArray[0] + pressureArray[1] + pressureArray[2]) / 3, now.month());

    // Determine trend
    
    if (pressureArray[9] > 0 and pressureArray[0] > 0) {
      if (pressureArray[9] + pressureArray[8] + pressureArray[7] - pressureArray[0] - pressureArray[1] - pressureArray[2] >= 3) {
        //RAISING
        //trend = ("Raising");
        if (Z < 3) {
          Serial.println("Sunny");
          sunny = true;
        }
        else if (Z >= 3 and Z <= 9) {
          Serial.println("Sunny Cloudy");
          sunnyCloudy = true;
        }
        else if (Z > 9 and Z <= 17) {
          Serial.println("Cloudy");
          cloudy = true;
        }
        else if (Z > 17) {
          Serial.println("Rainy");
          rainy = true;
        }
      }

      else if (pressureArray[0] + pressureArray[1] + pressureArray[2] - pressureArray[9] - pressureArray[8] - pressureArray[7] >= 3) {
        //FALLING
        //trend = ("Falling");
        if (Z < 4) {
          Serial.println("Sunny");
          sunny = true;
        }
        else if (Z >= 4 and Z < 14) {
          Serial.println("Sunny Cloudy");
          sunnyCloudy = true;
        }
        else if (Z >= 14 and Z < 19) {
          Serial.println("Worsening");
          worsening = true;
        }
        else if (Z >= 19 and Z < 21) {
          Serial.println("Cloudy");
          cloudy = true;
        }
        else if (Z >= 21) {
          Serial.println("Rainy");
          rainy = true;
        }
      }
      else {
        //STEADY
        //trend = ("Steady");
        if (Z < 5) {
          Serial.println("Sunny");
          sunny = true;
        }
        else if (Z >= 5 and Z <= 11) {
          Serial.println("Sunny Cloudy");
          sunnyCloudy = true;
        }
        else if (Z > 11 and Z < 14) {
          Serial.println("Cloudy");
          cloudy = true;
        }
        else if (Z >= 14 and Z < 19) {
          Serial.println("Worsening");
          worsening = true;
        }
        else if (Z >= 19) {
          Serial.println("Rainy");
          rainy = true;
        }
      }
    }
    else {
      if (seapressure < 1005) {
        Serial.println("Rainy");
        rainy = true;
      }
      else if (seapressure >= 1005 and seapressure <= 1015) {
        Serial.println("Cloudy");
        cloudy = true;
      }
      else if (seapressure > 1015 and seapressure < 1025) {
        Serial.println("Sunny Cloudy");
        sunnyCloudy = true;
      }
      else {
        Serial.println("Rainy");
        rainy = true;
      }
    }

    // Save current time for next loop iteration
    t_hour = t_hour2;
    t_minute = t_minute2;
  }

  // Display forecast, temperature, and pressure constantly
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(String(temperature) + "C      ");

  // Display pressure and trend


  // Display the trend: Raising, Falling, or Steady, arguably no point givin that the other values give 10 minute forcasts
  //lcd.setCursor(0, 2);
  //lcd.print("Trend: ");
  //lcd.print(trend);

  // Forecast display logic based on Zambretti result
  lcd.setCursor(0, 1);
  if (rainy == true) {
    lcd.print("Rainy           ");
  } else if (cloudy == true) {
    lcd.print("Cloudy          ");
  } else if (sunnyCloudy == true) {
    lcd.print("Sunny Cloudy    ");
  } else if (worsening == true) {
    lcd.print("Worsening       ");
  } else if (sunny == true) {
    lcd.print("Sunny           ");
  }

  delay(delayTime);  // Keep delay to allow refreshing

  
  
}


// this is the core code 
int calc_zambretti(int curr_pressure, int prev_pressure, int mon) {
    if (curr_pressure < prev_pressure) {
        // FALLING
        if (mon == 12 || mon == 1 || mon == 2) {
            // summer (Southern Hemisphere)
            if (curr_pressure >= 1030) return 2;
            else if (curr_pressure >= 1020 && curr_pressure < 1030) return 8;
            else if (curr_pressure >= 1010 && curr_pressure < 1020) return 18;
            else if (curr_pressure >= 1000 && curr_pressure < 1010) return 21;
            else if (curr_pressure >= 990 && curr_pressure < 1000) return 24;
            else if (curr_pressure >= 980 && curr_pressure < 990) return 24;
            else if (curr_pressure >= 970 && curr_pressure < 980) return 26;
            else if (curr_pressure < 970) return 26;
        } else {
            // winter (Southern Hemisphere)
            if (curr_pressure >= 1030) return 2;
            else if (curr_pressure >= 1020 && curr_pressure < 1030) return 8;
            else if (curr_pressure >= 1010 && curr_pressure < 1020) return 15;
            else if (curr_pressure >= 1000 && curr_pressure < 1010) return 21;
            else if (curr_pressure >= 990 && curr_pressure < 1000) return 22;
            else if (curr_pressure >= 980 && curr_pressure < 990) return 24;
            else if (curr_pressure >= 970 && curr_pressure < 980) return 26;
            else if (curr_pressure < 970) return 26;
        }
    } else if (curr_pressure > prev_pressure) {
        // RAISING
        if (mon == 12 || mon == 1 || mon == 2) {
            // summer (Southern Hemisphere)
            if (curr_pressure >= 1030) return 1;
            else if (curr_pressure >= 1020 && curr_pressure < 1030) return 2;
            else if (curr_pressure >= 1010 && curr_pressure < 1020) return 3;
            else if (curr_pressure >= 1000 && curr_pressure < 1010) return 7;
            else if (curr_pressure >= 990 && curr_pressure < 1000) return 9;
            else if (curr_pressure >= 980 && curr_pressure < 990) return 12;
            else if (curr_pressure >= 970 && curr_pressure < 980) return 17;
            else if (curr_pressure < 970) return 17;
        } else {
            // winter (Southern Hemisphere)
            if (curr_pressure >= 1030) return 1;
            else if (curr_pressure >= 1020 && curr_pressure < 1030) return 2;
            else if (curr_pressure >= 1010 && curr_pressure < 1020) return 6;
            else if (curr_pressure >= 1000 && curr_pressure < 1010) return 7;
            else if (curr_pressure >= 990 && curr_pressure < 1000) return 10;
            else if (curr_pressure >= 980 && curr_pressure < 990) return 13;
            else if (curr_pressure >= 970 && curr_pressure < 980) return 17;
            else if (curr_pressure < 970) return 17;
        }
    } else {
        if (curr_pressure >= 1030) return 1;
        else if (curr_pressure >= 1020 && curr_pressure < 1030) return 2;
        else if (curr_pressure >= 1010 && curr_pressure < 1020) return 11;
        else if (curr_pressure >= 1000 && curr_pressure < 1010) return 14;
        else if (curr_pressure >= 990 && curr_pressure < 1000) return 19;
        else if (curr_pressure >= 980 && curr_pressure < 990) return 23;
        else if (curr_pressure >= 970 && curr_pressure < 980) return 24;
        else if (curr_pressure < 970) return 26;
    }
}


int station2sealevel(int p, int height, int t) {  // from pressure at our height to sea level
  return (double) p * pow(1 - 0.0065 * (double)height / (t + 0.0065 * (double)height + 273.15), -5.275);
}

  
