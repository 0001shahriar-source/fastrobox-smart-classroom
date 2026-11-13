#ifndef DEVICE_SIGNALS_H
#define DEVICE_SIGNALS_H

#include <Arduino.h>
#include "pins.h"

enum SystemDeviceState {
  STATE_FIRE = 1,
  STATE_PANIC = 2,
  STATE_ACCESS_GRANTED = 3,
  STATE_ACCESS_DENIED = 4,
  STATE_HEAT_DANGER = 5,
  STATE_SENSOR_ERROR = 6,
  STATE_CAMERA_FAULT = 7,
  STATE_WEATHER_DANGER = 8,
  STATE_WARNING = 9,
  STATE_WIFI_SETUP_OR_OFFLINE = 10,
  STATE_SAFE = 11
};

class DeviceSignals {
private:
  SystemDeviceState currentState;
  unsigned long stateStartTime;
  unsigned long lastBlink;
  bool blinkState;
  
  // Temporary overrides (e.g. Access Granted / Denied)
  bool isTempOverride;
  unsigned long tempOverrideEnd;
  SystemDeviceState baseState;

  // Buzzer timing
  unsigned long lastBuzzerToggle;
  bool buzzerPinState;
  int beepsRemaining;
  unsigned long nextBeepTime;

  void writeRgb(uint8_t r, uint8_t g, uint8_t b) {
    #ifdef RGB_BUILTIN
      neopixelWrite(RGB_BUILTIN, r, g, b);
    #endif
  }

  void setBuzzer(bool on) {
    digitalWrite(BUZZER_PIN, on ? HIGH : LOW);
    buzzerPinState = on;
  }

public:
  DeviceSignals() : currentState(STATE_SAFE), stateStartTime(0), lastBlink(0), 
                    blinkState(false), isTempOverride(false), tempOverrideEnd(0), 
                    baseState(STATE_SAFE), lastBuzzerToggle(0), buzzerPinState(false),
                    beepsRemaining(0), nextBeepTime(0) {}

  void begin() {
    pinMode(BUZZER_PIN, OUTPUT);
    setBuzzer(false);
    setState(STATE_SAFE);
  }

  void setState(SystemDeviceState st) {
    if (currentState != st) {
      currentState = st;
      stateStartTime = millis();
    }
  }

  SystemDeviceState getState() const {
    return currentState;
  }

  const char* getStateString() const {
    switch (currentState) {
      case STATE_FIRE: return "FIRE";
      case STATE_PANIC: return "PANIC";
      case STATE_HEAT_DANGER: return "DANGER";
      case STATE_WARNING: return "WARNING";
      case STATE_SAFE: return "SAFE";
      case STATE_SENSOR_ERROR: return "SENSOR_ERROR";
      case STATE_CAMERA_FAULT: return "CAMERA_FAULT";
      case STATE_WEATHER_DANGER: return "WEATHER_DANGER";
      case STATE_WIFI_SETUP_OR_OFFLINE: return "OFFLINE";
      default: return "SAFE";
    }
  }

  // Trigger Access Granted (Solid Green 5s + 1 short beep)
  void triggerAccessGranted(unsigned long durationMs = 5000) {
    isTempOverride = true;
    tempOverrideEnd = millis() + durationMs;
    writeRgb(0, 128, 0); // Solid green
    // One short beep
    setBuzzer(true);
    delay(80);
    setBuzzer(false);
  }

  // Trigger Access Denied (3 quick red flashes + 2 short beeps)
  void triggerAccessDenied() {
    for (int i = 0; i < 3; i++) {
      writeRgb(128, 0, 0);
      setBuzzer(i < 2); // 2 short beeps total
      delay(70);
      writeRgb(0, 0, 0);
      setBuzzer(false);
      delay(70);
    }
    // Return to current state visual
  }

  // Trigger Weather Danger (Three beeps once on arrival + Purple 2Hz)
  void triggerWeatherDanger() {
    setState(STATE_WEATHER_DANGER);
    for (int i = 0; i < 3; i++) {
      setBuzzer(true);
      delay(100);
      setBuzzer(false);
      delay(80);
    }
  }

  void update() {
    unsigned long now = millis();

    // Check temp override expiry
    if (isTempOverride) {
      if (now >= tempOverrideEnd) {
        isTempOverride = false;
      } else {
        // In solid green access granted
        writeRgb(0, 128, 0);
        return;
      }
    }

    // Process State Signals according to Device Signals Table
    switch (currentState) {
      case STATE_FIRE:
        // 1. Fire: Red fast flashing (5 times a second = 100ms on/off), continuous buzzer (200ms on/off)
        if (now - lastBlink >= 100) {
          lastBlink = now;
          blinkState = !blinkState;
          writeRgb(blinkState ? 255 : 0, 0, 0);
        }
        if (now - lastBuzzerToggle >= 200) {
          lastBuzzerToggle = now;
          setBuzzer(!buzzerPinState);
        }
        break;

      case STATE_PANIC:
        // 2. Panic: Red and blue alternating (100ms on / 100ms off), buzzer for 10s
        if (now - lastBlink >= 100) {
          lastBlink = now;
          blinkState = !blinkState;
          if (blinkState) writeRgb(255, 0, 0); // Red
          else writeRgb(0, 0, 255);            // Blue
        }
        if (now - stateStartTime < 10000) {
          if (now - lastBuzzerToggle >= 100) {
            lastBuzzerToggle = now;
            setBuzzer(!buzzerPinState);
          }
        } else {
          setBuzzer(false);
        }
        break;

      case STATE_HEAT_DANGER:
        // 5. Heat Danger: Red 2 Hz heartbeat (250ms on / 250ms off), One short beep every 2s
        if (now - lastBlink >= 250) {
          lastBlink = now;
          blinkState = !blinkState;
          writeRgb(blinkState ? 200 : 0, 0, 0);
        }
        if (now - lastBuzzerToggle >= 2000) {
          lastBuzzerToggle = now;
          setBuzzer(true);
        } else if (buzzerPinState && now - lastBuzzerToggle >= 80) {
          setBuzzer(false);
        }
        break;

      case STATE_SENSOR_ERROR:
        // 6. Sensor error: Magenta 2 Hz heartbeat
        if (now - lastBlink >= 250) {
          lastBlink = now;
          blinkState = !blinkState;
          writeRgb(blinkState ? 180 : 0, 0, blinkState ? 180 : 0);
        }
        setBuzzer(false);
        break;

      case STATE_CAMERA_FAULT:
        // 7. Camera fault: Cyan 2 Hz heartbeat
        if (now - lastBlink >= 250) {
          lastBlink = now;
          blinkState = !blinkState;
          writeRgb(0, blinkState ? 180 : 0, blinkState ? 180 : 0);
        }
        setBuzzer(false);
        break;

      case STATE_WEATHER_DANGER:
        // 8. Weather Danger: Purple 2 Hz heartbeat
        if (now - lastBlink >= 250) {
          lastBlink = now;
          blinkState = !blinkState;
          writeRgb(blinkState ? 140 : 0, 0, blinkState ? 200 : 0);
        }
        break;

      case STATE_WARNING:
        // 9. Warning: Yellow 2 Hz heartbeat
        if (now - lastBlink >= 250) {
          lastBlink = now;
          blinkState = !blinkState;
          writeRgb(blinkState ? 180 : 0, blinkState ? 140 : 0, 0);
        }
        setBuzzer(false);
        break;

      case STATE_WIFI_SETUP_OR_OFFLINE:
        // 10. Wi-Fi setup mode or offline: Blue 2 Hz heartbeat
        if (now - lastBlink >= 250) {
          lastBlink = now;
          blinkState = !blinkState;
          writeRgb(0, 0, blinkState ? 180 : 0);
        }
        setBuzzer(false);
        break;

      case STATE_SAFE:
      default:
        // 11. Safe (normal): Green 2 Hz heartbeat
        if (now - lastBlink >= 250) {
          lastBlink = now;
          blinkState = !blinkState;
          writeRgb(0, blinkState ? 120 : 0, 0);
        }
        setBuzzer(false);
        break;
    }
  }
};

extern DeviceSignals Signals;

#endif // DEVICE_SIGNALS_H
