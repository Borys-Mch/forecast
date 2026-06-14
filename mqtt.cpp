#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"
#include "sensors.h"
#include "display.h"

extern AppConfig config;

WiFiClient espClient;
PubSubClient client(espClient);

static unsigned long lastPublish = 0;
static unsigned long lastDiscoveryPublish = 0;
static const char *baseTopic = "home/forecast";

static void topic(char *out, size_t len, const char *suffix)
{
  snprintf(out, len, "%s/%s", baseTopic, suffix);
}

static void mqttCallback(char *topicStr, byte *payload, unsigned int length)
{
  char cmdTopic[64];
  topic(cmdTopic, sizeof(cmdTopic), "brightness/set");

  if (strcmp(topicStr, cmdTopic) == 0 && length > 0)
  {
    char buf[16];
    memcpy(buf, payload, length);
    buf[length] = '\0';

    int brightness = atoi(buf);

    config.brightness = constrain(brightness, 0, 255);
    setBrightness(config.brightness);
    saveConfig(config);

    char out[16];
    snprintf(out, sizeof(out), "%d", config.brightness);

    char stateTopic[64];
    topic(stateTopic, sizeof(stateTopic), "brightness");
    client.publish(stateTopic, out, true);
  }
}

static void publishDiscoverySensor(const char *configId,
                                   const char *stateSuffix,
                                   const char *name,
                                   const char *unit,
                                   const char *deviceClass,
                                   const char *stateClass)
{
  char cfgTopic[96];
  char stateTopic[64];
  char avTopic[64];
  char payload[448];

  snprintf(cfgTopic, sizeof(cfgTopic), "homeassistant/sensor/forecast_lab/%s/config", configId);
  topic(stateTopic, sizeof(stateTopic), stateSuffix);
  topic(avTopic, sizeof(avTopic), "status");

  if (deviceClass && deviceClass[0] != '\0')
  {
    snprintf(payload, sizeof(payload),
             "{\"name\":\"%s\",\"uniq_id\":\"forecast_lab_v2_%s\",\"stat_t\":\"%s\","
             "\"avty_t\":\"%s\",\"pl_avail\":\"online\",\"pl_not_avail\":\"offline\","
             "\"unit_of_meas\":\"%s\",\"dev_cla\":\"%s\",\"stat_cla\":\"%s\","
             "\"dev\":{\"name\":\"Forecast Lab\",\"ids\":\"forecast_lab\","
             "\"mf\":\"Forecast Lab\",\"mdl\":\"ESP32 Forecast Station\"}}",
             name, configId, stateTopic, avTopic, unit, deviceClass, stateClass);
  }
  else
  {
    snprintf(payload, sizeof(payload),
             "{\"name\":\"%s\",\"uniq_id\":\"forecast_lab_v2_%s\",\"stat_t\":\"%s\","
             "\"avty_t\":\"%s\",\"pl_avail\":\"online\",\"pl_not_avail\":\"offline\","
             "\"unit_of_meas\":\"%s\",\"dev\":{\"name\":\"Forecast Lab\","
             "\"ids\":\"forecast_lab\",\"mf\":\"Forecast Lab\","
             "\"mdl\":\"ESP32 Forecast Station\"}}",
             name, configId, stateTopic, avTopic, unit);
  }

  client.publish(cfgTopic, payload, true);
}

static void publishDiscoveryBrightness()
{
  char topic[128];
  char payload[384];

  snprintf(topic, sizeof(topic),
           "homeassistant/number/forecast_lab/brightness/config");

  snprintf(payload, sizeof(payload),
           "{"
           "\"name\":\"Brightness\","
           "\"uniq_id\":\"forecast_lab_brightness\","
           "\"cmd_t\":\"home/forecast/brightness/set\","
           "\"stat_t\":\"home/forecast/brightness\","
           "\"min\":0,"
           "\"max\":255,"
           "\"step\":1,"
           "\"mode\":\"slider\","
           "\"avty_t\":\"home/forecast/status\","
           "\"pl_avail\":\"online\","
           "\"pl_not_avail\":\"offline\","
           "\"dev\":{"
           "\"name\":\"Forecast Lab\","
           "\"ids\":\"forecast_lab\","
           "\"mf\":\"Forecast Lab\","
           "\"mdl\":\"ESP32 Forecast Station\""
           "}"
           "}");

  client.publish(topic, payload, true);
}

static void publishDiscoveryTemperature()
{
  char cfgTopic[96];
  char stateTopic[64];
  char avTopic[64];
  char payload[448];

  snprintf(cfgTopic, sizeof(cfgTopic), "homeassistant/sensor/forecast_lab/temperature/config");
  topic(stateTopic, sizeof(stateTopic), "temp");
  topic(avTopic, sizeof(avTopic), "status");

  snprintf(payload, sizeof(payload),
           "{"
           "\"name\":\"Temperature\","
           "\"uniq_id\":\"forecast_lab_v3_temperature\","
           "\"stat_t\":\"%s\","
           "\"avty_t\":\"%s\","
           "\"pl_avail\":\"online\","
           "\"pl_not_avail\":\"offline\","
           "\"unit_of_meas\":\"\xC2\xB0"
           "C\","
           "\"dev_cla\":\"temperature\","
           "\"stat_cla\":\"measurement\","
           "\"dev\":{"
           "\"name\":\"Forecast Lab\","
           "\"ids\":\"forecast_lab\","
           "\"mf\":\"Forecast Lab\","
           "\"mdl\":\"ESP32 Forecast Station\""
           "}"
           "}",
           stateTopic, avTopic);

  client.publish(cfgTopic, payload, true);
}

static void publishDiscovery()
{
  publishDiscoveryTemperature();
  publishDiscoverySensor("humidity", "humidity", "Forecast Lab Humidity", "%", "humidity", "measurement");
  publishDiscoverySensor("co2", "co2", "Forecast Lab CO2", "ppm", "carbon_dioxide", "measurement");
  publishDiscoverySensor("pm25", "pm25", "Forecast Lab PM2.5", "\xC2\xB5g/m\xC2\xB3", "", "");
  publishDiscoverySensor("pm10", "pm10", "Forecast Lab PM10", "\xC2\xB5g/m\xC2\xB3", "", "");
  publishDiscoveryBrightness();
  lastDiscoveryPublish = millis();
}

void mqttReconnect()
{
  static unsigned long lastAttempt = 0;

  if (client.connected())
    return;

  if (config.mqttHost.length() == 0 || config.mqttPort <= 0)
  {
    lastAttempt = millis();
    return;
  }

  if (millis() - lastAttempt < 5000)
    return;

  lastAttempt = millis();

  const char *cid = "forecast_lab";
  char statusTopic[64];
  topic(statusTopic, sizeof(statusTopic), "status");

  bool connected;
  if (config.mqttUser.isEmpty())
  {
    connected = client.connect(cid, statusTopic, 1, true, "offline");
  }
  else
  {
    connected = client.connect(cid,
                               config.mqttUser.c_str(),
                               config.mqttPass.c_str(),
                               statusTopic,
                               1,
                               true,
                               "offline");
  }

  if (connected)
  {
    client.publish(statusTopic, "online", true);
    publishDiscovery();

    char cmdTopic[64];
    topic(cmdTopic, sizeof(cmdTopic), "brightness/set");
    client.subscribe(cmdTopic);
  }
}

void mqttInit()
{
  client.disconnect();
  client.setBufferSize(1024);
  client.setServer(config.mqttHost.c_str(), config.mqttPort);
  client.setCallback(mqttCallback);
  lastPublish = 0;
  lastDiscoveryPublish = 0;
}

void mqttPublish()
{
  if (!client.connected())
    return;

  char buf[16];
  char topicBuf[64];

  topic(topicBuf, sizeof(topicBuf), "temp");
  dtostrf(getTempLocal(), 1, 1, buf);
  client.publish(topicBuf, buf, true);

  topic(topicBuf, sizeof(topicBuf), "humidity");
  dtostrf(getHumidityLocal(), 1, 1, buf);
  client.publish(topicBuf, buf, true);

  topic(topicBuf, sizeof(topicBuf), "co2");
  dtostrf(getCO2(), 1, 0, buf);
  client.publish(topicBuf, buf, true);

  topic(topicBuf, sizeof(topicBuf), "pm25");
  dtostrf(getPM25(), 1, 1, buf);
  client.publish(topicBuf, buf, true);

  topic(topicBuf, sizeof(topicBuf), "pm10");
  dtostrf(getPM10(), 1, 1, buf);
  client.publish(topicBuf, buf, true);

  topic(topicBuf, sizeof(topicBuf), "brightness");
  snprintf(buf, sizeof(buf), "%d", config.brightness);
  client.publish(topicBuf, buf, true);

  topic(topicBuf, sizeof(topicBuf), "status");
  client.publish(topicBuf, "online", true);
}

void mqttLoop()
{
  if (!client.connected())
    mqttReconnect();

  client.loop();

  if (client.connected() && millis() - lastDiscoveryPublish > 300000)
  {
    publishDiscovery();
  }

  if (millis() - lastPublish > 10000)
  {
    mqttPublish();
    lastPublish = millis();
  }
}
