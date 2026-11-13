# ESP32-S3-CAM Hardware Wiring & Pinout Guide
**Project:** AI-Powered Smart Classroom: Safety, Access and Vision  
**Competition:** FastRobox 1.0 Techathon Grand Finale (BUBT)

> **Important Constraint:** No breadboard, no additional electronics, and no second microcontroller. All connections are made directly using jumper wires to the ESP32-S3-CAM board.

---

## 1. Pin Assignment Table

| Component | Component Pin | ESP32-S3 Pin | Function / Notes |
| :--- | :--- | :--- | :--- |
| **Power** | VCC (Modules) | 3.3V / 5V | Power supply rail from ESP32-S3 board |
| **Power** | GND (Modules) | GND | Common ground return |
| **RC522 RFID** | SDA (SS) | **GPIO 10** | SPI Chip Select |
| **RC522 RFID** | SCK | **GPIO 12** | SPI Clock |
| **RC522 RFID** | MOSI | **GPIO 11** | SPI Master Out Slave In |
| **RC522 RFID** | MISO | **GPIO 13** | SPI Master In Slave Out |
| **RC522 RFID** | IRQ | *Not Connected* | Unused |
| **RC522 RFID** | GND | GND | Ground |
| **RC522 RFID** | RST | **GPIO 3** | RFID Reset Pin |
| **RC522 RFID** | 3.3V | 3.3V | **Must be 3.3V** (Do NOT connect to 5V) |
| **DHT11 Sensor** | DATA / OUT | **GPIO 14** | Single-bus digital temperature & humidity |
| **DHT11 Sensor** | VCC | 3.3V / 5V | Power |
| **DHT11 Sensor** | GND | GND | Ground |
| **IR Flame Sensor**| DO (Digital Out)| **GPIO 21** | Digital Fire detect (Active LOW) |
| **IR Flame Sensor**| VCC | 3.3V / 5V | Power |
| **IR Flame Sensor**| GND | GND | Ground |
| **Active Buzzer** | (+) Positive | **GPIO 2** | Active HIGH trigger for beep / alarm |
| **Active Buzzer** | (-) Negative | GND | Ground |
| **Touch Panic Wire**| Bare wire end | **GPIO 1** | ESP32-S3 Capacitive Touch Pin (Touch 1) |
| **Onboard RGB LED**| Built-in | **GPIO 48 / RGB_BUILTIN** | WS2812 NeoPixel Status Light |
| **BOOT Button** | Built-in | **GPIO 0** | Mode toggle / Emergency reset |

*Note: Pins are chosen to strictly avoid conflicting with the OV2640 camera data bus and PSRAM pins.*

---

## 2. Power & Ground Distribution without a Breadboard

Since only jumper wires are allowed and no breadboard is permitted:
1. **GND Sharing:** Daisy-chain GND jumper wires or use female-to-female jumper links if available, or tap ground pins available on both sides of the ESP32-S3-CAM board (most ESP32-S3-CAM modules provide multiple GND pins and a 3.3V and 5V pin).
2. **RFID Power Warning:** The RC522 RFID Reader **strictly requires 3.3V**. Connecting it to 5V will permanently damage the RC522 chip.
3. **DHT11 & Flame Sensor:** Can safely operate on either 3.3V or 5V.

---

## 3. Touch Panic Pad Setup

* Take **1 single jumper wire**.
* Connect one end to **GPIO 1**.
* Leave the other end exposed as a bare metal pin/tip.
* When touched by human fingers, the capacitance drops sharply (`touchRead(1) < TOUCH_THRESHOLD`), instantly triggering the Panic Alarm.

---

## 4. Simulated Door Operation

* As per rule Section 3: *No physical motor or lock is provided.*
* The door state is maintained as a finite state machine: `LOCKED` vs `UNLOCKED`.
* Indicated by:
  - **Onboard RGB LED (Device Signals Table):**
    - State 1 (Fire): Red fast flashing (5 Hz)
    - State 2 (Panic): Red and Blue alternating (100ms toggle)
    - State 3 (Access Granted / Unlocked): Solid Green for door unlock duration (default 5s)
    - State 4 (Access Denied): Three quick Red flashes
    - State 5 (Heat Danger): Red 2 Hz heartbeat
    - State 9 (Warning): Yellow 2 Hz heartbeat
    - State 10 (Wi-Fi Setup / Offline): Blue 2 Hz heartbeat
    - State 11 (Safe / Normal): Green 2 Hz heartbeat
  - **Buzzer:** 1 short beep on Access Granted, 2 beeps on Denied, continuous 200ms on/off on Fire, 100ms on/off on Panic.
  - **Local Status Webpage:** Real-time visual lock icon and badge.
  - **Cloud Dashboard:** Live toggle switch and animated door badge.

