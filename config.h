#pragma once
#include <Arduino.h>

struct AppConfig
{
  String apiKey;
  float lat;
  float lon;
  String region;
};

void loadConfig(AppConfig &cfg);
void saveConfig(const AppConfig &cfg);