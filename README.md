# FastRobox 1.0 Techathon Grand Finale — Participant Repository
## AI-Powered Smart Classroom: Safety, Access and Vision
**Organizer:** Bangladesh University of Business and Technology (BUBT)  
**Hardware Platform:** ESP32-S3-CAM (1 Board)  
**Total Marks:** 500 | **Duration:** 7 Hours  

---

## 1. Project Mission Overview
This repository contains the complete, battle-tested software and hardware architecture for the **AI-Powered Smart Classroom: Safety, Access and Vision** system. 

Built exclusively with **1 ESP32-S3-CAM** and 5 kit peripherals (without breadboards or auxiliary microcontrollers), the system achieves:
1. **Safety & Environmental Surveillance:** Continuous classroom temperature and relative humidity telemetry (DHT11) with instant fire/flame detection (IR Flame Sensor).
2. **Simulated Door Access Control:** Multi-tier RFID authentication via RC522 (White Card for Student Check-in; Blue Key-Tag for Teacher Override) with visual state indicators (WS2812 NeoPixel RGB LED) and acoustic buzzer feedback.
3. **Fail-Safe Fire Evacuation:** Instantaneous (<150ms) emergency unlock upon flame or panic detection.
4. **Capacitive Touch Panic Wire:** Single bare jumper wire on Touch GPIO acting as an emergency classroom alarm pad.
5. **Surveillance & Vision:** OV2640 camera streaming, snapshot capture, and Wi-Fi signal tracking.
6. **Zero-Hardcoding Architecture:** Wi-Fi credentials, Room ID, and class start times are dynamically configured on-the-spot via Serial CLI or local Web portal.
7. **Offline Resilience:** Buffered circular queue storing up to 50 security/attendance events during network drops, with automatic cloud flush upon reconnection.
8. **Live-Hosted Public Cloud Platform:** HTTPS-accessible real-time dashboard and searchable audit history database.

---

## 2. Hardware Bill of Materials (Strict Kit Only)

| Component | Pin Connections to ESP32-S3-CAM | Purpose in System |
| :--- | :--- | :--- |
| **ESP32-S3-CAM** | - | Main SoC, Wi-Fi/BT, Camera, Status LED, Boot Button |
| **RC522 RFID Reader** | SS: GPIO 10, SCK: GPIO 12, MOSI: GPIO 11, MISO: GPIO 13, RST: GPIO 3 | Student check-in & teacher action |
| **DHT11 Sensor** | DATA: GPIO 14, VCC: 3.3V, GND: GND | Ambient temperature & humidity tracking |
| **IR Flame Sensor** | DO: GPIO 21, VCC: 3.3V, GND: GND | High-speed fire / optical flame detection |
| **Active Buzzer** | (+): GPIO 47, (-): GND | Audible alarm, chirps, and feedback |
| **Jumper Wires** | Bare Wire Tip: GPIO 1 (Touch 1) | Interconnections + Capacitive Panic Touch Pad |

*See [`docs/WIRING_AND_PINOUT.md`](docs/WIRING_AND_PINOUT.md) for full wiring schematics without a breadboard.*

---

## 3. Directory Structure

```
├── docs/
│   ├── WIRING_AND_PINOUT.md       # Full jumper wire routing without breadboard
│   ├── JUDGING_STRATEGY.md        # Strategy for 4 judging calls, 70% rule, timing
│   └── SYSTEM_ARCHITECTURE.md     # Sequence diagrams and state machine specifications
├── firmware/
│   └── SmartClassroom/
│       ├── SmartClassroom.ino     # Main Arduino sketch & event loop
│       ├── config.h               # NVS Preferences dynamic configuration
│       ├── pins.h                 # Safe GPIO mapping for ESP32-S3-CAM
│       ├── door.h                 # Simulated door FSM & RGB visual feedback
│       ├── safety.h               # DHT11, Flame sensor debouncing, Touch panic
│       ├── rfid.h                 # RC522 SPI handler (White card / Blue tag)
│       ├── serial_cli.h           # Dynamic Serial CLI (Zero hardcoding)
│       ├── web_local.h            # Local HTTP status webpage (`http://<esp32-ip>`)
│       └── cloud_sync.h           # Cloud synchronization & offline buffering
├── web-platform/
│   ├── server.js                  # Express.js Cloud API & WebSocket/REST server
│   ├── package.json               # Node.js dependencies
│   └── public/
│       ├── index.html             # Modern Glassmorphic Dashboard
│       ├── style.css              # Custom styling & animated alert banners
│       └── app.js                 # Real-time telemetry fetcher & audit log filter
└── writeups/
    └── templates/
        └── level_writeup_template.md # 2-page write-up format with 5 required sections
```

---

## 4. Zero Hard-Coding: On-The-Spot Dynamic Configuration

Judges will assign test values on the spot (Wi-Fi SSID, Room ID, Class Time). The system provides two methods to configure these instantly:

### Method A: Via Serial Monitor (115200 Baud)
Open the Arduino Serial Monitor and send:
```text
SET_WIFI:<ssid>,<password>    # Sets Wi-Fi credentials and initiates connection
SET_ROOM:<roomId>             # Updates Room ID (e.g., Room-302)
SET_TIME:<HH:MM>              # Updates Class Start Time (e.g., 09:30)
SET_CLOUD:<url>               # Updates Web Platform endpoint
STATUS                        # Prints complete JSON snapshot of all sensors
UNLOCK                        # Manually triggers door unlock for 5 seconds
LOCK                          # Manually locks door
```

### Method B: Via Local Web Configuration
Connect to the ESP32's local IP address or SoftAP (`SmartClassroom-Setup` / Password: `bubt1234`) and visit:
`http://192.168.4.1/` to change settings from any smartphone browser.

---

## 5. 12 Sequential Levels Roadmap

The competition consists of 12 sequential levels across 6 groups:
* **Level 1–2 (Group 1):** ESP32-S3 Boot, Onboard RGB LED door status, DHT11 & Flame digital read.
* **Level 3–4 (Group 2):** RC522 SPI integration, White Student card unlock, Blue tag teacher override.
* **Level 5–6 (Group 3):** Local Web Server status page (`/api/status`), Serial CLI dynamic configuration.
* **Level 7–8 (Group 4):** Capacitive touch panic wire alert, instant fire fail-safe evacuation unlock.
* **Level 9–10 (Group 5):** Live HTTPS Web Dashboard sync, real-time audit database with search filters.
* **Level 11–12 (Group 6):** Offline buffering resilience (network drop handling) & fully integrated classroom vision demo.

---

## 6. Git Commit Tags & Submission

As per competition rules, each level must be tagged upon completion:
```bash
git tag level-01
git tag level-02
# ... up to level-12
git push origin --tags
```
File naming convention for submitted evidence:
* Video: `<TeamID>_L<NN>_video.mp4`
* Write-up: `<TeamID>_L<NN>_writeup.pdf`

