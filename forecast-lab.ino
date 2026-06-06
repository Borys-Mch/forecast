#include <WiFi.h>
#include "secrets.h"
#include "config.h"
#include "web.h"
#include "alerts.h"
#include "display.h"
#include "weather.h"

#include <Adafruit_NeoPixel.h>

bool hasData = false;

AppConfig config;
Adafruit_NeoPixel led(1, 8, NEO_GRB + NEO_KHZ800);

unsigned long lastAlertUpdate = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long lastBlink = 0;
bool ledState = false;

void setup()
{
  Serial.begin(115200);
  led.begin();

  loadConfig(config);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi OK");

  initDisplay();

  setupWeb(config);
}

void loop()
{
  handleWeb();

  // тривога
  if (millis() - lastAlertUpdate > 10000)
  {
    lastAlertUpdate = millis();
    updateAlert(config.region);
    hasData = true;
  }

  if (getAlertState())
  {
    if (millis() - lastBlink > 500) // кожні 500мс
    {
      lastBlink = millis();
      ledState = !ledState;

      if (ledState)
        led.setPixelColor(0, led.Color(255, 0, 0));
      else
        led.setPixelColor(0, 0);

      led.show();
    }
  }
  else
  {
    led.setPixelColor(0, 0);
    led.show();
  }

  // погода
  if (millis() - lastWeatherUpdate > 100000)
  {
    lastWeatherUpdate = millis();
    updateWeather(config.apiKey, config.lat, config.lon);
  }

  updateDisplay(hasData, getAlertState());
}