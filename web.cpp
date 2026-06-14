#include "web.h"
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "sensors.h"
#include "mqtt.h"
#include "display.h"

static WebServer server(80);
static AppConfig *cfgPtr;

String cachedOptions = "";
unsigned long lastFetch = 0;

String getRegionsOptions(String selected)
{
  HTTPClient http;
  http.begin("https://ubilling.net.ua/aerialalerts/");

  int code = http.GET();
  if (code != 200 || code <= 0)
  {
    http.end();
    return "<option>Error loading</option>";
  }

  String payload = http.getString();
  http.end();

  StaticJsonDocument<12288> doc;
  DeserializationError err = deserializeJson(doc, payload);

  if (err)
  {
    return "<option>JSON error</option>";
  }

  String options = "";

  JsonObject states = doc["states"];

  for (JsonPair kv : states)
  {
    String name = kv.key().c_str();

    options += "<option value='" + name + "'";

    if (name == selected)
    {
      options += " selected";
    }

    options += ">" + name + "</option>";
  }

  return options;
}

String getRegionsOptionsCached(String selected)
{
  if (millis() - lastFetch > 600000 || cachedOptions == "") // 10 хв
  {
    cachedOptions = getRegionsOptions(selected);
    lastFetch = millis();
  }

  return cachedOptions;
}

String htmlPage()
{
  String options = getRegionsOptionsCached(cfgPtr->region);
  String status = cachedOptions.indexOf("Error") != -1 ? "API ERROR" : "OK";

  String html = "<html><head><meta charset='UTF-8'></head><body>";
  html += "<h2>Forecast Lab Config</h2>";

  html += "<p>Status: " + status + "</p>";

  html += "<form action='/save'>";

  html += "API Key:<br>";
  html += "<input name='api' value='" + cfgPtr->apiKey + "'><br>";

  html += "Lat:<br>";
  html += "<input name='lat' value='" + String(cfgPtr->lat, 6) + "'><br>";

  html += "Lon:<br>";
  html += "<input name='lon' value='" + String(cfgPtr->lon, 6) + "'><br>";

  html += "Region:<br>";
  html += "<select name='region'>" + options + "</select><br><br>";

  html += "<h3>Calibration</h3>";

  html += "Temp offset:<br>";
  html += "<input name='to' value='" + String(cfgPtr->tempOffset, 1) + "'><br>";

  html += "Humidity offset:<br>";
  html += "<input name='ho' value='" + String(cfgPtr->humOffset, 1) + "'><br><br>";

  html += "<h3>Display</h3>";
  html += "Brightness: <br>";
  html += "<input name='brightness' type='range' min='0' max='255' value='" + String(cfgPtr->brightness) + "' class='slider' oninput='this.nextElementSibling.value=this.value'><br>";
  html += "<input type='text' value='" + String(cfgPtr->brightness) + "' class='number' readonly><br><br>";

  html += "<h3>MQTT</h3>";
  html += "Host:<br><input name='mqtt_host' value='" + cfgPtr->mqttHost + "'><br>";
  html += "Port:<br><input name='mqtt_port' value='" + String(cfgPtr->mqttPort) + "'><br>";
  html += "User:<br><input name='mqtt_user' value='" + cfgPtr->mqttUser + "'><br>";
  html += "Password:<br><input name='mqtt_pass' value='" + cfgPtr->mqttPass + "'><br><br>";

  html += "<input type='submit' value='Save'>";
  html += "</form></body></html>";
  return html;
}

void handleRoot()
{
  server.send(200, "text/html; charset=utf-8", htmlPage());
}

void handleSave()
{
  cfgPtr->apiKey = server.arg("api");
  cfgPtr->lat = server.arg("lat").toFloat();
  cfgPtr->lon = server.arg("lon").toFloat();
  cfgPtr->region = server.arg("region");

  if (server.hasArg("brightness"))
  {
    cfgPtr->brightness = server.arg("brightness").toInt();
    setBrightness(cfgPtr->brightness);
  }

  cfgPtr->mqttHost = server.arg("mqtt_host");
  cfgPtr->mqttHost.trim();

  cfgPtr->mqttPort = server.arg("mqtt_port").toInt();
  if (cfgPtr->mqttPort <= 0)
    cfgPtr->mqttPort = 1883;

  cfgPtr->mqttUser = server.arg("mqtt_user");
  cfgPtr->mqttUser.trim();
  cfgPtr->mqttPass = server.arg("mqtt_pass");

  if (server.hasArg("to"))
    cfgPtr->tempOffset = server.arg("to").toFloat();

  if (server.hasArg("ho"))
    cfgPtr->humOffset = server.arg("ho").toFloat();

  saveConfig(*cfgPtr);
  cachedOptions = "";
  lastFetch = 0;
  setTempOffset(cfgPtr->tempOffset);
  setHumidityOffset(cfgPtr->humOffset);
  mqttInit();

  server.sendHeader("Location", "/");
  server.send(303);
}

void setupWeb(AppConfig &cfg)
{
  cfgPtr = &cfg;
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();
}

void handleWeb()
{
  server.handleClient();
}