#include "display.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// 🔧 під свої піни
#define TFT_CS 1
#define TFT_DC 2
#define TFT_RST 3

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

void initDisplay()
{
  SPI.begin(7, -1, 6, 1);
  tft.init(240, 320);
  tft.setRotation(2); // горизонтально

  tft.fillScreen(ST77XX_BLACK);

  // лінія
  tft.drawLine(0, 40, tft.width(), 40, ST77XX_WHITE);
}

void updateDisplay(bool hasData, bool alert)
{
  static String lastText = "";

  String text;

  if (!hasData)
    text = "no data";
  else if (alert)
    text = "ALERT"; // ⚠ кирилицю поки не юзаємо
  else
    text = "SAFE";

  // щоб не моргало
  if (text == lastText)
    return;

  lastText = text;

  // очистка верхньої частини
  tft.fillRect(0, 0, tft.width(), 40, ST77XX_BLACK);

  tft.setCursor(5, 10);
  tft.setTextSize(2);

  if (!hasData)
    tft.setTextColor(ST77XX_YELLOW);
  else if (alert)
    tft.setTextColor(ST77XX_RED);
  else
    tft.setTextColor(ST77XX_GREEN);

  tft.print(text);
}