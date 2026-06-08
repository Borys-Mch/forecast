#include "display.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <SPI.h>
#include <WiFi.h>
#include "weather.h"
#include "icons.h"
#include "systemicons.h"
#include "alerts.h"

// ===== ПІНИ =================================================
#define TFT_CS 1
#define TFT_DC 2
#define TFT_RST 3
#define TFT_MISO -1
#define TFT_MOSI 6
#define TFT_SCK 7

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

void initDisplay()
{
  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.init(240, 320);
  tft.setRotation(2); // Вертикально піни зверху

  u8g2.begin(tft);

  tft.fillScreen(ST77XX_BLACK);

  // лінії
  tft.drawLine(0, 40, tft.width(), 40, tft.color565(120, 120, 120));
  tft.drawLine(0, 180, tft.width(), 180, tft.color565(120, 120, 120));
}

void updateDisplay(bool hasData, bool alert)
{
  static String lastText = "";
  static float lastTemp = -1000;
  static float lastFeels = -1000;
  static float lastHumidity = -1000;
  static float lastWindSpeed = -1000;

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
    float feels_like = getTemperatureFeels();
    float humidity = getHumidity();
    float speed = getWindSpeed();

    if (abs(temp - lastTemp) > 0.1)
    {
      lastTemp = temp;
      lastFeels = feels_like;
      lastHumidity = humidity;
      lastWindSpeed = speed;

      int iconSize = 67;

      int x = tft.width() - 230; // 🔥 вся група (іконка + текст)
      int y = 55;

      // очистка області
      tft.fillRect(x, y, 200, 80, ST77XX_BLACK);

      int code = getWeatherCode();
      bool isDay = getIsDay();

      const uint16_t *iconBitmap = getWeatherIcon(code, isDay);

      drawBitmapTransparent(x, y, iconBitmap, iconSize, iconSize);

      u8g2.setFont(u8g2_font_fub42_tn);
      u8g2.setCursor(x + iconSize + 20, y + 55); // Y = baseline!

      u8g2.setForegroundColor(tft.color565(86, 174, 194));
      u8g2.print((int)temp);

      u8g2.setFont(u8g2_font_10x20_t_cyrillic);
      u8g2.setCursor(x + iconSize + 100, y + 20);
      u8g2.print("o");

      tft.drawLine(200, 70, 180, 110, tft.color565(120, 120, 120));

      u8g2.setFont(u8g2_font_fub20_tn);
      u8g2.setCursor(x + 190, y + 55);
      u8g2.setForegroundColor(tft.color565(120, 120, 120));
      u8g2.print((int)feels_like);

      tft.drawLine(10, 140, 230, 140, tft.color565(99, 99, 99));

      drawBitmapTransparent(15, 150, system_humidity, 20, 20);

      u8g2.setFont(u8g2_font_fub20_tn);
      u8g2.setCursor(x + 35, y + 115);
      u8g2.setForegroundColor(tft.color565(150, 150, 150));
      u8g2.print((int)humidity);

      u8g2.setFont(u8g2_font_10x20_t_cyrillic);
      u8g2.setCursor(x + 70, y + 115);
      u8g2.print("%");

      drawBitmapTransparent(110, 150, system_wind, 20, 20);

      u8g2.setFont(u8g2_font_fub20_tn);
      u8g2.setCursor(x + 130, y + 115);
      u8g2.setForegroundColor(tft.color565(150, 150, 150));

      if (speed < 10)
        u8g2.print(String(speed, 1));
      else
        u8g2.print((int)speed);

      u8g2.setFont(u8g2_font_10x20_t_cyrillic);
      u8g2.setCursor(x + 180, y + 115);
      u8g2.print("м/с");
    }
  }
}