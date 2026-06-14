#pragma once
#include <Arduino.h>

void initDisplay();
void updateDisplay(bool hasData, bool alert);
void setBrightness(int brightness);