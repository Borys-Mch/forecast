#include "weather.h"
#include "config.h"

static bool weatherAvailable = false;
static float temperature = 0;
static float temperatureFeels = 0;
static float humidity = 0;
static float speed = 0;
static unsigned long lastUpdate = 0;
static String lastPayload = "";
static int weatherCode = 0;
static bool isDay = true;
ForecastDay forecast[3];
String formatDate(String date)
{
  // формат: YYYY-MM-DD
  if (date.length() >= 10)
  {
    String day = date.substring(8, 10);
    String month = date.substring(5, 7);
    return day + "." + month;
  }
  return date;
}

// кеш 5 хв
bool fetchWeatherJson(String &payload, String url)
{
  if (lastPayload != "" && millis() - lastUpdate < 300000)
  {
    payload = lastPayload;
    return true;
  }

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
  String url = "http://api.weatherapi.com/v1/forecast.json?key=" +
               apiKey +
               "&q=" + String(lat, 6) + "," + String(lon, 6) +
               "&days=3&lang=uk";

  String payload;

  if (!fetchWeatherJson(payload, url))
  {
    weatherAvailable = false;
    return;
  }

  StaticJsonDocument<6144> doc;
  DeserializationError err = deserializeJson(doc, payload.c_str());

  if (err)
  {
    weatherAvailable = false;
    return;
  }

  temperature = doc["current"]["temp_c"];
  temperatureFeels = doc["current"]["feelslike_c"];
  humidity = doc["current"]["humidity"];
  speed = doc["current"]["wind_kph"];
  weatherCode = doc["current"]["condition"]["code"];
  isDay = doc["current"]["is_day"] == 1;

  JsonArray days = doc["forecast"]["forecastday"];

  if (days.size() >= 3)
  {
    for (int i = 0; i < 3; i++)
    {
      forecast[i].date = formatDate(days[i]["date"].as<String>());
      forecast[i].maxTemp = days[i]["day"]["maxtemp_c"];
      forecast[i].minTemp = days[i]["day"]["mintemp_c"];
      forecast[i].avghumidity = days[i]["day"]["avghumidity"];
      forecast[i].avgvis_km = days[i]["day"]["avgvis_km"];
      forecast[i].weatherText = days[i]["day"]["condition"]["text"].as<String>();
      forecast[i].weatherCode = days[i]["day"]["condition"]["code"];
    }
  }

  weatherAvailable = true;
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