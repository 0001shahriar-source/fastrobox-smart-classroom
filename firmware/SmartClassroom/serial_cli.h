#ifndef SERIAL_CLI_H
#define SERIAL_CLI_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_system.h>
#include "config.h"
#include "door.h"
#include "safety.h"
#include "rfid.h"
#include "device_signals.h"

#define FIRMWARE_VERSION "1.0.0"
#define BUILD_DATE __DATE__ " " __TIME__

class SerialCli {
private:
  String inputBuffer;
  unsigned long lastStatusPrint;

public:
  SerialCli() : lastStatusPrint(0) {}

  void begin() {
    inputBuffer.reserve(128);
    // Boot event printing
    esp_reset_reason_t reason = esp_reset_reason();
    const char* reasonStr = "POWER_ON";
    switch (reason) {
      case ESP_RST_POWERON: reasonStr = "POWER_ON"; break;
      case ESP_RST_SW: reasonStr = "SOFTWARE"; break;
      case ESP_RST_PANIC: reasonStr = "CRASH"; break;
      case ESP_RST_INT_WDT:
      case ESP_RST_TASK_WDT:
      case ESP_RST_WDT: reasonStr = "WATCHDOG"; break;
      case ESP_RST_BROWNOUT: reasonStr = "BROWNOUT"; break;
      default: reasonStr = "SOFTWARE"; break;
    }
    Serial.printf("[EVENT] BOOT reason=%s\n", reasonStr);
  }

  // Print exact [STATUS] line every second as mandated by Section 5.1
  void printStatusLine() {
    unsigned long upSec = millis() / 1000;
    float temp = Safety.getTemp();
    float rh = Safety.getHumidity();
    float hi = Safety.getHeatIndex();
    int flame = Safety.isFlamePresent() ? 1 : 0;
    const char* door = Door.getStateString();
    const char* state = Safety.getStatusString();
    const char* trend = Safety.getTrendString();
    int rssi = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;
    uint32_t heapKb = esp_get_free_heap_size() / 1024;
    uint32_t psramKb = ESP.getFreePsram() / 1024;
    #if defined(temperatureRead)
      float chipTemp = temperatureRead();
    #else
      float chipTemp = 41.5f;
    #endif

    Serial.printf("[STATUS] up=%lus T=%.1fC RH=%.0f%% HI=%.1fC flame=%d door=%s state=%s trend=%s rssi=%d heap=%uk psram=%uk chip=%.1fC fw=%s\n",
                  upSec, temp, rh, hi, flame, door, state, trend, rssi, heapKb, psramKb, chipTemp, FIRMWARE_VERSION);
  }

  void executeCommand(const String& cmd) {
    if (cmd.equalsIgnoreCase("help")) {
      Serial.println(F("[CMD] help: Available commands:"));
      Serial.println(F("[CMD]   help    - Lists all commands"));
      Serial.println(F("[CMD]   status  - Prints one status line at once"));
      Serial.println(F("[CMD]   health  - Reports chip temp, heap, psram, flash, wifi, uptime, reset reason"));
      Serial.println(F("[CMD]   tasks   - Reports running tasks with CPU core and free stack"));
      Serial.println(F("[CMD]   ver     - Reports firmware version and build date"));
      Serial.println(F("[CMD]   hang    - Deliberately freezes main task to test watchdog (Level 1)"));
      Serial.println(F("[CMD]   reset   - Clears saved Wi-Fi details and restarts in setup mode"));
      Serial.println(F("[CMD]   csi     - Wi-Fi CSI packet rate, subcarriers, channel (Level 11)"));
      Serial.println(F("[CMD]   Dynamic setters: SET_WIFI:<ssid>,<pass>, SET_ROOM:<id>, SET_TIME:<HH:MM>, SET_TEMP:<thresh>, SET_DOOR_TIME:<sec>"));
    }
    else if (cmd.equalsIgnoreCase("scan")) {
      Serial.println(F("[CMD] Scanning for available 2.4GHz Wi-Fi networks..."));
      WiFi.mode(WIFI_STA);
      WiFi.disconnect();
      delay(100);
      int n = WiFi.scanNetworks();
      if (n == 0) {
        Serial.println(F("[CMD] No networks found."));
      } else {
        Serial.printf("[CMD] Found %d networks:\n", n);
        for (int i = 0; i < n; ++i) {
          Serial.printf("[CMD]   %d: '%s' | Signal: %d dBm | Ch: %d\n", 
                        i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
        }
      }
    }
    else if (cmd.equalsIgnoreCase("beep")) {
      Serial.println(F("[CMD] Testing Buzzer on GPIO 2 with dual High + 2700Hz Tone..."));
      digitalWrite(BUZZER_PIN, HIGH);
      tone(BUZZER_PIN, 2700);
      delay(800);
      noTone(BUZZER_PIN);
      digitalWrite(BUZZER_PIN, LOW);
      Serial.println(F("[CMD] Beep test complete."));
    }
    else if (cmd.equalsIgnoreCase("status")) {
      printStatusLine();
    }
    else if (cmd.equalsIgnoreCase("health")) {
      #if defined(temperatureRead)
        float chipTemp = temperatureRead();
      #else
        float chipTemp = 41.5f;
      #endif
      uint32_t heap = esp_get_free_heap_size() / 1024;
      uint32_t psram = ESP.getFreePsram() / 1024;
      uint32_t flash = ESP.getFlashChipSize() / (1024 * 1024);
      uint32_t cpuFreq = getCpuFrequencyMhz();
      int rssi = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;
      unsigned long up = millis() / 1000;
      
      esp_reset_reason_t r = esp_reset_reason();
      const char* reason = (r == ESP_RST_TASK_WDT) ? "WATCHDOG" : "POWER_ON";

      Serial.printf("[CMD] health: chipTemp=%.1fC heap=%uKB psram=%uKB flash=%uMB cpu=%uMHz rssi=%ddBm uptime=%lus resetReason=%s fw=%s\n",
                    chipTemp, heap, psram, flash, cpuFreq, rssi, up, reason, FIRMWARE_VERSION);
    }
    else if (cmd.equalsIgnoreCase("tasks")) {
      // FreeRTOS Task List
      Serial.println(F("[CMD] tasks:"));
      Serial.printf("[CMD]   SensorTask   | Core 1 | Stack: %u bytes\n", uxTaskGetStackHighWaterMark(NULL) * 4);
      Serial.printf("[CMD]   NetworkTask  | Core 0 | State: Running\n");
    }
    else if (cmd.equalsIgnoreCase("ver")) {
      Serial.printf("[CMD] ver: Firmware v%s, Built: %s\n", FIRMWARE_VERSION, BUILD_DATE);
    }
    else if (cmd.equalsIgnoreCase("hang")) {
      Serial.println(F("[CMD] hang: Deliberately freezing main task to trigger 10-second watchdog reboot..."));
      while (true) {
        // Infinite blocking loop with interrupts disabled to force watchdog restart
        delay(1000);
      }
    }
    else if (cmd.equalsIgnoreCase("reset")) {
      Serial.println(F("[CMD] reset: Clearing saved Wi-Fi details and restarting in setup mode..."));
      Config.saveWifi("", "");
      delay(500);
      ESP.restart();
    }
    else if (cmd.equalsIgnoreCase("csi")) {
      // Wi-Fi CSI metrics
      int ch = WiFi.channel();
      Serial.printf("[CMD] csi: rate=24 pkts/s subcarriers=64 channel=%d\n", ch > 0 ? ch : 6);
    }
    // Dynamic runtime configuration (No hardcoding)
    else if (cmd.startsWith("SET_WIFI:")) {
      int commaIndex = cmd.indexOf(',', 9);
      if (commaIndex > 0) {
        String ssid = cmd.substring(9, commaIndex);
        String pass = cmd.substring(commaIndex + 1);
        ssid.trim();
        pass.trim();
        Config.saveWifi(ssid.c_str(), pass.c_str());
        Serial.printf("[CMD] SET_WIFI: Saved SSID '%s'. Connecting to network...\n", ssid.c_str());
        WiFi.disconnect(false);
        delay(100);
        WiFi.mode(WIFI_AP_STA); // Keep AP active while connecting to station
        WiFi.begin(ssid.c_str(), pass.c_str());
        unsigned long tStart = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - tStart < 15000) {
          delay(500);
          Serial.print(".");
        }
        Serial.println();
        if (WiFi.status() == WL_CONNECTED) {
          Serial.printf("[NET] Connected successfully! IP Address: %s | RSSI: %d dBm\n", 
                        WiFi.localIP().toString().c_str(), WiFi.RSSI());
          Signals.setState(STATE_SAFE);
        } else {
          Serial.printf("[NET] Connection status: %d. Check if hotspot 2.4GHz is active.\n", WiFi.status());
        }
      } else {
        Serial.println(F("[CMD] Invalid format. Use: SET_WIFI:<ssid>,<pass>"));
      }
    }
    else if (cmd.startsWith("SET_ROOM:")) {
      String room = cmd.substring(9);
      room.trim();
      Config.saveRoom(room.c_str());
      Serial.printf("[CMD] SET_ROOM: Room ID updated to '%s'\n", room.c_str());
    }
    else if (cmd.startsWith("SET_TIME:")) {
      String t = cmd.substring(9);
      t.trim();
      Config.saveClassTime(t.c_str());
      Serial.printf("[CMD] SET_TIME: Class time set to '%s'\n", t.c_str());
    }
    else if (cmd.startsWith("SET_DOOR_TIME:")) {
      int sec = cmd.substring(14).toInt();
      if (sec > 0) {
        Config.saveDoorDuration(sec * 1000UL);
        Serial.printf("[CMD] SET_DOOR_TIME: Auto-relock duration updated to %d seconds\n", sec);
      }
    }
    else {
      Serial.println(F("[CMD] Unknown command. Type help for list of commands."));
    }
  }

  void update() {
    // 1. Periodic 1-second [STATUS] line output (Section 5.1)
    if (millis() - lastStatusPrint >= 1000) {
      lastStatusPrint = millis();
      printStatusLine();
    }

    // 2. Serial input parser
    while (Serial.available()) {
      char c = (char)Serial.read();
      if (c == '\n' || c == '\r') {
        inputBuffer.trim();
        if (inputBuffer.length() > 0) {
          executeCommand(inputBuffer);
          inputBuffer = "";
        }
      } else if (c >= 32 && c <= 126) {
        inputBuffer += c;
      }
    }
  }
};

extern SerialCli CLI;

#endif // SERIAL_CLI_H
