#include <WiFi.h>
#include "secrets.h"
#include "config.h"
#include "web.h"
#include "alerts.h"
#include "display.h"

#include <Adafruit_NeoPixel.h>

bool hasData = false;

AppConfig config;
Adafruit_NeoPixel led(1, 8, NEO_GRB + NEO_KHZ800);

unsigned long lastAlertUpdate = 0;

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

  if (millis() - lastAlertUpdate > 10000)
  {
    lastAlertUpdate = millis();

    updateAlert(config.region);

    Serial.println(getAlertState() ? "ALERT!" : "safe");
  }
  if (getAlertState())
  {
    led.setPixelColor(0, led.Color(255, 0, 0)); // червоний
  }
  else
  {
    led.setPixelColor(0, 0); // викл
  }
  led.show();

  static bool hasData = false;

  if (millis() - lastAlertUpdate > 10000)
  {
    lastAlertUpdate = millis();

    updateAlert(config.region);

    hasData = true;
  }

  updateDisplay(hasAlert(), getAlertState());
}