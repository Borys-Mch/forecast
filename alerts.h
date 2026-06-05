#pragma once
#include <Arduino.h>

bool isAlertNow(String region);
void updateAlert(String region);
bool getAlertState();
bool hasAlert();