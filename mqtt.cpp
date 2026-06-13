#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"
#include "sensors.h"

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

  bool ok = client.publish(cfgTopic, payload, true);
  Serial.print("MQTT discovery ");
  Serial.print(configId);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
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
           "{\"name\":null,\"uniq_id\":\"forecast_lab_v3_temperature\",\"stat_t\":\"%s\","
           "\"avty_t\":\"%s\",\"pl_avail\":\"online\",\"pl_not_avail\":\"offline\","
           "\"unit_of_meas\":\"\xC2\xB0""C\",\"dev_cla\":\"temperature\",\"stat_cla\":\"measurement\","
           "\"dev\":{\"name\":\"Forecast Lab\",\"ids\":\"forecast_lab\","
           "\"mf\":\"Forecast Lab\",\"mdl\":\"ESP32 Forecast Station\"}}",
           stateTopic, avTopic);

  bool ok = client.publish(cfgTopic, payload, true);
  Serial.print("MQTT discovery temperature: ");
  Serial.println(ok ? "OK" : "FAIL");
}

static void publishDiscovery()
{
  publishDiscoveryTemperature();
  publishDiscoverySensor("humidity", "humidity", "Forecast Lab Humidity", "%", "humidity", "measurement");
  publishDiscoverySensor("co2", "co2", "Forecast Lab CO2", "ppm", "carbon_dioxide", "measurement");
  publishDiscoverySensor("pm25", "pm25", "Forecast Lab PM2.5", "\xC2\xB5g/m\xC2\xB3", "", "");
  publishDiscoverySensor("pm10", "pm10", "Forecast Lab PM10", "\xC2\xB5g/m\xC2\xB3", "", "");
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

  Serial.print("MQTT cfg: host=");
  Serial.print(config.mqttHost);
  Serial.print(" port=");
  Serial.print(config.mqttPort);
  Serial.print(" user=");
  Serial.println(config.mqttUser);

  Serial.print("MQTT connecting to ");
  Serial.print(config.mqttHost);
  Serial.print(":");
  Serial.print(config.mqttPort);
  Serial.print("...");

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
    Serial.println("OK");
    client.publish(statusTopic, "online", true);
    publishDiscovery();
  }
  else
  {
    Serial.print("fail: ");
    Serial.println(client.state());
  }
}

void mqttInit()
{
  client.disconnect();
  client.setBufferSize(1024);
  client.setServer(config.mqttHost.c_str(), config.mqttPort);
  lastPublish = 0;
  lastDiscoveryPublish = 0;

  Serial.print("MQTT init host=");
  Serial.print(config.mqttHost);
  Serial.print(" port=");
  Serial.print(config.mqttPort);
  Serial.print(" user=");
  Serial.println(config.mqttUser);
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
