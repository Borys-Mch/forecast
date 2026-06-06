#include "weather.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

static bool weatherAvailable = false;
static float temperature = 0;
static float temperatureFeels = 0;

static unsigned long lastUpdate = 0;
static String lastPayload = "";
static String weatherIcon = "";

// кеш 5 хв
bool fetchWeatherJson(String &payload, String url)
{
  if (millis() - lastUpdate < 300000 && payload != "")
  {
    payload = lastPayload;
    return true;
  }

  HTTPClient http;
  http.begin(url);

  int code = http.GET();
  if (code != 200)
  {
    http.end();
    return false;
  }

  payload = http.getString();
  http.end();

  lastPayload = payload;
  lastUpdate = millis();

  return true;
}

void updateWeather(String apiKey, float lat, float lon)
{
  String url = "https://api.openweathermap.org/data/2.5/weather?lat=" +
               String(lat, 6) +
               "&lon=" + String(lon, 6) +
               "&appid=" + apiKey +
               "&units=metric";

  String payload;

  if (!fetchWeatherJson(payload, url))
  {
    weatherAvailable = false;
    return;
  }

  StaticJsonDocument<4096> doc;
  DeserializationError err = deserializeJson(doc, payload);

  if (err)
  {
    weatherAvailable = false;
    return;
  }

  temperature = doc["main"]["temp"];
  temperatureFeels = doc["main"]["feels_like"];
  weatherIcon = doc["weather"][0]["icon"].as<String>();
  weatherAvailable = true;

  Serial.println("=== WEATHER ===");
  Serial.println(payload);
}

bool hasWeather()
{
  return weatherAvailable;
}

float getTemperature()
{
  return temperature;
}

float getTemperatureFeels()
{
  return temperatureFeels;
}

String getWeatherIcon()
{
  return weatherIcon;
}