#include "web.h"
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "sensors.h"

static WebServer server(80);
static AppConfig *config;

String cachedOptions = "";
unsigned long lastFetch = 0;

String getRegionsOptions(String selected)
{
  HTTPClient http;
  http.begin("https://ubilling.net.ua/aerialalerts/");

  int code = http.GET();
  if (code != 200)
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
  String options = getRegionsOptionsCached(config->region);
  String status = cachedOptions.indexOf("Error") != -1 ? "API ERROR" : "OK";

  String html = "<html><head><meta charset='UTF-8'></head><body>";
  html += "<h2>Forecast Lab Config</h2>";

  html += "<p>Status: " + status + "</p>";

  html += "<form action='/save'>";

  html += "API Key:<br>";
  html += "<input name='api' value='" + config->apiKey + "'><br>";

  html += "Lat:<br>";
  html += "<input name='lat' value='" + String(config->lat, 6) + "'><br>";

  html += "Lon:<br>";
  html += "<input name='lon' value='" + String(config->lon, 6) + "'><br>";

  html += "Region:<br>";
  html += "<select name='region'>" + options + "</select><br><br>";

  html += "<h3>Calibration</h3>";

  html += "Temp offset:<br>";
  html += "<input name='to' value='" + String(config->tempOffset, 1) + "'><br>";

  html += "Humidity offset:<br>";
  html += "<input name='ho' value='" + String(config->humOffset, 1) + "'><br><br>";

  html += "<input type='submit' value='Save'>";
  html += "</form>";

  html += "</body></html>";

  return html;
}

void handleRoot()
{
  server.send(200, "text/html; charset=utf-8", htmlPage());
}

void handleSave()
{
  config->apiKey = server.arg("api");
  config->lat = server.arg("lat").toFloat();
  config->lon = server.arg("lon").toFloat();
  config->region = server.arg("region");

  if (server.hasArg("to"))
    config->tempOffset = server.arg("to").toFloat();

  if (server.hasArg("ho"))
    config->humOffset = server.arg("ho").toFloat();

  saveConfig(*config);

  setTempOffset(config->tempOffset);
  setHumidityOffset(config->humOffset);

  cachedOptions = "";
  lastFetch = 0;

  server.sendHeader("Location", "/");
  server.send(303);
}

void setupWeb(AppConfig &cfg)
{
  config = &cfg;

  server.on("/", handleRoot);
  server.on("/save", handleSave);

  server.begin();
}

void handleWeb()
{
  server.handleClient();
}