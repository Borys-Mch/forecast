#include "config.h"

bool connectWiFi(const char *ssid, const char *pass)
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000)
  {
    delay(500);
  }

  return WiFi.status() == WL_CONNECTED;
}

void startAP()
{
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ForecastLab_Setup");

  Serial.println(WiFi.softAPIP());
}