#pragma once
#include <Arduino.h>

void updateWeather(String apiKey, float lat, float lon);

bool hasWeather();
float getTemperature();
float getTemperatureFeels();
float getHumidity();
float getWindSpeed();
int getWeatherCode();
bool getIsDay();

struct ForecastDay
{
  String date;
  float maxTemp;
  float minTemp;
  float avgvis_km;
  int avghumidity;

  String weatherText;
  int weatherCode;
};

extern ForecastDay forecast[3];