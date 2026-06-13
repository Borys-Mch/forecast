#include "web.h"
#include <WebServer.h>
#include "sensors.h"
#include "mqtt.h"

static WebServer server(80);
static AppConfig *cfgPtr;

String htmlPage()
{
  String html = "<html><head><meta charset='UTF-8'></head><body>";
  html += "<h2>Forecast Lab Config</h2>";
  html += "<form action='/save'>";
  html += "API Key:<br>";
  html += "<input name='api' type='password' value='" + cfgPtr->apiKey + "'><br>";
  html += "Lat:<br>";
  html += "<input name='lat' value='" + String(cfgPtr->lat, 6) + "'><br>";
  html += "Lon:<br>";
  html += "<input name='lon' value='" + String(cfgPtr->lon, 6) + "'><br>";
  html += "Region:<br>";
  html += "<input name='region' value='" + cfgPtr->region + "'><br><br>";
  html += "<h3>Calibration</h3>";
  html += "Temp offset:<br>";
  html += "<input name='to' value='" + String(cfgPtr->tempOffset, 1) + "'><br>";
  html += "Humidity offset:<br>";
  html += "<input name='ho' value='" + String(cfgPtr->humOffset, 1) + "'><br><br>";
  html += "<h3>MQTT</h3>";
  html += "Host:<br>";
  html += "<input name='mqtt_host' value='" + cfgPtr->mqttHost + "'><br>";
  html += "Port:<br>";
  html += "<input name='mqtt_port' value='" + String(cfgPtr->mqttPort) + "'><br>";
  html += "User:<br>";
  html += "<input name='mqtt_user' value='" + cfgPtr->mqttUser + "'><br>";
  html += "Password:<br>";
  html += "<input name='mqtt_pass' value='" + cfgPtr->mqttPass + "'><br>";
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
