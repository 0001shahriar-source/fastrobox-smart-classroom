#ifndef CLOUD_SYNC_H
#define CLOUD_SYNC_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "door.h"
#include "safety.h"

struct OfflineEvent {
  char eventType[24];     // "ATTENDANCE", "FLAME_ALERT", "PANIC", "DOOR"
  char payload[64];       // e.g. UID or detail
  unsigned long timestamp;
};

class CloudSyncManager {
private:
  unsigned long lastTelemetry;
  static const int MAX_BUFFER = 50;
  OfflineEvent offlineBuffer[MAX_BUFFER];
  int bufferHead;
  int bufferCount;

public:
  CloudSyncManager() : lastTelemetry(0), bufferHead(0), bufferCount(0) {}

  void begin() {
    Serial.println(F("[CLOUD] Cloud Synchronization Manager Initialized."));
  }

  void queueOfflineEvent(const char* type, const char* payload) {
    if (bufferCount < MAX_BUFFER) {
      int idx = (bufferHead + bufferCount) % MAX_BUFFER;
      strncpy(offlineBuffer[idx].eventType, type, sizeof(offlineBuffer[idx].eventType));
      strncpy(offlineBuffer[idx].payload, payload, sizeof(offlineBuffer[idx].payload));
      offlineBuffer[idx].timestamp = millis();
      bufferCount++;
      Serial.printf("[OFFLINE] Event buffered (Queue size: %d): %s\n", bufferCount, type);
    }
  }

  void flushOfflineBuffer() {
    if (WiFi.status() != WL_CONNECTED || bufferCount == 0) return;

    Serial.printf("[CLOUD] Flushing %d buffered offline events to cloud...\n", bufferCount);
    
    HTTPClient http;
    String url = String(Config.get().cloudApiUrl) + "/api/offline-sync";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    JsonDocument doc;
    doc["roomId"] = Config.get().roomId;
    JsonArray events = doc["events"].to<JsonArray>();

    for (int i = 0; i < bufferCount; i++) {
      int idx = (bufferHead + i) % MAX_BUFFER;
      JsonObject obj = events.add<JsonObject>();
      obj["type"] = offlineBuffer[idx].eventType;
      obj["payload"] = offlineBuffer[idx].payload;
      obj["deviceTimeMs"] = offlineBuffer[idx].timestamp;
    }

    String jsonStr;
    serializeJson(doc, jsonStr);
    int httpCode = http.POST(jsonStr);
    
    if (httpCode >= 200 && httpCode < 300) {
      Serial.println(F("[CLOUD] Offline events successfully synced!"));
      bufferCount = 0;
      bufferHead = 0;
    } else {
      Serial.printf("[CLOUD] Sync failed with HTTP code %d\n", httpCode);
    }
    http.end();
  }

  void sendEvent(const char* eventType, const char* detail) {
    if (WiFi.status() != WL_CONNECTED) {
      queueOfflineEvent(eventType, detail);
      return;
    }

    HTTPClient http;
    String url = String(Config.get().cloudApiUrl) + "/api/events";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    JsonDocument doc;
    doc["roomId"] = Config.get().roomId;
    doc["type"] = eventType;
    doc["detail"] = detail;
    doc["doorState"] = Door.getStateString();
    doc["temp"] = Safety.getTemperature();
    doc["humidity"] = Safety.getHumidity();
    doc["timestamp"] = millis();

    String jsonStr;
    serializeJson(doc, jsonStr);
    int httpCode = http.POST(jsonStr);
    http.end();
  }

  void update() {
    if (WiFi.status() != WL_CONNECTED) {
      return;
    }

    // If we have buffered offline events, flush them
    if (bufferCount > 0) {
      flushOfflineBuffer();
    }

    // Periodic telemetry every 4 seconds
    unsigned long now = millis();
    if (now - lastTelemetry >= 4000) {
      lastTelemetry = now;

      HTTPClient http;
      String url = String(Config.get().cloudApiUrl) + "/api/telemetry";
      http.begin(url);
      http.addHeader("Content-Type", "application/json");

      JsonDocument doc;
      doc["roomId"] = Config.get().roomId;
      doc["classTime"] = Config.get().classStartTime;
      doc["doorState"] = Door.getStateString();
      doc["temp"] = Safety.getTemperature();
      doc["humidity"] = Safety.getHumidity();
      doc["flame"] = Safety.isFlameDetected();
      doc["panic"] = Safety.isPanicTriggered();
      doc["uptime"] = millis() / 1000;

      String jsonStr;
      serializeJson(doc, jsonStr);
      http.POST(jsonStr);
      http.end();
    }
  }
};

extern CloudSyncManager Cloud;

#endif // CLOUD_SYNC_H

