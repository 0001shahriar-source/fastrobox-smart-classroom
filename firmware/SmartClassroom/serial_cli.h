#ifndef SERIAL_CLI_H
#define SERIAL_CLI_H

#include <Arduino.h>
#include "config.h"
#include "door.h"
#include "safety.h"
#include "rfid.h"

class SerialCli {
private:
  String inputBuffer;

public:
  SerialCli() {}

  void begin() {
    inputBuffer.reserve(128);
    Serial.println(F("=================================================="));
    Serial.println(F(" AI-Powered Smart Classroom System (FastRobox 1.0)"));
    Serial.println(F(" Type HELP for dynamic configuration commands    "));
    Serial.println(F("=================================================="));
  }

  void printStatus() {
    Serial.println(F("--- CURRENT SYSTEM STATUS ---"));
    Serial.printf("Room ID       : %s\n", Config.get().roomId);
    Serial.printf("Class Time    : %s\n", Config.get().classStartTime);
    Serial.printf("Wi-Fi SSID    : %s\n", Config.get().wifiSsid[0] ? Config.get().wifiSsid : "(Not configured)");
    Serial.printf("Door State    : %s\n", Door.getStateString());
    Serial.printf("Temperature   : %.1f C\n", Safety.getTemperature());
    Serial.printf("Humidity      : %.1f %%\n", Safety.getHumidity());
    Serial.printf("Flame Status  : %s\n", Safety.isFlameDetected() ? "FIRE ALARM!" : "SAFE");
    Serial.printf("Panic Status  : %s\n", Safety.isPanicTriggered() ? "PANIC ACTIVE" : "NORMAL");
    Serial.printf("Last RFID UID : %s\n", Rfid.getLastUid()[0] ? Rfid.getLastUid() : "None");
    Serial.printf("Cloud API URL : %s\n", Config.get().cloudApiUrl);
    Serial.println(F("-----------------------------"));
  }

  void printHelp() {
    Serial.println(F("Available Dynamic Commands (Zero Hardcoding):"));
    Serial.println(F("  SET_WIFI:<ssid>,<password> -> Set Wi-Fi and connect"));
    Serial.println(F("  SET_ROOM:<roomId>           -> Set Room ID (e.g. Room-204)"));
    Serial.println(F("  SET_TIME:<HH:MM>            -> Set Class Start Time (e.g. 10:30)"));
    Serial.println(F("  SET_CLOUD:<url>             -> Set Public HTTPS Web Platform URL"));
    Serial.println(F("  SET_TEMP:<threshold>        -> Set Temperature Threshold (e.g. 40.0)"));
    Serial.println(F("  SET_DOOR_TIME:<seconds>     -> Set Door Unlock Duration (e.g. 3 or 8)"));
    Serial.println(F("  UNLOCK                      -> Unlock simulated door"));
    Serial.println(F("  LOCK                        -> Lock simulated door"));
    Serial.println(F("  STATUS                      -> Display live system status"));
    Serial.println(F("  RESET                       -> Clear all saved configurations"));
  }

  void update() {
    while (Serial.available()) {
      char c = (char)Serial.read();
      if (c == '\n' || c == '\r') {
        inputBuffer.trim();
        if (inputBuffer.length() > 0) {
          processCommand(inputBuffer);
          inputBuffer = "";
        }
      } else {
        inputBuffer += c;
      }
    }
  }

  void processCommand(const String& cmd) {
    if (cmd.equalsIgnoreCase("HELP")) {
      printHelp();
    } else if (cmd.equalsIgnoreCase("STATUS")) {
      printStatus();
    } else if (cmd.equalsIgnoreCase("UNLOCK")) {
      Door.unlock(5000);
      Serial.println(F("[COMMAND] Door manually UNLOCKED for 5 seconds."));
    } else if (cmd.equalsIgnoreCase("LOCK")) {
      Door.lock();
      Serial.println(F("[COMMAND] Door manually LOCKED."));
    } else if (cmd.startsWith("SET_WIFI:")) {
      int commaIndex = cmd.indexOf(',', 9);
      if (commaIndex > 0) {
        String ssid = cmd.substring(9, commaIndex);
        String pass = cmd.substring(commaIndex + 1);
        Config.saveWifi(ssid.c_str(), pass.c_str());
        Serial.printf("[CONFIG] Wi-Fi set to SSID: '%s'. Reconnecting...\n", ssid.c_str());
        WiFi.disconnect();
        WiFi.begin(ssid.c_str(), pass.c_str());
      } else {
        Serial.println(F("[ERROR] Syntax: SET_WIFI:<ssid>,<password>"));
      }
    } else if (cmd.startsWith("SET_ROOM:")) {
      String room = cmd.substring(9);
      room.trim();
      Config.saveRoom(room.c_str());
      Serial.printf("[CONFIG] Room ID updated to: %s\n", room.c_str());
    } else if (cmd.startsWith("SET_TIME:")) {
      String timeStr = cmd.substring(9);
      timeStr.trim();
      Config.saveClassTime(timeStr.c_str());
      Serial.printf("[CONFIG] Class Start Time updated to: %s\n", timeStr.c_str());
    } else if (cmd.startsWith("SET_CLOUD:")) {
      String url = cmd.substring(10);
      url.trim();
      Config.saveCloudUrl(url.c_str());
      Serial.printf("[CONFIG] Cloud API URL updated to: %s\n", url.c_str());
    } else if (cmd.startsWith("SET_TEMP:")) {
      float thresh = cmd.substring(9).toFloat();
      Config.saveTempThreshold(thresh);
      Serial.printf("[CONFIG] Temperature threshold updated to: %.1f C\n", thresh);
    } else if (cmd.startsWith("SET_DOOR_TIME:")) {
      int sec = cmd.substring(14).toInt();
      if (sec > 0) {
        Config.saveDoorDuration(sec * 1000UL);
        Serial.printf("[CONFIG] Door auto-relock duration updated to: %d seconds\n", sec);
      }
    } else if (cmd.equalsIgnoreCase("RESET")) {
      Config.resetDefaults();
      Serial.println(F("[CONFIG] All configurations reset to defaults."));
    } else {
      Serial.println(F("[UNKNOWN COMMAND] Type HELP for list of valid commands."));
    }
  }
};

extern SerialCli CLI;

#endif // SERIAL_CLI_H

