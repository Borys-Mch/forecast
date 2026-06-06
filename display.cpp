#include "display.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <SPI.h>
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

const uint16_t *getWeatherIconBitmap(String icon)
{
  if (icon == "01d")
    return weather_clear;
  if (icon == "01n")
    return weather_clear_n;

  if (icon == "02d")
    return weather_few_clouds;
  if (icon == "02n")
    return weather_few_clouds_n;

  if (icon == "03d" || icon == "03n" || icon == "04d" || icon == "04n")
    return weather_clouds;

  if (icon == "10d" || icon == "09d")
    return weather_rain;

  if (icon == "13d" || icon == "13n")
    return weather_snow;

  if (icon == "50d")
    return weather_fog;

  if (icon == "11n")
    return weather_thunderstorm;

  return weather_clouds; // дефолт
}

const uint16_t *getAlertIconBitmap(bool alert)
{
  return alert ? system_alert : system_no_alert;
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

  String text;
  const uint16_t *iconAlert = getAlertIconBitmap(alert);

  drawBitmapTransparent(5, 10, iconAlert, 20, 20);

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

    if (abs(temp - lastTemp) > 0.1)
    {
      lastTemp = temp;
      lastFeels = feels_like;

      int iconSize = 70;

      int x = tft.width() - 230; // 🔥 вся група (іконка + текст)
      int y = 55;

      // очистка області
      tft.fillRect(x, y, 200, 80, ST77XX_BLACK);

      String iconCode = getWeatherIcon();
      const uint16_t *iconBitmap = getWeatherIconBitmap(iconCode);

      drawBitmapTransparent(x, y, iconBitmap, iconSize, iconSize);

      tft.setCursor(x + iconSize + 20, y + 10);
      tft.setTextSize(4);
      tft.setTextColor(ST77XX_CYAN);

      tft.print(String(temp, 1));
      tft.print("C");

      tft.setCursor(x + 20, y + iconSize + 20);
      tft.setTextSize(2);
      tft.setTextColor(tft.color565(180, 180, 180));
      tft.print("Feels like: ");
      tft.print(String(feels_like, 1));
    }
  }
}