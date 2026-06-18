#pragma once
#include "config.h"

enum ScreenState
{
  SCREEN_MAIN,
  SCREEN_FORECAST
};

void switchScreen(ScreenState newScreen);
ScreenState getCurrentScreen();

void initDisplay();
void drawMainScreen(bool hasData, bool alert);
void drawForecastScreen(bool hasData, bool alert);
void setBrightness(int brightness);