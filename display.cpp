#include "display.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <SPI.h>
#include <WiFi.h>
#include <math.h>
#include "weather.h"
#include "icons.h"
#include "systemicons.h"
#include "alerts.h"
#include "sensors.h"

// ===== ПІНИ =================================================
#define TFT_CS 1
#define TFT_DC 2
#define TFT_RST 3
#define TFT_MISO -1
#define TFT_MOSI 6
#define TFT_SCK 7
#define TFT_BL 23

U8G2_FOR_ADAFRUIT_GFX u8g2;
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

void drawBitmapTransparent(int x, int y, const uint16_t *bitmap, int w, int h)
{
  for (int j = 0; j < h; j++)
  {
    for (int i = 0; i < w; i++)
    {
      uint16_t color = pgm_read_word(&bitmap[j * w + i]);

      if (color != 0x0000) // чорний = прозорий
      {
        tft.drawPixel(x + i, y + j, color);
      }
    }
  }
}

const uint16_t *getWeatherIcon(int code, bool isDay)
{
  if (code == 1000)
    return isDay ? weather_clear : weather_clear_n;

  if (code == 1003)
    return isDay ? weather_few_clouds : weather_few_clouds_n;

  if (code == 1006 || code == 1009)
    return weather_clouds;

  if (code == 1030 || code == 1135 || code == 1147)
    return weather_fog;

  if (code == 1063 || code == 1150 || code == 1153 || code == 1168 || code == 1171 || code == 1180 || code == 1183 || code == 1186 || code == 1189 || code == 1192 || code == 1195 || code == 1198 || code == 1201 || code == 1237 || code == 1240 || code == 1243 || code == 1246)
    return weather_rain;

  if (code == 1066 || code == 1069 || code == 1072 || code == 1114 || code == 1117 || code == 1204 || code == 1207 || code == 1210 || code == 1213 || code == 1216 || code == 1219 || code == 1222 || code == 1225 || code == 1249 || code == 1252 || code == 1255 || code == 1258 || code == 1261 || code == 1264)
    return weather_snow;

  if (code == 1087 || code == 1273 || code == 1276 || code == 1279 || code == 1282)
    return weather_thunderstorm;

  return weather_clouds;
}

const uint16_t *getAlertIcon(bool alert)
{
  return alert ? system_alert : system_no_alert;
}

const uint16_t *getWiFiIcon()
{
  if (WiFi.status() != WL_CONNECTED)
    return system_no_wifi;

  int rssi = WiFi.RSSI();

  if (rssi > -60)
    return system_wifi_3; // сильний
  else if (rssi > -70)
    return system_wifi_2; // середній
  else
    return system_wifi_1; // слабкий
}

float calculateFeelsLike(float t, float h, float v)
{
  // ❄️ холод + вітер (wind chill)
  if (t <= 10 && v > 1.3)
  {
    return 13.12 + 0.6215 * t - 11.37 * pow(v, 0.16) + 0.3965 * t * pow(v, 0.16);
  }

  // 🔥 спека + вологість (heat index)
  if (t >= 24)
  {
    return -8.784695 + 1.61139411 * t + 2.338549 * h - 0.14611605 * t * h - 0.012308094 * t * t - 0.016424828 * h * h + 0.002211732 * t * t * h + 0.00072546 * t * h * h - 0.000003582 * t * t * h * h;
  }

  // 🌤 комфортна зона (твій кейс)
  float feels = t;

  // вітер
  feels -= v * 0.6;

  // вологість
  feels += (h - 50) * 0.04;

  return feels;
}

void initDisplay()
{
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.init(240, 320);
  tft.setRotation(2); // Вертикально піни зверху

  u8g2.begin(tft);

  tft.fillScreen(ST77XX_BLACK);

  // лінії
  tft.drawLine(0, 40, tft.width(), 40, tft.color565(120, 120, 120));
  tft.drawLine(0, 166, tft.width(), 166, tft.color565(120, 120, 120));
  tft.drawLine(0, 169, tft.width(), 169, tft.color565(120, 120, 120));
}

void updateDisplay(bool hasData, bool alert)
{
  static String lastText = "";
  static float lastTemp = -1000;
  static float lastFeels = -1000;
  static float lastHumidity = -1000;
  static float lastWindSpeed = -1000;

  const float co2 = getCO2();
  const float pm2_5 = getPM25();
  const float pm10 = getPM10();
  const float temp_scd = getTempLocal();
  const float hum_scd = getHumidityLocal();

  String text;
  const uint16_t *iconAlert = getAlertIcon(alert);
  const uint16_t *wifiIcon = getWiFiIcon();

  drawBitmapTransparent(5, 10, iconAlert, 20, 20);
  drawBitmapTransparent(tft.width() - 25, 10, wifiIcon, 20, 20);

  if (!hasData)
    text = "Немає даних";
  else if (alert)
    text = "Тривога";
  else
    text = "Безпечно";

  if (text != lastText)
  {
    lastText = text;

    tft.fillRect(0, 0, tft.width(), 40, ST77XX_BLACK);

    u8g2.setFont(u8g2_font_10x20_t_cyrillic);
    u8g2.setCursor(35, 25);

    if (!hasData)
      u8g2.setForegroundColor(tft.color565(120, 120, 120));
    else if (alert)
      u8g2.setForegroundColor(tft.color565(255, 111, 111));
    else
      u8g2.setForegroundColor(tft.color565(180, 180, 180));

    u8g2.print(text);
  }

  if (hasWeather())
  {
    float temp = getTemperature();
    float humidity = getHumidity();
    float speed = getWindSpeed();
    float feels_like = calculateFeelsLike(
        getTemperature(),
        getHumidity(),
        getWindSpeed());

    if (abs(temp - lastTemp) > 0.1 ||
        abs(feels_like - lastFeels) > 0.1 ||
        abs(humidity - lastHumidity) > 1 ||
        abs(speed - lastWindSpeed) > 0.1)
    {
      lastTemp = temp;
      lastFeels = feels_like;
      lastHumidity = humidity;
      lastWindSpeed = speed;

      int iconSize = 60;

      int x = tft.width() - 225; // вся група (іконка + текст)
      int y = 55;

      // очистка області
      tft.fillRect(x, y, 230, 110, ST77XX_BLACK);

      int code = getWeatherCode();
      bool isDay = getIsDay();

      const uint16_t *iconBitmap = getWeatherIcon(code, isDay);

      drawBitmapTransparent(x, y, iconBitmap, iconSize, iconSize);

      u8g2.setFont(u8g2_font_fub42_tn);
      u8g2.setCursor(x + iconSize + 15, y + 51); // Y = baseline!
      u8g2.setForegroundColor(tft.color565(86, 174, 194));
      u8g2.print((int)temp);

      u8g2.setFont(u8g2_font_10x20_t_cyrillic);
      u8g2.setCursor(x + iconSize + 85, y + 13);
      u8g2.print("o");

      tft.drawLine(190, 61, 165, 106, tft.color565(120, 120, 120));

      u8g2.setFont(u8g2_font_fub20_tn);
      u8g2.setCursor(x + 175, y + 51);
      u8g2.setForegroundColor(tft.color565(120, 120, 120));
      u8g2.print((int)feels_like);

      u8g2.setFont(u8g2_font_10x20_t_cyrillic);
      u8g2.setCursor(x + 205, y + 36);
      u8g2.setForegroundColor(tft.color565(120, 120, 120));
      u8g2.print("o");

      tft.drawLine(10, y + 75, 230, y + 75, tft.color565(99, 99, 99));

      drawBitmapTransparent(15, y + 85, system_humidity, 20, 20);

      u8g2.setFont(u8g2_font_fub20_tn);
      u8g2.setCursor(x + 35, y + 105);
      u8g2.setForegroundColor(tft.color565(150, 150, 150));
      u8g2.print((int)humidity);

      u8g2.setFont(u8g2_font_10x20_t_cyrillic);
      u8g2.setCursor(x + 70, y + 105);
      u8g2.print("%");

      drawBitmapTransparent(110, y + 85, system_wind, 20, 20);

      u8g2.setFont(u8g2_font_fub20_tn);
      u8g2.setCursor(x + 130, y + 105);
      u8g2.setForegroundColor(tft.color565(150, 150, 150));

      if (speed < 10)
        u8g2.print(String(speed, 1));
      else
        u8g2.print((int)speed);

      u8g2.setFont(u8g2_font_10x20_t_cyrillic);
      u8g2.setCursor(x + 180, y + 105);
      u8g2.print("м/с");
    }

    if (getTempLocal())
    {
      u8g2.setFont(u8g2_font_fub42_tn);
      u8g2.setCursor(15, 220);
      u8g2.setForegroundColor(tft.color565(86, 174, 194));
      u8g2.print((int)temp_scd);

      u8g2.setFont(u8g2_font_10x20_t_cyrillic);
      u8g2.setCursor(75, 180);
      u8g2.print("o");
    }

    if (getHumidityLocal())
    {
      u8g2.setFont(u8g2_font_fub42_tn);
      u8g2.setCursor(100, 220);
      u8g2.setForegroundColor(tft.color565(86, 174, 194));
      u8g2.print((int)hum_scd);

      u8g2.setFont(u8g2_font_10x20_t_cyrillic);
      u8g2.setCursor(160, 220);
      u8g2.print("%");
    }

    tft.drawLine(10, 230, 230, 230, tft.color565(120, 120, 120));

    u8g2.setFont(u8g2_font_10x20_t_cyrillic);
    u8g2.setCursor(30, 260);
    u8g2.print("СО2");

    if (getCO2())
    {
      u8g2.setFont(u8g2_font_fub20_tn);
      u8g2.setCursor(15, 300);
      u8g2.setForegroundColor(tft.color565(150, 150, 150));
      u8g2.print((int)co2);
    }

    tft.drawLine(100, 230, 100, 310, tft.color565(120, 120, 120));

    u8g2.setFont(u8g2_font_10x20_t_cyrillic);
    u8g2.setCursor(110, 260);
    u8g2.print("РМ2.5");

    if (getPM25())
    {
      u8g2.setFont(u8g2_font_fub20_tn);
      u8g2.setCursor(110, 300);
      u8g2.setForegroundColor(tft.color565(150, 150, 150));
      u8g2.print((int)pm2_5);
    }

    tft.drawLine(170, 230, 170, 310, tft.color565(120, 120, 120));

    u8g2.setFont(u8g2_font_10x20_t_cyrillic);
    u8g2.setCursor(180, 260);
    u8g2.print("РМ10");

    if (getPM10())
    {
      u8g2.setFont(u8g2_font_fub20_tn);
      u8g2.setCursor(180, 300);
      u8g2.setForegroundColor(tft.color565(150, 150, 150));
      u8g2.print((int)pm10);
    }
  }
}
