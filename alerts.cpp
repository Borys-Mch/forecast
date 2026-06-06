#include "alerts.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

static bool currentAlert = false;
static bool hasAlertData = false;

static unsigned long lastCheck = 0;
static String lastPayload = "";

// кешуємо JSON на 10 сек
bool fetchAlertsJson(String &payload)
{
  if (millis() - lastCheck < 10000 && payload != "")
  {
    payload = lastPayload;
    return true;
  }

  HTTPClient http;
  http.begin("https://ubilling.net.ua/aerialalerts/");

  int code = http.GET();
  if (code != 200)
  {
    http.end();
    return false;
  }

  payload = http.getString();
  http.end();

  lastPayload = payload;
  lastCheck = millis();

  return true;
}

bool isAlertNow(String region)
{
  String payload;

  if (!fetchAlertsJson(payload))
    return false;

  StaticJsonDocument<12288> doc;
  DeserializationError err = deserializeJson(doc, payload);

  if (err)
    return false;

  JsonObject states = doc["states"];

  if (!states.containsKey(region))
    return false;

  return states[region]["alertnow"];
}

void updateAlert(String region)
{
  currentAlert = isAlertNow(region);
  hasAlertData = true;
}

bool hasAlert()
{
  return hasAlertData;
}

bool getAlertState()
{
  return currentAlert;
}
