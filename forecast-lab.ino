#include "secrets.h"
#include "config.h"
#include "web.h"
#include "alerts.h"
#include "display.h"
#include "weather.h"
#include "sensors.h"
#include "mqtt.h"

bool hasData = false;

AppConfig config;
Adafruit_NeoPixel led(1, 8, NEO_GRB + NEO_KHZ800);

unsigned long lastAlertUpdate = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastBlink = 0;
bool ledState = false;

unsigned long forecastStart = 0;
static const unsigned long FORECAST_TIMEOUT = 15000;

void setup()
{
  Serial.begin(115200);
  delay(1000);

  led.begin();
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  loadConfig(config);
  setTempOffset(config.tempOffset);
  setHumidityOffset(config.humOffset);

  Serial.println("\nWiFi OK");

  pinMode(BTN_PIN, INPUT_PULLUP);
  updateWeather(config.apiKey, config.lat, config.lon);

  initDisplay();
  setBrightness(config.brightness);
  initSensors();
  setupWeb(config);
  mqttInit();
}

void loop()
{
  handleWeb();

  // ── кнопка ──
  static bool btnWasPressed = false;
  static unsigned long btnPressTime = 0;
  bool btnNow = (digitalRead(BTN_PIN) == LOW);

  if (btnNow && !btnWasPressed && millis() - btnPressTime > 200)
  {
    btnPressTime = millis();
    if (getCurrentScreen() != SCREEN_FORECAST)
    {
      forecastStart = millis(); // ← спочатку час, потім switch
      switchScreen(SCREEN_FORECAST);
    }
    else
    {
      switchScreen(SCREEN_MAIN);
    }
  }
  btnWasPressed = btnNow;

  // ── повернення на головну ──
  if (getCurrentScreen() == SCREEN_FORECAST &&
      millis() - forecastStart > FORECAST_TIMEOUT)
  {
    switchScreen(SCREEN_MAIN);
  }

  // ── тривога ──
  if (millis() - lastAlertUpdate > 10000)
  {
    lastAlertUpdate = millis();
    updateAlert(config.region);
    hasData = true;
  }

  if (getAlertState())
  {
    if (millis() - lastBlink > 500)
    {
      lastBlink = millis();
      ledState = !ledState;
      if (ledState)
        led.setPixelColor(0, led.Color(255, 0, 0));
      else
        led.setPixelColor(0, 0);
      led.show();
    }
  }
  else
  {
    led.setPixelColor(0, 0);
    led.show();
  }

  // ── погода ──
  if (millis() - lastWeatherUpdate > 300000)
  {
    lastWeatherUpdate = millis();
    updateWeather(config.apiKey, config.lat, config.lon);
  }

  updateSensors();

  // ── малювання ──
  if (millis() - lastDisplayUpdate > 250)
  {
    lastDisplayUpdate = millis();

    if (getCurrentScreen() == SCREEN_FORECAST)
      drawForecastScreen(hasData, getAlertState());
    else
      drawMainScreen(hasData, getAlertState());
  }

  mqttLoop();
  delay(1);
}