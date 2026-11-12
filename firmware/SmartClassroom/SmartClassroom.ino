#include <Arduino.h>
#include <WiFi.h>

#include "pins.h"
#include "config.h"
#include "door.h"
#include "safety.h"
#include "rfid.h"
#include "serial_cli.h"
#include "web_local.h"
#include "cloud_sync.h"

// Global instances
ConfigManager Config;
SimulatedDoor Door;
SafetySystem Safety;
RfidManager Rfid;
SerialCli CLI;
LocalWebServer LocalWeb;
CloudSyncManager Cloud;

unsigned long lastWifiCheck = 0;
bool apModeActive = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println(F("\n========================================================"));
  Serial.println(F(" FASTROBOX 1.0 GRAND FINALE - SMART CLASSROOM SYSTEM    "));
  Serial.println(F(" Hardware: ESP32-S3-CAM | Zero Hardcoding Active        "));
  Serial.println(F("========================================================"));

  // 1. Initialize Configuration (Loads from non-volatile NVS storage)
  Config.begin();

  // 2. Initialize Actuators & Safety
  Door.begin();
  Safety.begin();

  // 3. Initialize RFID Reader
  Rfid.begin();

  // 4. Initialize Serial Command Line Interface
  CLI.begin();

  // 5. Connect Wi-Fi if credentials exist, otherwise start SoftAP
  if (strlen(Config.get().wifiSsid) > 0) {
    Serial.printf("[WIFI] Connecting to dynamically configured network: %s\n", Config.get().wifiSsid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(Config.get().wifiSsid, Config.get().wifiPass);
    
    // Non-blocking initial wait (max 4 seconds)
    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 4000) {
      delay(200);
      Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.print(F("[WIFI] Connected successfully! IP Address: "));
      Serial.println(WiFi.localIP());
      Door.beep(80, 2);
    } else {
      Serial.println(F("[WIFI] Could not connect. Starting Fallback Configuration SoftAP..."));
      WiFi.mode(WIFI_AP_STA);
      WiFi.softAP("SmartClassroom-Setup", "bubt1234");
      Serial.print(F("[WIFI] SoftAP Active: 'SmartClassroom-Setup' | IP: "));
      Serial.println(WiFi.softAPIP());
      apModeActive = true;
    }
  } else {
    Serial.println(F("[WIFI] No Wi-Fi configured. Starting SoftAP for setup..."));
    WiFi.mode(WIFI_AP);
    WiFi.softAP("SmartClassroom-Setup", "bubt1234");
    Serial.print(F("[WIFI] Connect to 'SmartClassroom-Setup' (Pass: bubt1234) at IP: "));
    Serial.println(WiFi.softAPIP());
    apModeActive = true;
  }

  // 6. Initialize Local Web Status Server & Cloud Client
  LocalWeb.begin();
  Cloud.begin();

  Serial.println(F("[SYSTEM] Initialization Complete. Ready for judging tests."));
  CLI.printStatus();
}

void loop() {
  // Update Simulated Door state machine and visual LED/buzzer animations
  Door.update();

  // Safety sensors: flame detection, capacitive touch panic, DHT11 readings
  bool prevFlame = Safety.isFlameDetected();
  bool prevPanic = Safety.isPanicTriggered();
  Safety.update();

  if (!prevFlame && Safety.isFlameDetected()) {
    Cloud.sendEvent("FLAME_ALERT", "Flame sensor detected fire! Emergency unlock.");
  }
  if (!prevPanic && Safety.isPanicTriggered()) {
    Cloud.sendEvent("PANIC_ALERT", "Touch panic wire activated by hand touch.");
  }

  // RFID scan update
  String scannedUid, role;
  if (Rfid.update(scannedUid, role)) {
    char detailBuf[64];
    snprintf(detailBuf, sizeof(detailBuf), "%s card: %s", role.c_str(), scannedUid.c_str());
    Cloud.sendEvent("ATTENDANCE", detailBuf);
  }

  // Handle Serial Commands (Instant on-the-spot test configuration)
  CLI.update();

  // Handle Local Web Server requests
  LocalWeb.update();

  // Handle Cloud Telemetry & Offline Event Flushing
  Cloud.update();

  // Periodic Wi-Fi reconnection monitor (non-blocking)
  if (millis() - lastWifiCheck > 10000) {
    lastWifiCheck = millis();
    if (strlen(Config.get().wifiSsid) > 0 && WiFi.status() != WL_CONNECTED && !apModeActive) {
      Serial.println(F("[WIFI] Auto-reconnecting..."));
      WiFi.reconnect();
    }
  }
}
