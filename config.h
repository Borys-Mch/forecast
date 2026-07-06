#pragma once
#include <Update.h>
#include <Arduino.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7796S.h>
#include <SensirionI2cSps30.h>
#include <Adafruit_NeoPixel.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <SparkFun_SCD30_Arduino_Library.h>
#include <SPI.h>
#include <WiFi.h>
#include <math.h>
#include <Wire.h>

// ===== ПІНИ =================================================
#define TFT_CS 1
#define TFT_DC 3
#define TFT_RST 2
#define TFT_MISO -1
#define TFT_MOSI 6
#define TFT_SCK 7
#define TFT_BL 0
#define BTN_PIN 9

#define FW_VERSION "1.0.5"

struct AppConfig
{
  String apiKey;
  float lat;
  float lon;
  String region;
  float tempOffset = 0;
  float humOffset = 0;
  String mqttHost;
  int mqttPort;
  String mqttUser;
  String mqttPass;
  int brightness = 255;
};

void loadConfig(AppConfig &cfg);
void saveConfig(const AppConfig &cfg);