#include "weather.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

static bool weatherAvailable = false;
static float temperature = 0;
static float temperatureFeels = 0;
static float humidity = 0;
static float speed = 0;
static unsigned long lastUpdate = 0;
static String lastPayload = "";
static int weatherCode = 0;
static bool isDay = true;

// кеш 5 хв
bool fetchWeatherJson(String &payload, String url)
{
  // якщо вже є кеш і ще не пройшло 5 хв
  if (lastPayload != "" && millis() - lastUpdate < 300000)
  {
    payload = lastPayload;
    return true;
  }

  Serial.println("Weather HTTP request...");

  HTTPClient http;
  http.begin(url);

  int code = http.GET();

  if (code <= 0)
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
  String url = "http://api.weatherapi.com/v1/current.json?key=" +
               apiKey +
               "&q=" + String(lat, 6) + "," + String(lon, 6) +
               "&aqi=no";

  String payload;

  if (!fetchWeatherJson(payload, url))
  {
    weatherAvailable = false;
    return;
  }

  StaticJsonDocument<2048> doc;
  DeserializationError err = deserializeJson(doc, payload.c_str());

  if (err)
  {
    Serial.print("JSON ERROR: ");
    Serial.println(err.c_str());
    weatherAvailable = false;
    return;
  }

  temperature = doc["current"]["temp_c"];
  temperatureFeels = doc["current"]["feelslike_c"];
  humidity = doc["current"]["humidity"];
  speed = doc["current"]["wind_kph"];
  int code = doc["current"]["condition"]["code"];
  weatherCode = doc["current"]["condition"]["code"];
  isDay = doc["current"]["is_day"] == 1;

  weatherAvailable = true;

  Serial.println("=== WEATHER ===");
  Serial.println(payload);

  Serial.print("Weather JSON: ");
  Serial.println(doc.memoryUsage());

  Serial.print("Capacity: ");
  Serial.println(doc.capacity());

  Serial.print("Usage: ");
  Serial.println(doc.memoryUsage());

  Serial.print("Payload length: ");
  Serial.println(payload.length());
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

float getHumidity()
{
  return humidity;
}

float getWindSpeed()
{
  return speed / 3.6;
}

int getWeatherCode()
{
  return weatherCode;
}

bool getIsDay()
{
  return isDay;
}