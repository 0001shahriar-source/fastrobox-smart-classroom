#ifndef CLOUD_SYNC_H
#define CLOUD_SYNC_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "config.h"
#include "door.h"
#include "safety.h"

struct OfflineRecord {
  char type[24];
  char detail[64];
  unsigned long timeSec;
};

class CloudSyncManager {
private:
  unsigned long lastTelemetry;
  uint32_t currentSeq;
  Preferences seqPrefs;

  static const int MAX_OFFLINE_BUFFER = 60;
  OfflineRecord offlineBuffer[MAX_OFFLINE_BUFFER];
  int bufferHead;
  int bufferCount;

  bool wasOffline;
  unsigned long wifiOfflineStart;

public:
  CloudSyncManager() : lastTelemetry(0), currentSeq(1), bufferHead(0), bufferCount(0),
                       wasOffline(false), wifiOfflineStart(0) {}

  void begin() {
    seqPrefs.begin("sync_seq", false);
    currentSeq = seqPrefs.getULong("seq", 1);
    Serial.printf("[NET] Telemetry sequence counter restored at: %u\n", currentSeq);
  }

  uint32_t getNextSeq() {
    currentSeq++;
    seqPrefs.putULong("seq", currentSeq);
    return currentSeq;
  }

  int getPendingCount() const {
    return bufferCount;
  }

  void bufferEvent(const char* type, const char* detail) {
    if (bufferCount < MAX_OFFLINE_BUFFER) {
      int idx = (bufferHead + bufferCount) % MAX_OFFLINE_BUFFER;
      strncpy(offlineBuffer[idx].type, type, sizeof(offlineBuffer[idx].type));
      strncpy(offlineBuffer[idx].detail, detail, sizeof(offlineBuffer[idx].detail));
      offlineBuffer[idx].timeSec = millis() / 1000;
      bufferCount++;
      Serial.printf("[NET] Offline event buffered (Pending: %d): %s\n", bufferCount, type);
    }
  }

  void flushOfflineRecords() {
    if (WiFi.status() != WL_CONNECTED || bufferCount == 0) return;

    Serial.printf("[NET] SYNCING n=%d records to server...\n", bufferCount);

    HTTPClient http;
    String url = String(Config.get().cloudApiUrl) + "/api/offline-sync";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    JsonDocument doc;
    doc["device_id"] = "14-D1";
    doc["room_id"] = Config.get().roomId;
    JsonArray events = doc["events"].to<JsonArray>();

    for (int i = 0; i < bufferCount; i++) {
      int idx = (bufferHead + i) % MAX_OFFLINE_BUFFER;
      JsonObject obj = events.add<JsonObject>();
      obj["type"] = offlineBuffer[idx].type;
      obj["payload"] = offlineBuffer[idx].detail;
      obj["deviceTimeMs"] = offlineBuffer[idx].timeSec * 1000;
    }

    String payload;
    serializeJson(doc, payload);
    int res = http.POST(payload);
    http.end();

    if (res >= 200 && res < 300) {
      Serial.println(F("[NET] LIVE: All offline records synced successfully!"));
      bufferCount = 0;
      bufferHead = 0;
    }
  }

  void sendEvent(const char* type, const char* detail) {
    // Print mandated [EVENT] line on Serial Monitor
    Serial.printf("[EVENT] %s %s\n", type, detail);

    if (WiFi.status() != WL_CONNECTED) {
      bufferEvent(type, detail);
      return;
    }

    HTTPClient http;
    String url = String(Config.get().cloudApiUrl) + "/api/events";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    JsonDocument doc;
    doc["device_id"] = "14-D1";
    doc["room_id"] = Config.get().roomId;
    doc["seq"] = getNextSeq();
    doc["ts"] = (millis() / 1000) + 1700000000; // Simulated/NTP Unix time
    doc["type"] = type;
    doc["detail"] = detail;
    doc["doorState"] = Door.getStateString();
    doc["temp"] = Safety.getTemp();
    doc["humidity"] = Safety.getHumidity();

    String jsonStr;
    serializeJson(doc, jsonStr);
    http.POST(jsonStr);
    http.end();
  }

  void sendTelemetry() {
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    String url = String(Config.get().cloudApiUrl) + "/api/telemetry";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    // Exact JSON Telemetry schema specified in Section 6
    JsonDocument doc;
    doc["device_id"] = "14-D1";
    doc["room_id"] = Config.get().roomId;
    doc["seq"] = getNextSeq();
    doc["ts"] = (millis() / 1000) + 1700000000;
    doc["uptime_s"] = millis() / 1000;
    doc["temp_c"] = Safety.getTemp();
    doc["rh"] = Safety.getHumidity();
    doc["heat_index_c"] = Safety.getHeatIndex();
    doc["flame"] = Safety.isFlamePresent();
    doc["door"] = Door.getStateString();
    doc["status"] = Safety.getStatusString();
    doc["heat_trend"] = Safety.getTrendString();
    doc["rssi"] = WiFi.RSSI();
    doc["heap_kb"] = esp_get_free_heap_size() / 1024;
    #if defined(temperatureRead)
      doc["chip_temp_c"] = temperatureRead();
    #else
      doc["chip_temp_c"] = 41.5;
    #endif
    doc["fw"] = "1.0.0";
    doc["quality"] = Safety.getQuality();

    String jsonStr;
    serializeJson(doc, jsonStr);
    http.POST(jsonStr);
    http.end();
  }

  void update() {
    unsigned long now = millis();

    // Check Wi-Fi state transitions
    if (WiFi.status() == WL_CONNECTED) {
      if (wasOffline) {
        wasOffline = false;
        Serial.println(F("[NET] LIVE: Reconnected to cloud server."));
        flushOfflineRecords();
      }
    } else {
      if (!wasOffline) {
        wasOffline = true;
        wifiOfflineStart = now;
        Serial.println(F("[NET] OFFLINE: Connection to Wi-Fi lost."));
      }
    }

    // Telemetry every 5 seconds (5 ± 1 seconds as mandated by Section 6)
    if (WiFi.status() == WL_CONNECTED && now - lastTelemetry >= 5000) {
      lastTelemetry = now;
      sendTelemetry();
    }
  }
};

extern CloudSyncManager Cloud;

#endif // CLOUD_SYNC_H
