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

const unsigned long PAGE_INTERVAL_MS = 3000;
const unsigned long SCROLL_FRAME_MS = 20;
const int SCREEN_WIDTH = 72;
const int SCROLL_STEP = 4;

unsigned long lastPageSwitchMillis = 0;
unsigned long lastScrollFrameMillis = 0;
bool showClockPage = true;
bool isScrolling = false;
bool fromClockPage = true;
bool toClockPage = false;
int scrollShift = 0;

void oledPrint(const char* text) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x13_tr);
  u8g2.setCursor(0, 20);
  u8g2.print(text);
  u8g2.sendBuffer();
}

void drawClockPage(int xOffset, int h, int m, int s) {
  char top[6], bot[3];
  sprintf(top, "%02d:%02d", h, m);
  sprintf(bot, "%02d", s);

  u8g2.setFont(u8g2_font_10x20_tr);
  u8g2.setCursor(xOffset + 11, 20);
  u8g2.print(top);
  u8g2.setCursor(xOffset + 26, 40);
  u8g2.print(bot);
}

void drawDatePage(int xOffset, const struct tm& t) {
  static const char* dayNames[] = {
    "MINGGU", "SENIN", "SELASA", "RABU", "KAMIS", "JUMAT", "SABTU"
  };
  static const char* monthNames[] = {
    "JAN", "FEB", "MAR", "APR", "MEI", "JUN",
    "JUL", "AGS", "SEP", "OKT", "NOV", "DES"
  };

  char line1[16], line2[16];
  sprintf(line1, "%s", dayNames[t.tm_wday]);
  sprintf(line2, "%02d %s %02d", t.tm_mday, monthNames[t.tm_mon], (t.tm_year + 1900) % 100);

  u8g2.setFont(u8g2_font_6x10_tr);

  int line1X = xOffset + (SCREEN_WIDTH - u8g2.getStrWidth(line1)) / 2;
  int line2X = xOffset + (SCREEN_WIDTH - u8g2.getStrWidth(line2)) / 2;

  u8g2.setCursor(line1X, 15);
  u8g2.print(line1);
  u8g2.setCursor(line2X, 34);
  u8g2.print(line2);
}

void drawPage(bool isClockPage, int xOffset, const struct tm& t) {
  if (isClockPage)
    drawClockPage(xOffset, t.tm_hour, t.tm_min, t.tm_sec);
  else
    drawDatePage(xOffset, t);
}

void drawFrame(const struct tm& t) {
  u8g2.clearBuffer();

  if (isScrolling) {
    drawPage(fromClockPage, -scrollShift, t);
    drawPage(toClockPage, SCREEN_WIDTH - scrollShift, t);
  } else {
    drawPage(showClockPage, 0, t);
  }

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
  unsigned long now = millis();
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    oledPrint("--:--:--");
    return;
  }

  if (!isScrolling && (now - lastPageSwitchMillis >= PAGE_INTERVAL_MS)) {
    isScrolling = true;
    fromClockPage = showClockPage;
    toClockPage = !showClockPage;
    scrollShift = 0;
    lastScrollFrameMillis = now;
  }

  if (isScrolling && (now - lastScrollFrameMillis >= SCROLL_FRAME_MS)) {
    lastScrollFrameMillis = now;
    scrollShift += SCROLL_STEP;

    if (scrollShift >= SCREEN_WIDTH) {
      scrollShift = SCREEN_WIDTH;
      isScrolling = false;
      showClockPage = toClockPage;
      lastPageSwitchMillis = now;
    }
  }

  drawFrame(timeinfo);
}

