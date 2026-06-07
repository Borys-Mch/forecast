#pragma once
#include <Arduino.h>

void updateWeather(String apiKey, float lat, float lon);

bool hasWeather();
float getTemperature();
float getTemperatureFeels();
float getHumidity();
float getWindSpeed();
String getWeatherIcon();