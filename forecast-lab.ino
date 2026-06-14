#include <WiFi.h>
#include "secrets.h"
#include "config.h"
#include "web.h"
#include "alerts.h"
#include "display.h"
#include "weather.h"
#include "sensors.h"
#include "mqtt.h"

#include <Adafruit_NeoPixel.h>

bool hasData = false;

AppConfig config;
Adafruit_NeoPixel led(1, 8, NEO_GRB + NEO_KHZ800);

unsigned long lastAlertUpdate = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastBlink = 0;
bool ledState = false;

void setup()
{
  Serial.begin(115200);
  delay(1000);

  led.begin();
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  loadConfig(config);
  setTempOffset(config.tempOffset);
  setHumidityOffset(config.humOffset);

  Serial.println("\nWiFi OK");
  updateWeather(config.apiKey, config.lat, config.lon);

  initDisplay();
  setBrightness(config.brightness);
  initSensors();
  setupWeb(config);
  mqttInit();
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
  if (millis() - lastWeatherUpdate > 300000)
  {
    lastWeatherUpdate = millis();
    updateWeather(config.apiKey, config.lat, config.lon);
  }

  updateSensors();

  if (millis() - lastDisplayUpdate > 250)
  {
    lastDisplayUpdate = millis();
    updateDisplay(hasData, getAlertState());
  }

  mqttLoop();

  delay(1);
}
