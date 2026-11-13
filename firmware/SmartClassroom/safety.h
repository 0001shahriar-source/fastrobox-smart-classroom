#ifndef SAFETY_H
#define SAFETY_H

#include <Arduino.h>
#include <DHT.h>
#include <Preferences.h>
#include "pins.h"
#include "device_signals.h"
#include "config.h"

enum SafetyState {
  STATUS_SAFE,
  STATUS_WARNING,
  STATUS_DANGER,
  STATUS_FIRE
};

enum HeatTrend {
  TREND_STABLE,
  TREND_RISING,
  TREND_FALLING
};

class SafetySystem {
private:
  DHT dht;
  float temperature;
  float humidity;
  float heatIndex;
  String dataQuality; // "good" or "bad"
  HeatTrend heatTrend;

  // History for trend calculation (last 10 readings)
  float tempHistory[10];
  int historyCount;
  int historyIdx;

  // Status state machine with 3-second hold filter
  SafetyState currentStatus;
  SafetyState candidateStatus;
  unsigned long candidateStatusStartTime;

  // Flame detection with 2-second continuous hold
  bool rawFlamePresent;
  unsigned long flameContinuousStartTime;
  bool fireLatched;
  bool buzzerSilenced;

  // Touch panic pad with baseline calibration and 2-second hold
  uint32_t touchBaseline;
  unsigned long touchStartTime;
  bool touchHolding;
  bool panicActive;
  unsigned long panicStartTime;

  // Sensor unplugged / failure monitors
  unsigned long lastValidDhtTime;
  unsigned long lastValidRfidTime;
  bool dhtError;
  bool rfidError;

  Preferences firePrefs;

  // NOAA Heat Index formula in Celsius
  float computeHeatIndexC(float T, float RH) {
    // Convert Celsius to Fahrenheit
    float tf = T * 1.8f + 32.0f;
    float hi_f = 0.5f * (tf + 61.0f + ((tf - 68.0f) * 1.2f) + (RH * 0.094f));
    if (hi_f >= 80.0f) {
      hi_f = -42.379f + 2.04901523f * tf + 10.14333127f * RH
             - 0.22475541f * tf * RH - 0.00683783f * tf * tf
             - 0.05481717f * RH * RH + 0.00122874f * tf * tf * RH
             + 0.00085282f * tf * RH * RH - 0.00000199f * tf * tf * RH * RH;
      if (RH < 13.0f && tf >= 80.0f && tf <= 112.0f) {
        hi_f -= ((13.0f - RH) / 4.0f) * sqrt((17.0f - abs(tf - 95.0f)) / 17.0f);
      } else if (RH > 85.0f && tf >= 80.0f && tf <= 87.0f) {
        hi_f += ((RH - 85.0f) / 10.0f) * ((87.0f - tf) / 5.0f);
      }
    }
    // Convert Fahrenheit back to Celsius
    return (hi_f - 32.0f) / 1.8f;
  }

  void updateHeatTrend() {
    if (historyCount < 10) return;
    // Compare average of oldest 3 with newest 3
    float oldestSum = 0, newestSum = 0;
    for (int i = 0; i < 3; i++) {
      int oldIdx = (historyIdx + i) % 10;
      int newIdx = (historyIdx + 7 + i) % 10;
      oldestSum += tempHistory[oldIdx];
      newestSum += tempHistory[newIdx];
    }
    float diff = (newestSum / 3.0f) - (oldestSum / 3.0f);
    if (diff > 0.5f) {
      heatTrend = TREND_RISING;
    } else if (diff < -0.5f) {
      heatTrend = TREND_FALLING;
    } else {
      heatTrend = TREND_STABLE;
    }
  }

public:
  uint32_t lastTouchVal;

  SafetySystem() : dht(DHT_PIN, DHT11), temperature(25.0f), humidity(50.0f), 
                   heatIndex(25.0f), dataQuality("good"), heatTrend(TREND_STABLE),
                   historyCount(0), historyIdx(0), currentStatus(STATUS_SAFE),
                   candidateStatus(STATUS_SAFE), candidateStatusStartTime(0),
                   rawFlamePresent(false), flameContinuousStartTime(0), fireLatched(false),
                   buzzerSilenced(false), touchBaseline(0), touchStartTime(0),
                   touchHolding(false), panicActive(false), panicStartTime(0),
                   lastValidDhtTime(0), lastValidRfidTime(0), dhtError(false), rfidError(false),
                   lastTouchVal(0) {}

  void begin() {
    dht.begin();
    pinMode(FLAME_PIN, INPUT_PULLUP);
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

    // 1. Calibrate Touch Baseline (Average of 10 samples)
    uint32_t sum = 0;
    for (int i = 0; i < 10; i++) {
      #if SOC_TOUCH_SENSOR_NUM > 0
        sum += touchRead(TOUCH_PANIC_PIN);
      #else
        sum += 50000;
      #endif
      delay(20);
    }
    touchBaseline = sum / 10;
    Serial.printf("[STATUS] Touch-pad baseline calibrated at start-up: %u\n", touchBaseline);

    // 2. Load Fire Latch from NVS (Survives power cuts!)
    firePrefs.begin("fire_latch", false);
    fireLatched = firePrefs.getBool("latched", false);
    if (fireLatched) {
      Serial.println(F("[ALERT] Board restarted with FIRE latch active! Keeping door UNLOCKED."));
      Signals.setState(STATE_FIRE);
    }

    lastValidDhtTime = millis();
    lastValidRfidTime = millis();
  }

  bool isFireLatched() const { return fireLatched; }
  bool isPanicActive() const { return panicActive; }
  bool isFlamePresent() const { return rawFlamePresent; }
  float getTemp() const { return temperature; }
  float getHumidity() const { return humidity; }
  float getHeatIndex() const { return heatIndex; }
  const char* getQuality() const { return dataQuality.c_str(); }
  const char* getTrendString() const {
    switch (heatTrend) {
      case TREND_RISING: return "RISING";
      case TREND_FALLING: return "FALLING";
      default: return "STABLE";
    }
  }

  const char* getStatusString() const {
    if (fireLatched) return "FIRE";
    if (currentStatus == STATUS_DANGER) return "DANGER";
    if (currentStatus == STATUS_WARNING) return "WARNING";
    return "SAFE";
  }

  void markRfidActivity() {
    lastValidRfidTime = millis();
    if (rfidError) {
      rfidError = false;
      Serial.println(F("[INFO] RC522 Sensor Reconnected and healthy."));
      if (!dhtError && !fireLatched && !panicActive) {
        Signals.setState(STATE_SAFE);
      }
    }
  }

  void reportRfidError() {
    if (!rfidError) {
      rfidError = true;
      Serial.println(F("[ERROR] SENSOR ERROR: RC522"));
      Signals.setState(STATE_SENSOR_ERROR);
    }
  }

  // Teacher key-tag silences buzzer ONLY after flame has cleared
  bool silenceBuzzerByTeacher() {
    if (rawFlamePresent) {
      Serial.println(F("[ERROR] CANNOT SILENCE - FLAME PRESENT"));
      return false;
    }
    buzzerSilenced = true;
    Serial.println(F("[EVENT] SILENCED by Teacher Tag"));
    return true;
  }

  // Reset Fire (Admin/Teacher only)
  void resetFire() {
    fireLatched = false;
    buzzerSilenced = false;
    firePrefs.putBool("latched", false);
    Serial.println(F("[EVENT] FIRE_CLEARED"));
    Signals.setState(STATE_SAFE);
  }

  void resetPanic() {
    panicActive = false;
    Serial.println(F("[EVENT] PANIC_CLEARED"));
    Signals.setState(STATE_SAFE);
  }

  // Fast sampling loop (called every 50-100ms)
  void updateFast() {
    unsigned long now = millis();

    // --- 1. Continuous Flame Detection (Active LOW on DO) ---
    // A flame seen continuously for 2 seconds is FIRE at once (no 3-second hold)
    if (digitalRead(FLAME_PIN) == LOW) {
      rawFlamePresent = true;
      if (flameContinuousStartTime == 0) {
        flameContinuousStartTime = now;
      } else if (now - flameContinuousStartTime >= 2000 && !fireLatched) {
        fireLatched = true;
        firePrefs.putBool("latched", true);
        Serial.println(F("[EVENT] FIRE: Flame verified continuously for 2s! Emergency Unlock!"));
        Signals.setState(STATE_FIRE);
      }
    } else {
      rawFlamePresent = false;
      flameContinuousStartTime = 0;
    }

    // --- 2. Capacitive Touch Panic Pad ---
    #if SOC_TOUCH_SENSOR_NUM > 0
    lastTouchVal = touchRead(TOUCH_PANIC_PIN);
    // Baseline is ~17,460. Real finger touch drops below 12,000.
    bool touchDown = (lastTouchVal > 0 && lastTouchVal < 12000); 
    #else
    lastTouchVal = 99999;
    bool touchDown = false;
    #endif

    if (touchDown) {
      if (!touchHolding) {
        touchHolding = true;
        touchStartTime = now;
      } else if (now - touchStartTime >= 2000 && !panicActive) {
        panicActive = true;
        panicStartTime = now;
        Serial.println(F("[EVENT] PANIC: Touch panic pad held for 2s! Emergency Alarm!"));
        Signals.setState(STATE_PANIC);
      }
    } else {
      touchHolding = false;
      touchStartTime = 0;
    }

    // Auto-clear panic state after 10s buzzer pattern completes
    if (panicActive && now - panicStartTime >= 10000) {
      panicActive = false;
      if (!fireLatched && !dhtError && !rfidError) {
        Signals.setState(STATE_SAFE);
      }
    }

    // --- 3. Sensor Unplugged Watchdogs ---
    // RFID timeout: within 3 seconds of being unplugged
    if (now - lastValidRfidTime >= 3000 && !rfidError) {
      reportRfidError();
    }
    // DHT11 timeout: within 12 seconds of failure
    if (now - lastValidDhtTime >= 12000 && !dhtError) {
      dhtError = true;
      dataQuality = "bad";
      Serial.println(F("[ERROR] SENSOR ERROR: DHT11"));
      Signals.setState(STATE_SENSOR_ERROR);
    }
  }

  // Periodic DHT11 Sampling (Every 5 seconds as specified in Level 1 task 3)
  void updateDht() {
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h) || t < 0.0f || t > 50.0f || h < 20.0f || h > 90.0f) {
      // Failed or impossible reading sets quality to bad and is never used
      dataQuality = "bad";
      return;
    }

    // Healthy reading
    dataQuality = "good";
    temperature = t;
    humidity = h;
    heatIndex = computeHeatIndexC(t, h);
    lastValidDhtTime = millis();
    if (dhtError) {
      dhtError = false;
      Serial.println(F("[INFO] DHT11 Sensor Reconnected and healthy."));
      if (!rfidError && !fireLatched && !panicActive) Signals.setState(STATE_SAFE);
    }

    // Append to history and update trend
    tempHistory[historyIdx] = temperature;
    historyIdx = (historyIdx + 1) % 10;
    if (historyCount < 10) historyCount++;
    updateHeatTrend();

    // Evaluate Heat Status with Hysteresis
    SafetyState target = currentStatus;
    // Enter Danger at 40 C or above; Leave at 38 C (2 C below limit)
    if (currentStatus == STATUS_DANGER) {
      if (temperature <= 38.0f) {
        target = (temperature >= 32.0f || humidity >= 85.0f) ? STATUS_WARNING : STATUS_SAFE;
      }
    }
    // Enter Warning at 32 C or above, or 85% humidity; Leave at 30 C and 80% humidity
    else if (currentStatus == STATUS_WARNING) {
      if (temperature >= 40.0f) {
        target = STATUS_DANGER;
      } else if (temperature <= 30.0f && humidity <= 80.0f) {
        target = STATUS_SAFE;
      }
    }
    else { // STATUS_SAFE
      if (temperature >= 40.0f) {
        target = STATUS_DANGER;
      } else if (temperature >= 32.0f || humidity >= 85.0f) {
        target = STATUS_WARNING;
      }
    }

    // State change filter: counts only after holding for 3 seconds
    if (target != currentStatus) {
      if (candidateStatus != target) {
        candidateStatus = target;
        candidateStatusStartTime = millis();
      } else if (millis() - candidateStatusStartTime >= 3000) {
        const char* oldName = getStatusString();
        currentStatus = target;
        const char* newName = getStatusString();
        Serial.printf("[EVENT] STATUS %s->%s\n", oldName, newName);

        // Update signals if fire/panic is not overriding
        if (!fireLatched && !panicActive && !dhtError && !rfidError) {
          if (currentStatus == STATUS_DANGER) Signals.setState(STATE_HEAT_DANGER);
          else if (currentStatus == STATUS_WARNING) Signals.setState(STATE_WARNING);
          else Signals.setState(STATE_SAFE);
        }
      }
    } else {
      candidateStatus = currentStatus;
    }
  }
};

extern SafetySystem Safety;

#endif // SAFETY_H
