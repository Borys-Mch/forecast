#pragma once
#include "config.h"

// WiFi
bool connectWiFi(const char *ssid, const char *pass);
void startAP();

// storage
void saveWiFi(String ssid, String pass);
void loadWiFi();

// shared data
extern String saved_ssid;
extern String saved_pass;