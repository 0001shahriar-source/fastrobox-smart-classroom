#ifndef SAFETY_H
#define SAFETY_H

#include <Arduino.h>
#include <DHT.h>
#include "pins.h"
#include "door.h"
#include "config.h"

class SafetySystem {
private:
  DHT dht;
  float temperature;
  float humidity;
  bool flameDetected;
  bool panicTriggered;
  unsigned long lastSensorRead;
  unsigned long lastFlameDebounce;
  int flameLowCount;

public:
  SafetySystem() : dht(DHT_PIN, DHT11), temperature(0.0), humidity(0.0), 
                   flameDetected(false), panicTriggered(false), lastSensorRead(0),
                   lastFlameDebounce(0), flameLowCount(0) {}

  void begin() {
    dht.begin();
    pinMode(FLAME_PIN, INPUT_PULLUP);
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  }

  float getTemperature() const { return temperature; }
  float getHumidity() const { return humidity; }
  bool isFlameDetected() const { return flameDetected; }
  bool isPanicTriggered() const { return panicTriggered; }

  void clearPanic() {
    panicTriggered = false;
    Door.setEmergency(false);
  }

  void update() {
    unsigned long now = millis();

    // 1. Instant Flame Detection (Digital Input Active LOW)
    // Digital debounce filter: 3 consecutive LOW samples to avoid optical flicker
    int rawFlame = digitalRead(FLAME_PIN);
    if (rawFlame == LOW) {
      flameLowCount++;
      if (flameLowCount >= 3 && !flameDetected) {
        flameDetected = true;
        Serial.println(F("[ALERT] FLAME DETECTED! Unlocking simulated door and triggering alarm!"));
        Door.setEmergency(true);
      }
    } else {
      flameLowCount = 0;
      if (flameDetected) {
        flameDetected = false;
        Serial.println(F("[RECOVERY] Flame cleared. Returning to normal state."));
        Door.setEmergency(false);
      }
    }

    // 2. Capacitive Touch Panic Wire (Single bare wire connected to GPIO 1)
    #if SOC_TOUCH_SENSOR_NUM > 0
    uint32_t touchVal = touchRead(TOUCH_PANIC_PIN);
    if (touchVal > 0 && touchVal < TOUCH_THRESHOLD) {
      if (!panicTriggered) {
        panicTriggered = true;
        Serial.println(F("[PANIC] Touch Panic Wire Activated! Classroom Alarm Triggered!"));
        Door.setVisual(VISUAL_PANIC_ALARM);
        Door.unlock(0, true);
      }
    }
    #endif

    // 3. Physical BOOT Button (Press to clear emergency / test reset)
    if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
      delay(50);
      if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
        if (panicTriggered || flameDetected) {
          clearPanic();
          Serial.println(F("[SYSTEM] Manual Emergency Reset via BOOT button."));
        } else {
          // Manual toggle door test
          if (Door.getState() == DOOR_LOCKED) {
            Door.unlock(3000);
          } else {
            Door.lock();
          }
        }
        while (digitalRead(BOOT_BUTTON_PIN) == LOW) delay(10);
      }
    }

    // 4. Periodic DHT11 Environment Sampling (Every 2 seconds)
    if (now - lastSensorRead >= 2000) {
      lastSensorRead = now;
      float t = dht.readTemperature();
      float h = dht.readHumidity();

      if (!isnan(t) && !isnan(h)) {
        temperature = t;
        humidity = h;

        // Check high temperature threshold
        if (temperature >= Config.get().tempThreshold && !flameDetected) {
          Serial.printf("[WARNING] High Temperature Alert: %.1f C (Threshold: %.1f C)\n", 
                        temperature, Config.get().tempThreshold);
        }
      }
    }
  }
};

extern SafetySystem Safety;

#endif // SAFETY_H

