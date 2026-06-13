#include "config.h"
#include <Preferences.h>

Preferences prefs;

void loadConfig(AppConfig &cfg)
{
  prefs.begin("cfg", true);

  cfg.apiKey = prefs.getString("api", "");
  cfg.lat = prefs.getFloat("lat", 0);
  cfg.lon = prefs.getFloat("lon", 0);
  cfg.region = prefs.getString("region", "kyiv");

  cfg.tempOffset = prefs.getFloat("to", 0);
  cfg.humOffset = prefs.getFloat("ho", 0);

  prefs.end();
}

void saveConfig(const AppConfig &cfg)
{
  prefs.begin("cfg", false);

  prefs.putString("api", cfg.apiKey);
  prefs.putFloat("lat", cfg.lat);
  prefs.putFloat("lon", cfg.lon);
  prefs.putString("region", cfg.region);

  prefs.putFloat("to", cfg.tempOffset);
  prefs.putFloat("ho", cfg.humOffset);

  prefs.end();
}