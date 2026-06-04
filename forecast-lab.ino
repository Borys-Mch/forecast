#include <WiFi.h>
#include "config.h"
#include "web.h"
#include "secrets.h"

AppConfig config;

void setup()
{
  Serial.begin(115200);

  loadConfig(config);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi OK");

  setupWeb(config);
}

void loop()
{
  handleWeb();
}