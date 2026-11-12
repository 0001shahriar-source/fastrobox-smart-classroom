#ifndef RFID_HANDLER_H
#define RFID_HANDLER_H

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include "pins.h"
#include "door.h"

struct AttendanceRecord {
  char cardUid[32];
  char personType[16];   // "STUDENT" or "TEACHER"
  unsigned long timestamp;
};

class RfidManager {
private:
  MFRC522 mfrc522;
  char lastUid[32];
  unsigned long lastScanTime;
  AttendanceRecord lastRecord;

public:
  RfidManager() : mfrc522(RFID_SS_PIN, RFID_RST_PIN), lastScanTime(0) {
    memset(lastUid, 0, sizeof(lastUid));
  }

  void begin() {
    // Explicitly configure SPI pins for ESP32-S3 to avoid camera line conflicts
    SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SS_PIN);
    mfrc522.PCD_Init();
    delay(10);
    Serial.println(F("[RFID] RC522 Reader Initialized."));
  }

  const char* getLastUid() const {
    return lastUid;
  }

  const AttendanceRecord& getLastRecord() const {
    return lastRecord;
  }

  bool update(String& outUid, String& outRole) {
    if (!mfrc522.PICC_IsNewCardPresent()) {
      return false;
    }
    if (!mfrc522.PICC_ReadCardSerial()) {
      return false;
    }

    // Cooldown check (prevent repeated triggers within 1.5 seconds)
    if (millis() - lastScanTime < 1500) {
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      return false;
    }

    lastScanTime = millis();

    // Format UID as uppercase Hex String
    char uidBuf[32] = {0};
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      char byteStr[4];
      sprintf(byteStr, "%02X", mfrc522.uid.uidByte[i]);
      strcat(uidBuf, byteStr);
      if (i < mfrc522.uid.size - 1) strcat(uidBuf, ":");
    }

    strncpy(lastUid, uidBuf, sizeof(lastUid));
    outUid = String(uidBuf);

    // Differentiate between Student Card (White Card) and Teacher Key-Tag (Blue Tag)
    // Dynamic role logic: Cards ending in odd/even or specifically tagged
    bool isTeacher = false;
    if (strstr(uidBuf, "TEACHER") != NULL || mfrc522.uid.size == 7) {
      isTeacher = true;
    }

    // Default: Check-in action
    if (!isTeacher) {
      outRole = "STUDENT";
      strncpy(lastRecord.personType, "STUDENT", sizeof(lastRecord.personType));
      Serial.printf("[ATTENDANCE] Student Check-In! UID: %s | Door UNLOCKED\n", uidBuf);
      Door.unlock(4000); // Unlock for 4 seconds
    } else {
      outRole = "TEACHER";
      strncpy(lastRecord.personType, "TEACHER", sizeof(lastRecord.personType));
      Serial.printf("[TEACHER] Teacher Action! UID: %s | Override Toggle\n", uidBuf);
      if (Door.getState() == DOOR_LOCKED) {
        Door.unlock(0); // Permanent unlock until swiped again
      } else {
        Door.lock();
      }
    }

    strncpy(lastRecord.cardUid, uidBuf, sizeof(lastRecord.cardUid));
    lastRecord.timestamp = millis();

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return true;
  }
};

extern RfidManager Rfid;

#endif // RFID_HANDLER_H

