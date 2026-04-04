#include <Arduino.h>
#include "config.h"
#include <U8g2lib.h>
#include <Wire.h>
#include <WiFi.h>
#include <time.h>

#define SDA_PIN 5
#define SCL_PIN 6

const char* ssid      = WIFI_SSID;
const char* password  = WIFI_PASSWORD;
const char* ntpServer = "ntp.ui.ac.id";

U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
unsigned long previousMillis = 0;

void oledPrint(const char* text) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.setCursor(0, 20);
  u8g2.print(text);
  u8g2.sendBuffer();
}

void drawClock(int h, int m, int s) {
  char top[6], bot[3];
  sprintf(top, "%02d:%02d", h, m);
  sprintf(bot, "%02d", s);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_10x20_tr);
  u8g2.setCursor(11, 20);   // center "HH:MM" (5*10=50px) on 72px width
  u8g2.print(top);
  u8g2.setCursor(26, 40);   // center "SS" (2*10=20px) on 72px width
  u8g2.print(bot);
  u8g2.sendBuffer();
}

void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  u8g2.begin();

  oledPrint("WiFi...");
  WiFi.begin(ssid, password);
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 15000)
    delay(500);

  if (WiFi.status() != WL_CONNECTED) {
    oledPrint("No WiFi");
    delay(2000);
    return;
  }

  oledPrint("NTP sync...");
  configTime(7 * 3600, 0, ntpServer);

  struct tm timeinfo;
  t = millis();
  while (!getLocalTime(&timeinfo) && millis() - t < 10000)
    delay(500);
}

void loop() {
  if (millis() - previousMillis >= 1000) {
    previousMillis = millis();

    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
      drawClock(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    else
      oledPrint("--:--:--");
  }
}

