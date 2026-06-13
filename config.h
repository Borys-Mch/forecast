#pragma once
#include <Arduino.h>

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
};

void loadConfig(AppConfig &cfg);
void saveConfig(const AppConfig &cfg);