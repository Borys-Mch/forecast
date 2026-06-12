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
  tft.drawLine(0, 177, tft.width(), 177, tft.color565(120, 120, 120));
  tft.drawLine(0, 180, tft.width(), 180, tft.color565(120, 120, 120));
}

void updateDisplay(bool hasData, bool alert)
{
  static String lastText = "";
  static float lastTemp = -1000;
  static float lastFeels = -1000;
  static float lastHumidity = -1000;
  static float lastWindSpeed = -1000;
  static int lastCO2 = -1;
  static String lastPM25Text = "";
  static String lastPM10Text = "";
  static String lastTempText = "";
  static String lastHumText = "";

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

      int iconSize = 55;

      int x = tft.width() - 225; // вся група (іконка + текст)
      int y = 57;

      // очистка області
      tft.fillRect(x, y, 230, 110, ST77XX_BLACK);

      int code = getWeatherCode();
      bool isDay = getIsDay();

      const uint16_t *iconBitmap = getWeatherIcon(code, isDay);

      drawBitmapTransparent(x, y, iconBitmap, iconSize, iconSize);

      u8g2.setFont(u8g2_font_fub42_tn);
      u8g2.setCursor(x + iconSize + 15, y + 49); // Y = baseline!
      u8g2.setForegroundColor(tft.color565(86, 174, 194));
      u8g2.print((int)temp);

      u8g2.setFont(u8g2_font_10x20_t_cyrillic);
      u8g2.setCursor(x + iconSize + 85, y + 11);
      u8g2.print("o");

      tft.drawLine(190, 59, 165, 104, tft.color565(120, 120, 120));

      u8g2.setFont(u8g2_font_fub20_tn);
      u8g2.setCursor(x + 175, y + 49);
      u8g2.setForegroundColor(tft.color565(120, 120, 120));
      u8g2.print((int)feels_like);

      u8g2.setFont(u8g2_font_10x20_t_cyrillic);
      u8g2.setCursor(x + 205, y + 34);
      u8g2.setForegroundColor(tft.color565(120, 120, 120));
      u8g2.print("o");

      tft.drawLine(10, y + 72, 230, y + 72, tft.color565(99, 99, 99));

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
  }

  String tempText = "";
  if (temp_scd > 0)
  {
    tempText = String((int)temp_scd);
  }

  if (tempText != lastTempText)
  {
    tft.fillRect(10, 190, 110, 60, ST77XX_BLACK);

    if (tempText.length() > 0)
    {
      u8g2.setFont(u8g2_font_fub42_tn);
      u8g2.setCursor(20, 240);
      u8g2.setForegroundColor(tft.color565(86, 174, 194));
      u8g2.print(tempText);

      u8g2.setFont(u8g2_font_10x20_t_cyrillic);
      u8g2.setCursor(90, 200);
      u8g2.print("o");
    }

    lastTempText = tempText;
  }

  String humText = "";
  if (hum_scd > 0)
  {
    humText = String((int)hum_scd);
  }

  if (humText != lastHumText)
  {
    tft.fillRect(130, 190, 90, 60, ST77XX_BLACK);

    if (humText.length() > 0)
    {
      u8g2.setFont(u8g2_font_fub42_tn);
      u8g2.setCursor(140, 240);
      u8g2.setForegroundColor(tft.color565(86, 174, 194));
      u8g2.print(humText);

      u8g2.setFont(u8g2_font_10x20_t_cyrillic);
      u8g2.setCursor(210, 240);
      u8g2.print("%");
    }

    lastHumText = humText;
  }

  tft.drawLine(10, 250, 230, 250, tft.color565(120, 120, 120));

  u8g2.setFont(u8g2_font_10x20_t_cyrillic);
  u8g2.setForegroundColor(tft.color565(120, 120, 120));
  u8g2.setCursor(30, 275);
  u8g2.print("СО2");

  const int co2Value = (int)co2;
  if (co2Value != lastCO2)
  {
    tft.fillRect(10, 287, 90, 28, ST77XX_BLACK);

    if (co2Value > 0)
    {
      u8g2.setFont(u8g2_font_fub20_tn);
      u8g2.setForegroundColor(tft.color565(150, 150, 150));

      if (co2Value >= 1000)
        u8g2.setCursor(15, 310);
      else
        u8g2.setCursor(20, 310);

      if (co2Value >= 2000)
        u8g2.setForegroundColor(tft.color565(145, 36, 255));
      else if (co2Value >= 1400)
        u8g2.setForegroundColor(tft.color565(254, 32, 32));
      else if (co2Value >= 800)
        u8g2.setForegroundColor(tft.color565(255, 235, 107));
      else
        u8g2.setForegroundColor(tft.color565(122, 225, 144));

      u8g2.print(co2Value);
    }

    lastCO2 = co2Value;
  }

  tft.drawLine(100, 250, 100, 310, tft.color565(120, 120, 120));

  u8g2.setFont(u8g2_font_10x20_t_cyrillic);
  u8g2.setForegroundColor(tft.color565(120, 120, 120));
  u8g2.setCursor(110, 275);
  u8g2.print("РМ2.5");

  String pm25Text = "";
  if (pm2_5 > 0)
  {
    pm25Text = pm2_5 < 10 ? String(pm2_5, 1) : String((int)pm2_5);
  }

  if (pm25Text != lastPM25Text)
  {
    tft.fillRect(105, 287, 60, 28, ST77XX_BLACK);

    if (pm25Text.length() > 0)
    {
      u8g2.setFont(u8g2_font_fub20_tn);
      u8g2.setCursor(110, 310);

      if (pm2_5 >= 75)
        u8g2.setForegroundColor(tft.color565(145, 36, 255));
      else if (pm2_5 >= 35)
        u8g2.setForegroundColor(tft.color565(254, 32, 32));
      else if (pm2_5 >= 15)
        u8g2.setForegroundColor(tft.color565(255, 130, 46));
      else if (pm2_5 >= 5)
        u8g2.setForegroundColor(tft.color565(255, 235, 107));
      else
        u8g2.setForegroundColor(tft.color565(122, 225, 144));

      u8g2.print(pm25Text);
    }

    lastPM25Text = pm25Text;
  }

  tft.drawLine(170, 250, 170, 310, tft.color565(120, 120, 120));

  u8g2.setFont(u8g2_font_10x20_t_cyrillic);
  u8g2.setForegroundColor(tft.color565(120, 120, 120));
  u8g2.setCursor(180, 275);
  u8g2.print("РМ10");

  String pm10Text = "";
  if (pm10 > 0)
  {
    pm10Text = pm10 < 10 ? String(pm10, 1) : String((int)pm10);
  }

  if (pm10Text != lastPM10Text)
  {
    tft.fillRect(175, 287, 60, 28, ST77XX_BLACK);

    if (pm10Text.length() > 0)
    {
      u8g2.setFont(u8g2_font_fub20_tn);
      u8g2.setCursor(180, 310);

      if (pm10 >= 200)
        u8g2.setForegroundColor(tft.color565(145, 36, 255));
      else if (pm10 >= 100)
        u8g2.setForegroundColor(tft.color565(254, 32, 32));
      else if (pm10 >= 50)
        u8g2.setForegroundColor(tft.color565(255, 130, 46));
      else if (pm10 >= 20)
        u8g2.setForegroundColor(tft.color565(255, 235, 107));
      else
        u8g2.setForegroundColor(tft.color565(122, 225, 144));

      u8g2.print(pm10Text);
    }

    lastPM10Text = pm10Text;
  }
}
