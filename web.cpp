#include "web.h"
#include "mqtt.h"
#include "config.h"
#include "secrets.h"
#include "sensors.h"
#include "display.h"
#include "fallback.h"

static WebServer server(80);
static AppConfig *cfgPtr;
extern Preferences prefs;

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

String getWiFiList()
{
  String list = "";

  int n = WiFi.scanNetworks();

  for (int i = 0; i < n; i++)
  {
    String ssid = WiFi.SSID(i);
    list += "<option value='" + ssid + "'>" + ssid + "</option>";
  }

  return list;
}

String htmlPage()
{
  String options = getRegionsOptionsCached(cfgPtr->region);
  String status = cachedOptions.indexOf("Error") != -1 ? "API ERROR" : "OK";

  String html = "<html><head><meta charset='UTF-8'></head><body>";

  html += "<h3>WiFi Setup</h3>";
  html += "<form method='POST' action='/wifi'>";
  html += "SSID:<br>";
  html += "<select name='ssid'>";
  html += getWiFiList();
  html += "</select><br>";
  html += "Password:<br><input name='pass' type='password'><br>";
  html += "<input type='submit' value='Save'>";
  html += "</form>";

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
  html += "<h3>CO2</h3>";
  html += "<button onclick=\"fetch('/calibrate',{method:'POST'})\">Calibrate CO2</button>";

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
  html += "</form>";
  html += "<h3>Оновлення прошивки</h3>";
  html += "<p>Version: " + String(FW_VERSION) + "</p>";
  html += "<form method='POST' action='/update' enctype='multipart/form-data'>";
  html += "<input type='file' name='update'><br>";
  html += "<input type='password' name='key' placeholder='OTA key'><br>";
  html += "<input type='submit' value='Upload'>";
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
  server.on("/calibrate", HTTP_POST, handleCalibrate);
  server.on("/update", HTTP_POST, []()
            {
    if (!server.hasArg("key") || server.arg("key") != OTA_KEY)
    {
      server.send(403, "text/plain", "Forbidden");
      return;
    }

    server.send(200, "text/plain", "OK");
    ESP.restart(); }, []()
            {
    if (!server.hasArg("key") || server.arg("key") != OTA_KEY)
    {
      return;
    }

    HTTPUpload& upload = server.upload();

    if (upload.status == UPLOAD_FILE_START)
    {
      Update.begin(UPDATE_SIZE_UNKNOWN);
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
      Update.write(upload.buf, upload.currentSize);
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
      Update.end(true);
    } });
  server.on("/version", HTTP_GET, []()
            { server.send(200, "text/plain", FW_VERSION); });
  server.on("/wifi", HTTP_POST, []()
            {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");

    saveWiFi(ssid, pass);

    server.send(200, "text/html", "Saved. Rebooting...");

    delay(1000);
    ESP.restart(); });
  server.begin();
}

void handleWeb()
{
  server.handleClient();
}

void handleCalibrate()
{
  calibrateCO2(415);
  server.send(200, "text/plain", "CO2 calibrated");
}

void saveWiFi(String ssid, String pass)
{
  prefs.begin("wifi", false);

  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);

  prefs.end();
}

String saved_ssid;
String saved_pass;

void loadWiFi()
{
  prefs.begin("wifi", true);

  saved_ssid = prefs.getString("ssid", "");
  saved_pass = prefs.getString("pass", "");

  prefs.end();
}