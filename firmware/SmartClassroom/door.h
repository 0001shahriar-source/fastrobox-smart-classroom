#ifndef DOOR_H
#define DOOR_H

#include <Arduino.h>
#include "pins.h"

enum DoorState {
  DOOR_LOCKED = 0,
  DOOR_UNLOCKED = 1
};

enum SystemStatusVisual {
  VISUAL_LOCKED,       // Solid Red
  VISUAL_UNLOCKED,     // Solid Green
  VISUAL_STANDBY,      // Solid Blue
  VISUAL_FIRE_ALARM,   // Flashing Red & Siren
  VISUAL_PANIC_ALARM   // Flashing Purple / Yellow
};

class SimulatedDoor {
private:
  DoorState state;
  unsigned long unlockTimestamp;
  unsigned long autoRelockDuration;
  bool emergencyOverride;
  SystemStatusVisual currentVisual;
  unsigned long lastBlink;
  bool blinkState;

  void setRgbColor(uint8_t r, uint8_t g, uint8_t b) {
    #ifdef RGB_BUILTIN
      neopixelWrite(RGB_BUILTIN, r, g, b);
    #endif
  }

public:
  SimulatedDoor() : state(DOOR_LOCKED), unlockTimestamp(0), autoRelockDuration(5000), 
                    emergencyOverride(false), currentVisual(VISUAL_LOCKED), lastBlink(0), blinkState(false) {}

  void begin() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    lock();
  }

  DoorState getState() const {
    return state;
  }

  const char* getStateString() const {
    return (state == DOOR_UNLOCKED) ? "UNLOCKED" : "LOCKED";
  }

  void beep(int durationMs, int times = 1) {
    for (int i = 0; i < times; i++) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(durationMs);
      digitalWrite(BUZZER_PIN, LOW);
      if (times > 1 && i < times - 1) delay(50);
    }
  }

  void unlock(unsigned long durationMs = 5000, bool emergency = false) {
    state = DOOR_UNLOCKED;
    unlockTimestamp = millis();
    autoRelockDuration = durationMs;
    emergencyOverride = emergency;

    if (!emergency) {
      currentVisual = VISUAL_UNLOCKED;
      setRgbColor(0, 64, 0); // Green
      beep(100, 1);          // 1 short pleasant beep
    } else {
      currentVisual = VISUAL_FIRE_ALARM;
    }
  }

  void lock() {
    if (emergencyOverride) return; // Cannot lock if emergency is active
    state = DOOR_LOCKED;
    currentVisual = VISUAL_LOCKED;
    setRgbColor(64, 0, 0); // Red
    beep(60, 2);           // 2 short beeps
  }

  void setEmergency(bool active) {
    emergencyOverride = active;
    if (active) {
      unlock(0, true); // Keep unlocked indefinitely
      currentVisual = VISUAL_FIRE_ALARM;
    } else {
      currentVisual = VISUAL_LOCKED;
      lock();
    }
  }

  void setVisual(SystemStatusVisual visual) {
    currentVisual = visual;
  }

  void update() {
    // Auto-relock check
    if (state == DOOR_UNLOCKED && !emergencyOverride && autoRelockDuration > 0) {
      if (millis() - unlockTimestamp >= autoRelockDuration) {
        lock();
      }
    }

    // Visual animation handler
    if (currentVisual == VISUAL_FIRE_ALARM) {
      if (millis() - lastBlink > 150) {
        lastBlink = millis();
        blinkState = !blinkState;
        if (blinkState) {
          setRgbColor(255, 0, 0); // Bright Red
          digitalWrite(BUZZER_PIN, HIGH);
        } else {
          setRgbColor(0, 0, 0);
          digitalWrite(BUZZER_PIN, LOW);
        }
      }
    } else if (currentVisual == VISUAL_PANIC_ALARM) {
      if (millis() - lastBlink > 200) {
        lastBlink = millis();
        blinkState = !blinkState;
        if (blinkState) {
          setRgbColor(255, 100, 0); // Orange
          digitalWrite(BUZZER_PIN, HIGH);
        } else {
          setRgbColor(0, 0, 0);
          digitalWrite(BUZZER_PIN, LOW);
        }
      }
    } else {
      // Normal solid colors
      if (state == DOOR_UNLOCKED) {
        setRgbColor(0, 64, 0); // Green
      } else {
        setRgbColor(64, 0, 0); // Red
      }
    }
  }
};

extern SimulatedDoor Door;

#endif // DOOR_H
