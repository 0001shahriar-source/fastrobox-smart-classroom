#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Preferences.h>

struct SystemConfig {
  char wifiSsid[64];
  char wifiPass[64];
  char roomId[32];
  char classStartTime[16];   // e.g. "09:00"
  char cloudApiUrl[128];     // Public HTTPS endpoint of web dashboard
  char apiKey[64];           // Dynamic API Token
  float tempThreshold;       // Temperature alarm threshold (°C)
  unsigned long doorDuration; // Door unlock duration in ms
  bool configured;
};

class ConfigManager {
private:
  Preferences prefs;
  SystemConfig currentConfig;

public:
  ConfigManager() {}

  void begin() {
    prefs.begin("smartclass", false);
    
    // Load persisted settings from NVS (No hardcoded credentials)
    String ssid = prefs.getString("ssid", "");
    String pass = prefs.getString("pass", "");
    String room = prefs.getString("room", "Room-101");
    String classTime = prefs.getString("classtime", "09:00");
    String cloudUrl = prefs.getString("cloudurl", "http://192.168.1.100:3000");
    String key = prefs.getString("apikey", "test-token");
    float tempThresh = prefs.getFloat("tempthresh", 45.0f);
    unsigned long doorDur = prefs.getULong("doordur", 5000);
    bool isConfigured = prefs.getBool("configured", false);

    strncpy(currentConfig.wifiSsid, ssid.c_str(), sizeof(currentConfig.wifiSsid));
    strncpy(currentConfig.wifiPass, pass.c_str(), sizeof(currentConfig.wifiPass));
    strncpy(currentConfig.roomId, room.c_str(), sizeof(currentConfig.roomId));
    strncpy(currentConfig.classStartTime, classTime.c_str(), sizeof(currentConfig.classStartTime));
    strncpy(currentConfig.cloudApiUrl, cloudUrl.c_str(), sizeof(currentConfig.cloudApiUrl));
    strncpy(currentConfig.apiKey, key.c_str(), sizeof(currentConfig.apiKey));
    currentConfig.tempThreshold = tempThresh;
    currentConfig.doorDuration = doorDur;
    currentConfig.configured = isConfigured;
  }

  SystemConfig& get() {
    return currentConfig;
  }

  void saveDoorDuration(unsigned long durationMs) {
    currentConfig.doorDuration = durationMs;
    prefs.putULong("doordur", durationMs);
  }

  void saveWifi(const char* ssid, const char* pass) {
    strncpy(currentConfig.wifiSsid, ssid, sizeof(currentConfig.wifiSsid));
    strncpy(currentConfig.wifiPass, pass, sizeof(currentConfig.wifiPass));
    currentConfig.configured = true;

    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.putBool("configured", true);
  }

  void saveRoom(const char* room) {
    strncpy(currentConfig.roomId, room, sizeof(currentConfig.roomId));
    prefs.putString("room", room);
  }

  void saveClassTime(const char* classTime) {
    strncpy(currentConfig.classStartTime, classTime, sizeof(currentConfig.classStartTime));
    prefs.putString("classtime", classTime);
  }

  void saveCloudUrl(const char* url) {
    strncpy(currentConfig.cloudApiUrl, url, sizeof(currentConfig.cloudApiUrl));
    prefs.putString("cloudurl", url);
  }

  void saveTempThreshold(float thresh) {
    currentConfig.tempThreshold = thresh;
    prefs.putFloat("tempthresh", thresh);
  }

  void resetDefaults() {
    prefs.clear();
    begin();
  }
};

extern ConfigManager Config;

#endif // CONFIG_H

