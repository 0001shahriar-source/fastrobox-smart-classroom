#ifndef RFID_HANDLER_H
#define RFID_HANDLER_H

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Preferences.h>
#include "pins.h"
#include "door.h"
#include "device_signals.h"
#include "safety.h"

class RfidManager {
private:
  MFRC522 mfrc522;
  char lastUid[32];
  bool cardPresentPrev;
  unsigned long lastPollTime;
  String teacherTagUid;
  Preferences tagPrefs;

public:
  RfidManager() : mfrc522(RFID_SS_PIN, RFID_RST_PIN), cardPresentPrev(false), lastPollTime(0) {
    memset(lastUid, 0, sizeof(lastUid));
  }

  void begin() {
    SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SS_PIN);
    mfrc522.PCD_Init();
    delay(10);

    // Load registered Teacher Tag UID from NVS
    tagPrefs.begin("tags", false);
    teacherTagUid = tagPrefs.getString("teacher_tag", "BLUE_TAG");

    // Perform self-test / version check to detect unplugged sensor
    byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    if (v == 0x00 || v == 0xFF) {
      Serial.println(F("[ERROR] SENSOR ERROR: RC522"));
      Safety.reportRfidError();
    } else {
      Safety.markRfidActivity();
    }
  }

  const char* getLastUid() const {
    return lastUid;
  }

  void setTeacherTag(const String& uid) {
    teacherTagUid = uid;
    tagPrefs.putString("teacher_tag", uid);
    Serial.printf("[INFO] Registered Teacher Tag UID: %s\n", uid.c_str());
  }

  // Poll loop called every 200 ms
  bool update(String& outUid, String& outType) {
    unsigned long now = millis();
    if (now - lastPollTime < 200) {
      return false;
    }
    lastPollTime = now;

    // Check version register to ensure reader has not been unplugged
    byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    if (v == 0x00 || v == 0xFF) {
      Safety.reportRfidError();
      return false;
    } else {
      Safety.markRfidActivity();
    }

    if (!mfrc522.PICC_IsNewCardPresent()) {
      cardPresentPrev = false;
      return false;
    }
    if (!mfrc522.PICC_ReadCardSerial()) {
      return false;
    }

    // Report only once per tap, even if held continuously on the reader
    if (cardPresentPrev) {
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      return false;
    }
    cardPresentPrev = true;

    // Format UID as uppercase hex string
    char uidBuf[32] = {0};
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      char byteStr[4];
      sprintf(byteStr, "%02X", mfrc522.uid.uidByte[i]);
      strcat(uidBuf, byteStr);
      if (i < mfrc522.uid.size - 1) strcat(uidBuf, ":");
    }

    strncpy(lastUid, uidBuf, sizeof(lastUid));
    outUid = String(uidBuf);

    // Print mandated [CARD] tag on Serial Monitor
    Serial.printf("[CARD] uid=%s\n", uidBuf);

    // Identify Teacher Tag (Blue Key Tag) vs Student Card (White Card)
    // The kit blue key tag or specifically registered tag acts as Teacher Tag
    bool isTeacher = false;
    if (teacherTagUid.length() > 0 && teacherTagUid.equalsIgnoreCase(uidBuf)) {
      isTeacher = true;
    } else if (mfrc522.uid.size == 7 || strstr(uidBuf, "TEACHER") != NULL) {
      isTeacher = true;
    }

    if (isTeacher) {
      outType = "TEACHER";
      // If fire is latched, Teacher tag silences the buzzer only after flame has gone
      if (Safety.isFireLatched()) {
        Safety.silenceBuzzerByTeacher();
      } else {
        // Teacher door toggle
        if (Door.getState() == DOOR_LOCKED) {
          Door.unlock(0); // Permanent teacher unlock until swiped again
          Signals.triggerAccessGranted(3000);
          Serial.println(F("[EVENT] ENTRY_GRANTED Teacher Override"));
        } else {
          Door.lock();
          Serial.println(F("[EVENT] DOOR Relocked by Teacher"));
        }
      }
    } else {
      outType = "STUDENT";
      // Student card check-in
      if (Safety.isFireLatched()) {
        // During fire, door is already unlocked for evacuation
        Serial.println(F("[EVENT] ENTRY_GRANTED Fire Evacuation Active"));
      } else {
        Door.unlock(Config.get().doorDuration);
        Signals.triggerAccessGranted(Config.get().doorDuration);
        Serial.printf("[EVENT] ENTRY_GRANTED Student %s\n", uidBuf);
      }
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return true;
  }
};

extern RfidManager Rfid;

#endif // RFID_HANDLER_H
