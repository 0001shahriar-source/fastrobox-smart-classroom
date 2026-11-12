#ifndef PINS_H
#define PINS_H

#include <Arduino.h>

// ==========================================
// ESP32-S3-CAM Hardware Pin Configuration
// Tested and safe with OV2640 Camera & PSRAM
// ==========================================

// --- RC522 RFID Reader (SPI) ---
#define RFID_SS_PIN     10   // Chip Select (SDA)
#define RFID_RST_PIN    3    // Reset
#define RFID_SCK_PIN    12   // SPI Clock
#define RFID_MISO_PIN   13   // SPI MISO
#define RFID_MOSI_PIN   11   // SPI MOSI

// --- Environmental & Safety Sensors ---
#define DHT_PIN         14   // DHT11 Data Pin
#define FLAME_PIN       21   // Flame Sensor Digital Output (Active LOW)
#define BUZZER_PIN      47   // Active Buzzer (Active HIGH)
#define TOUCH_PANIC_PIN 1    // Capacitive Touch Panic Wire (GPIO 1 / Touch 1)

// --- Onboard Peripherals ---
#define BOOT_BUTTON_PIN 0    // Physical BOOT button on ESP32-S3 board
#ifndef RGB_BUILTIN
  #define RGB_BUILTIN   48   // Onboard Addressable WS2812 RGB LED (Standard ESP32-S3)
#endif

// Touch detection threshold
#define TOUCH_THRESHOLD 40000 // Capacitive threshold for bare-wire touch

#endif // PINS_H

