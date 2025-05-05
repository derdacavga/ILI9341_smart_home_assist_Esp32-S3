/*
freeFont links :  https://github.com/Bodmer/TFT_eSPI/blob/master/examples/320%20x%20240/Free_Font_Demo/Free_Fonts.h

*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include "time.h"
#include <SPIFFS.h>
#include <FS.h>

const char* ssid = "YOUR_SSID";            // Change your SSID
const char* password = "SSID_PASSWORD";    // Change your SSID Password

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 1 * 3600;       // Chage your country Gmt
const int daylightOffset_sec = 0;

#define RELAY_LIGHT 5
#define RELAY_LOCK 6

const char* weatherURL = "https://api.open-meteo.com/v1/forecast?latitude=41.0082&longitude=28.9784&current_weather=true";

TFT_eSPI tft = TFT_eSPI();
TFT_eSPI_Button btnLight, btnLock;
bool lightState = false;
bool lockState = false;

void drawMainScreen();
void updateWeather();
void updateClock();
void updateStatus();
void handleTouch();
void touchCalibration();
void logo();

void setup() {
  Serial.begin(115200);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS not start!");
  }

  tft.init();
  tft.setRotation(2); // Change display rotation
  logo();
  delay(2000);
  tft.setTextFont(1); // Change Font
  tft.setTextSize(0); // Change Font Size
  tft.fillScreen(TFT_GREEN);

  if (!SPIFFS.exists("/calibrationData")) {
    touchCalibration();
  } else {
    fs::File f = SPIFFS.open("/calibrationData", "r");

    uint16_t data[5];
    f.read((uint8_t*)data, 10);
    f.close();
    tft.setTouch(data);
  }

  WiFi.begin(ssid, password);
  tft.drawString("Wifi Connecting", 120, 10);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  tft.drawString("WiFi connected!", 120, 30);
  delay(1000);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  pinMode(RELAY_LIGHT, OUTPUT);
  pinMode(RELAY_LOCK, OUTPUT);
  digitalWrite(RELAY_LIGHT, LOW);
  digitalWrite(RELAY_LOCK, LOW);

  drawMainScreen();
}

void loop() {
  static unsigned long lastUpdate = 0;
  static unsigned long lastWeather = 0;

  handleTouch();

  if (millis() - lastUpdate > 1000) {
    lastUpdate = millis();
    updateClock();
  }

  if (millis() - lastWeather > 60000) {
    lastWeather = millis();
    updateWeather();
  }
}

void drawMainScreen() {
  tft.fillScreen(TFT_GREEN);
  tft.setTextColor(TFT_BLACK, TFT_GREEN);
  tft.drawString("Smart Home Assist", 110, 5);

  btnLight.initButton(&tft, 120, 100, 180, 40, TFT_BLACK, TFT_BLUE, TFT_BLACK, "LIGHT", 1);
  btnLock.initButton(&tft, 120, 160, 180, 40, TFT_BLACK, TFT_RED, TFT_BLACK, "DOOR", 1);
  btnLight.drawButton();
  btnLock.drawButton();

  updateStatus();
  updateWeather();
  updateClock();
}

void updateStatus() {
  tft.setTextColor(TFT_BLACK, TFT_GREEN);
  tft.fillRect(0, 210, 240, 30, TFT_GREEN);
  tft.setCursor(10, 210);
  tft.print("Light: ");
  tft.print(lightState ? "Open" : "Close");

  tft.setCursor(130, 210);
  tft.print("Door: ");
  tft.print(lockState ? "Locked" : "Unlock");
}

void updateWeather() {
  if ((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;
    http.begin(weatherURL);
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      DynamicJsonDocument doc(4096);
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        float temp = doc["current_weather"]["temperature"];
        float wind = doc["current_weather"]["windspeed"];

        tft.setTextColor(TFT_BLACK, TFT_GREEN);
        tft.fillRect(0, 60, 240, 20, TFT_GREEN);
        tft.setCursor(10, 65);
        tft.printf("Temp: %.1fC  Wind: %.1f", temp, wind);
      } else {
        Serial.println("JSON debug error");
      }
    }
    http.end();
  }
}

void updateClock() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Not get Time");
    return;
  }

  char timeStr[16];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  char dateStr[16];
  strftime(dateStr, sizeof(dateStr), "%d/%m/%Y", &timeinfo);

  tft.setTextColor(TFT_BLACK, TFT_GREEN);
  tft.fillRect(0, 30, 240, 20, TFT_GREEN);
  tft.setCursor(10, 35);
  tft.printf("Time: %s  %s", timeStr, dateStr);
}

void handleTouch() {
  uint16_t x, y;
  if (tft.getTouch(&x, &y)) {
    if (btnLight.contains(x, y)) {
      lightState = !lightState;
      digitalWrite(RELAY_LIGHT, lightState ? HIGH : LOW);
      updateStatus();
      delay(300);
    }
    if (btnLock.contains(x, y)) {
      lockState = !lockState;
      digitalWrite(RELAY_LOCK, lockState ? HIGH : LOW);
      updateStatus();
      delay(300);
    }
  }
}

void touchCalibration() {
  uint16_t calData[5];
  tft.fillScreen(TFT_GREEN);
  tft.setTextColor(TFT_BLACK, TFT_GREEN);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Touch that corners", tft.width() / 2, tft.height() / 2);
  delay(2000);
  tft.calibrateTouch(calData, TFT_BLACK, TFT_GREEN, 15);
  fs::File f = SPIFFS.open("/calibrationData", "w");
  f.write((const uint8_t*)calData, 10);
  f.close();
}

void logo() {
  tft.fillScreen(TFT_RED);
  int centerX = 120;
  int centerY = 160;
  int radius = 120;
  int thickness = 10;

  for (int r = radius - thickness / 2; r <= radius + thickness / 2; r++) {
    tft.drawCircle(centerX, centerY, r, TFT_BLACK);
  }

  tft.setTextColor(TFT_BLACK, TFT_RED);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  tft.setFreeFont(&FreeSansBold24pt7b);
  tft.drawString("DSN", centerX, centerY);
}