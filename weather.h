#pragma once
#include <Arduino.h>

void updateWeather(String apiKey, float lat, float lon);

bool hasWeather();
float getTemperature();
float getTemperatureFeels();
String getWeatherIcon();