#include <SPI.h>
#include "src/EPD_3in52.h"
#include "src/imagedata.h"
#include "src/epdpaint.h"

#include <Wire.h>
#include "RTClib.h"

#include <WiFi.h>

#include <NTPClient.h>
#include <WiFiUdp.h>

#include <Preferences.h>
#include <BH1750.h>

//powersave wifi off
#include <esp_wifi.h>

Preferences pref;  //preference library object

RTC_DS3231 rtc;           //ds3231 object
BH1750 lightMeter(0x23);  //Initalize light sensor
int nightFlag = 0;        //remembers last state of clocl i.e. sleeping or not

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
int TIME_TO_SLEEP = 60;        //in seconds

#define BATPIN A0                  //battery voltage divider connection pin (1M Ohm with 104 Capacitor)
#define BATTERY_LEVEL_SAMPLING 32  //number of times to average the reading

const char *ssid = "SonyBraviaX400";
const char *password = "79756622761";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", 19800);  //19800 is offset of India, asia.pool.ntp.org is close to India
char daysOfTheWeek[7][10] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

#define COLORED 0
#define UNCOLORED 1

UBYTE image[68000];
Epd epd;

bool errFlag = false;
//takes samples based on BATTERY_LEVEL_SAMPLING, averages them and returns actual battery voltage
float batteryLevel() {
  uint32_t Vbatt = 0;
  for (int i = 0; i < BATTERY_LEVEL_SAMPLING; i++) {
    Vbatt = Vbatt + analogReadMilliVolts(BATPIN);  // ADC with correction
  }
  float Vbattf = 2 * Vbatt / BATTERY_LEVEL_SAMPLING / 1000.0;  // attenuation ratio 1/2, mV --> V
  //Serial.println(Vbattf);
  return Vbattf;
}

void enableWiFi() {
  //adc_power_on();
  WiFi.disconnect(false);  // Reconnect the network
  WiFi.mode(WIFI_STA);     // Switch WiFi off

  Serial1.println("START WIFI");
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed");
    break;
  }
  Serial1.println("IP address: ");
  Serial1.println(WiFi.localIP());
}

void disableWiFi() {
  //adc_power_off();
  WiFi.disconnect(true);  // Disconnect from the network
  WiFi.mode(WIFI_OFF);    // Switch WiFi off
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(BATPIN, INPUT);
  Wire.begin();
  Wire.setClock(400000);  // Set clock speed to be the fastest for better communication (fast mode)
  disableWiFi();

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    //showMsg("RTC Error");
    errFlag = true;
  }

  if (lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE)) {
    Serial.println(F("BH1750 Advanced begin"));
  } else {
    Serial.println(F("Error initialising BH1750"));
    errFlag = true;
  }
  float lux = 0;
  while (!lightMeter.measurementReady(true)) {
    yield();
  }
  lux = lightMeter.readLightLevel();
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");

  pref.begin("database", false);

  bool checkFlag = pref.isKey("nightFlag");
  if (!checkFlag) {  //create key:value pair
    pref.putBool("nightFlag", "");
  }
  nightFlag = pref.getBool("nightFlag", false);
  bool tempNightFlag = nightFlag, timeNeedsUpdate = false;

  if (lux == 0) {
    if (nightFlag == 0) {  //prevents unnecessary redrawing of same thing
      nightFlag = 1;
      if (epd.Init() != 0) {
        Serial.println("e-Paper init failed");
        return;
      }
      epd.Clear();
      if (errFlag)
        showMsg("Error");
      else
        showMsg("SLEEPING");
    }
  } else {
    nightFlag = 0;
    pref.begin("database", false);

    float battLevel;
    battLevel = pref.getFloat("battLevel", -1.0);
    if (battLevel == -1.0) {
      Serial.println("No value saved for battLevel");
      pref.putFloat("battLevel", 3.5);
    }

    float tempBattLevel = battLevel;

    DateTime now = rtc.now();
    Serial.println(now.month(), DEC);
    Serial.println(now.day(), DEC);
    Serial.println(now.hour(), DEC);
    Serial.println(now.minute(), DEC);
    Serial.println(now.second(), DEC);
    Serial.println(now.year(), DEC);

    bool timeUpdateNeeded;
    timeUpdateNeeded = pref.getBool("timeNeedsUpdate", false);

    if ((now.year() == 2070) || rtc.lostPower() || timeUpdateNeeded)
      timeNeedsUpdate = true;

    if (timeNeedsUpdate || (now.hour() == 18 && (now.minute() == 0 || now.minute() == 1))) {  //updates time in RTC everyday taking from NTP
      enableWiFi();
      delay(50);
      if (WiFi.status() == WL_CONNECTED) {
        timeClient.begin();
        if (timeClient.update()) {
          timeNeedsUpdate = false;
          time_t rawtime = timeClient.getEpochTime();
          struct tm *ti;
          ti = localtime(&rawtime);

          uint16_t year = ti->tm_year + 1900;
          uint8_t x = year % 10;
          year = year / 10;
          uint8_t y = year % 10;
          year = y * 10 + x;

          uint8_t month = ti->tm_mon + 1;

          uint8_t day = ti->tm_mday;
          rtc.adjust(DateTime(year, month, day, timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds()));
        }
        disableWiFi();
      }
    }

    //Battery level
    float newBattLevel = batteryLevel();
    if (newBattLevel < battLevel)  //to maintain steady decrease in battery level
      battLevel = newBattLevel;

    if (((newBattLevel - battLevel) >= 0.2) || newBattLevel > 3.5)  //to update the battery level in case of charging
      battLevel = newBattLevel;

    byte percent = ((battLevel - 2.7) / 0.7) * 100;  //range is 3.4v-100% and 2.7v-0%
    if (percent < 1)
      percent = 1;
    else if (percent > 100)
      percent = 100;

    if (tempBattLevel != battLevel)
      pref.putFloat("battLevel", battLevel);

    String percentStr;
    if (battLevel > 4.11)
      percentStr = "USB";
    else
      percentStr = String(percent) + "%";

    //battery level end

    bool timeFlag;
    byte tempHour;
    if (now.hour() > 12) {  //24 hours to 12 hour conversion
      tempHour = now.hour() - 12;
      timeFlag = true;
    } else if (now.hour() == 12) {
      tempHour = now.hour();
      timeFlag = true;
    } else if (now.hour() == 0) {
      tempHour = 12;
      timeFlag = false;
    } else {
      tempHour = now.hour();
      timeFlag = false;
    }

    String dateString = (now.day() < 10 ? "0" + String(now.day()) : String(now.day())) + "/" + (now.month() < 10 ? "0" + String(now.month()) : String(now.month())) + "/" + String(now.year());
    String timeString = (tempHour < 10 ? "0" + String(tempHour) : String(tempHour)) + ":" + (now.minute() < 10 ? "0" + String(now.minute()) : String(now.minute())) + (timeFlag == true ? " PM" : " AM");
    if (epd.Init() != 0) {
      Serial.println("e-Paper init failed");
      return;
    }
    epd.Clear();
    Serial.println("e-Paper Clear");
    if (timeNeedsUpdate)
      showMsg("Time Resync Await");
    else if (errFlag)
      showMsg("Error");
    else
      showTime(daysOfTheWeek[now.dayOfTheWeek()], timeString, dateString, String(battLevel) + "V", percentStr, percent);
  }
  if (tempNightFlag != nightFlag)
    pref.putBool("nightFlag", nightFlag);

  pref.putBool("timeNeedsUpdate", timeNeedsUpdate);

  //esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP / 60) + " Mins");
  //Go to sleep now
  Serial.println("Going to sleep now");
  Serial.flush();
  delay(50);
  //esp_deep_sleep_start();
}

void loop() {
  // This will never run
}


//various error msg display function
void showMsg(String msg) {
  epd.display_NUM(EPD_3IN52_WHITE);
  epd.lut_GC();
  epd.refresh();
  epd.SendCommand(0x50);
  epd.SendData(0x17);
  delay(100);

  Paint paint(image, 240, 360);  // width should be the multiple of 8
  paint.SetRotate(3);            // Top right (0,0)
  paint.Clear(COLORED);
  paint.DrawStringAt(10, 100, msg.c_str(), &Font48, UNCOLORED);

  epd.display_part(paint.GetImage(), 0, 0, paint.GetWidth(), paint.GetHeight());
  epd.lut_GC();
  epd.refresh();
  Serial.println("sleep......");
  delay(100);
  epd.sleep();
  Serial.println("end");
}

//Displays time, battery info. First para is week in const char, then time in hh:mm am/pm, then date in dd/mm/yyyy, then battlevel in X.YZV, percent in XY%
void showTime(char *w, String timeString, String dateString, String battLevel, String percentStr, byte percent) {
  epd.display_NUM(EPD_3IN52_WHITE);
  epd.lut_GC();
  epd.refresh();
  epd.SendCommand(0x50);
  epd.SendData(0x17);
  delay(100);

  Paint paint(image, 240, 360);  // width should be the multiple of 8
  paint.SetRotate(3);            // Top right (0,0)
  paint.Clear(UNCOLORED);

  //Battery icon
  paint.DrawRectangle(10, 4, 26, 12, COLORED);
  paint.DrawRectangle(8, 6, 10, 10, COLORED);
  
  if (percent >= 95)  //Full
    paint.DrawFilledRectangle(11, 4, 25, 11, COLORED);
  else if (percent >= 85 && percent < 95)  //ful-Med
    paint.DrawFilledRectangle(13, 4, 25, 11, COLORED);
  else if (percent >= 70 && percent < 85)  //Med
    paint.DrawFilledRectangle(15, 4, 25, 11, COLORED);
    else if (percent > 50 && percent < 70)  //Med-half
    paint.DrawFilledRectangle(17, 4, 25, 11, COLORED);
  else if (percent > 30 && percent <= 50)  //half
    paint.DrawFilledRectangle(19, 4, 25, 11, COLORED);
    else if (percent > 10 && percent <= 30)  //low-half
    paint.DrawFilledRectangle(21, 4, 25, 11, COLORED);
  else if (percent > 5 && percent <= 10)  //low
    paint.DrawFilledRectangle(23, 4, 25, 11, COLORED);
  else if (percent > 2 && percent <= 5)  //critical-low
    paint.DrawFilledRectangle(25, 4, 25, 11, COLORED);   
  //END Battery ICON

  paint.DrawStringAt(325, 5, percentStr.c_str(), &Font12, COLORED);
  paint.DrawStringAt(280, 5, battLevel.c_str(), &Font12, COLORED);
  paint.DrawStringAt(60, 30, w, &Font48, COLORED);
  paint.DrawStringAt(60, 100, timeString.c_str(), &Font48, COLORED);
  paint.DrawStringAt(60, 170, dateString.c_str(), &Font48, COLORED);

  epd.display_part(paint.GetImage(), 0, 0, paint.GetWidth(), paint.GetHeight());
  epd.lut_GC();
  epd.refresh();
  Serial.println("sleep......");
  delay(100);
  epd.sleep();
  Serial.println("end");
}
