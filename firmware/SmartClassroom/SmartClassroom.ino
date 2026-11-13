#include <Arduino.h>
#include <WiFi.h>

#include "pins.h"
#include "config.h"
#include "device_signals.h"
#include "door.h"
#include "safety.h"
#include "rfid.h"
#include "serial_cli.h"
#include "web_local.h"
#include "cloud_sync.h"

// Global instances
ConfigManager Config;
DeviceSignals Signals;
SimulatedDoor Door;
SafetySystem Safety;
RfidManager Rfid;
SerialCli CLI;
LocalWebServer LocalWeb;
CloudSyncManager Cloud;

// Task handles for Dual-Core FreeRTOS execution
TaskHandle_t sensorTaskHandle = NULL;
TaskHandle_t networkTaskHandle = NULL;

unsigned long bootButtonDownTime = 0;
bool bootButtonPressed = false;

// -------------------------------------------------------------
// Core 1 Task: Sensor Sampling, Fast Safety & Signal Actuation
// -------------------------------------------------------------
void sensorTask(void* pvParameters) {
  unsigned long lastDhtTime = 0;

  for (;;) {
    // 1. Fast safety poll (Flame 100ms, Touch, Sensor unplugged watchdog)
    Safety.updateFast();

    // 2. Periodic DHT11 read every 5 seconds (Level 1 Task 3)
    if (millis() - lastDhtTime >= 5000) {
      lastDhtTime = millis();
      Safety.updateDht();
    }

    // 3. RFID reader poll every 200 ms (Level 1 Task 3)
    String scannedUid, scannedType;
    if (Rfid.update(scannedUid, scannedType)) {
      char detailBuf[64];
      snprintf(detailBuf, sizeof(detailBuf), "%s card: %s", scannedType.c_str(), scannedUid.c_str());
      Cloud.sendEvent("CARD_TAP", detailBuf);
    }

    // 4. Update Door FSM and 2Hz LED/Buzzer State Signals
    Door.update();
    Signals.update();

    // 5. BOOT button 5-second hold detector (Reset Wi-Fi credentials)
    if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
      if (!bootButtonPressed) {
        bootButtonPressed = true;
        bootButtonDownTime = millis();
      } else if (millis() - bootButtonDownTime >= 5000) {
        Serial.println(F("[EVENT] BOOT button held for 5 seconds! Clearing saved details..."));
        Config.saveWifi("", "");
        Signals.triggerAccessDenied();
        delay(500);
        ESP.restart();
      }
    } else {
      bootButtonPressed = false;
    }

    // Yield to FreeRTOS (run at ~20Hz loop on Core 1)
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// -------------------------------------------------------------
// Core 0 Task: Wi-Fi, Local Web Server & Cloud Sync
// -------------------------------------------------------------
void networkTask(void* pvParameters) {
  for (;;) {
    // 1. Process Local HTTP Status Web Server
    LocalWeb.update();

    // 2. Process Cloud Telemetry & Offline event flushing
    Cloud.update();

    // Yield to FreeRTOS (Core 0 background tasks)
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println(F("\n========================================================"));
  Serial.println(F(" FASTROBOX 1.0 GRAND FINALE - AI SMART CLASSROOM SYSTEM "));
  Serial.println(F(" Dual-Core FreeRTOS Active | Hardware: ESP32-S3-CAM     "));
  Serial.println(F("========================================================"));

  // 1. Start Serial CLI & print BOOT reset reason
  CLI.begin();

  // 2. Initialize Configuration & Signals
  Config.begin();
  Signals.begin();
  Door.begin();

  // 3. Start-up Check: Flame sensor first (Level 1 Task 6)
  pinMode(FLAME_PIN, INPUT_PULLUP);
  delay(50);
  if (digitalRead(FLAME_PIN) == LOW) {
    Serial.println(F("[ALERT] Flame detected at start-up! Setting door UNLOCKED."));
    Door.unlock(0, true);
    Signals.setState(STATE_FIRE);
  } else {
    Serial.println(F("[INFO] No flame detected at start-up. Door securely LOCKED."));
    Door.lock();
    Signals.setState(STATE_SAFE);
  }

  // 4. Initialize Safety & RFID modules
  Safety.begin();
  Rfid.begin();

  // 5. Network Setup: Wi-Fi STA or Fallback Setup Access Point
  if (strlen(Config.get().wifiSsid) > 0) {
    Serial.printf("[NET] Connecting to Wi-Fi SSID: '%s'...\n", Config.get().wifiSsid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(Config.get().wifiSsid, Config.get().wifiPass);

    unsigned long startTry = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTry < 5000) {
      delay(200);
      Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.print(F("[NET] Connected successfully! IP Address: "));
      Serial.println(WiFi.localIP());
      Signals.setState(STATE_SAFE);
    } else {
      Serial.println(F("[NET] Wi-Fi connection timed out. Starting Setup Access Point..."));
      WiFi.mode(WIFI_AP_STA);
      // Setup AP name: SmartClass-<TeamID> with 8-char password printed at boot (Level 2 Task 1)
      const char* apPass = "sc889900";
      WiFi.softAP("SmartClass-14", apPass);
      Serial.printf("[NET] Access Point started: 'SmartClass-14' | Password: %s | IP: 192.168.4.1\n", apPass);
      Signals.setState(STATE_WIFI_SETUP_OR_OFFLINE);
    }
  } else {
    WiFi.mode(WIFI_AP);
    const char* apPass = "sc889900";
    WiFi.softAP("SmartClass-14", apPass);
    Serial.printf("[NET] No saved network. Access Point started: 'SmartClass-14' | Password: %s | Setup page at 192.168.4.1\n", apPass);
    Signals.setState(STATE_WIFI_SETUP_OR_OFFLINE);
  }

  // 6. Start Local Web Server & Cloud Sync Manager
  LocalWeb.begin();
  Cloud.begin();

  // 7. Launch FreeRTOS Tasks on Dual Cores (Core 0 and Core 1)
  xTaskCreatePinnedToCore(
    sensorTask,         // Task function
    "SensorTask",       // Name
    4096,               // Stack size
    NULL,               // Parameters
    2,                  // Priority
    &sensorTaskHandle,  // Handle
    1                   // Pin to Core 1
  );

  xTaskCreatePinnedToCore(
    networkTask,        // Task function
    "NetworkTask",      // Name
    8192,               // Stack size
    NULL,               // Parameters
    1,                  // Priority
    &networkTaskHandle, // Handle
    0                   // Pin to Core 0
  );

  Serial.println(F("[SYSTEM] Dual-Core FreeRTOS tasks started. System fully armed."));
}

void loop() {
  // Process Serial Monitor CLI input and periodic status in main loopTask
  CLI.update();
  delay(10);
}
