# Level Write-up Template (FastRobox 1.0 Grand Finale)
**Team ID:** [YOUR_TEAM_ID]  
**Level:** Level [NN]  
**Topic:** [e.g. Temperature, Fire Safety & Simulated Door Automation]  
**Date:** November 13, 2026  

---

### (1) Design
*Provide a concise description and architectural diagram of what was built in this level.*
* **Components Used:** ESP32-S3-CAM, [Sensors used in this level].
* **System Logic:** Explanation of how the inputs trigger outputs (e.g. Flame sensor active LOW triggers instant buzzer interrupt, unlocks simulated door, transitions RGB LED to flashing red, and posts alert to local page and cloud endpoint).
* **Block Diagram:**
```
[Sensor Input / Event] ---> [ESP32-S3 Event Loop / ISR] ---> [Actuator / Simulated Door / LED / Buzzer]
                                                          ---> [Local Status Page & Web Dashboard]
```

---

### (2) Where it shows
*List of all display surfaces demonstrating the operational state of this level with screenshots:*
1. **Serial Monitor:** Shows live timestamped logs (e.g., `[SAFETY] Flame detected! Evacuation unlock initiated`).
2. **Local Status Page (`http://<esp32-ip>`):** Live HTML badge showing sensor readings, door state, and alarm status.
3. **Web Dashboard (Public HTTPS):** Cloud dashboard displaying real-time telemetry, historical chart, and alert banner.
4. **Hardware Indicators:** Onboard WS2812 RGB LED color and Active Buzzer audible state.

---

### (3) Test results
*Tabulated test measurements and live check results showing compliance with competition response time limits:*

| Test Item | Parameter / Condition | Expected Result | Measured Value | Allowed Limit | Pass/Fail |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Response Time** | Flame trigger to buzzer alert | Instant audible alarm | ~120 ms | $\le 500\text{ ms}$ | **PASS** |
| **Door Simulation** | White RFID Card Check-in | Door status -> UNLOCKED | ~140 ms | $\le 1000\text{ ms}$ | **PASS** |
| **Cloud Sync** | Web Dashboard update | Telemetry reflected | ~350 ms | $\le 2000\text{ ms}$ | **PASS** |
| **Local Status** | Local web page refresh | Reading updated | ~80 ms | $\le 1000\text{ ms}$ | **PASS** |

*Note: Video timestamp reference: 01:23 - 01:45.*

---

### (4) Problems and fixes
*Specific technical challenges encountered during this level and how they were solved:*
* **Problem 1:** High-frequency noise or false triggers on the flame sensor pin during initial boot.
  - *Fix:* Added software debounce and digital filter window (requiring 3 consecutive LOW samples within 30ms) before confirming fire state.
* **Problem 2:** RC522 SPI bus contention with the camera peripherals.
  - *Fix:* Explicitly remapped SPI pins using `SPI.begin(SCK, MISO, MOSI, SS)` to dedicated non-camera GPIOs.

---

### (5) Extra item
*Additional resilience and edge-case handling built into this level:*
* **Offline Event Buffering:** If the Wi-Fi connection drops, all security logs and attendance events are buffered in non-volatile storage (NVS/LittleFS) and automatically flushed to the live cloud database upon reconnection without data loss.
* **Fail-Safe Door Release:** In the event of an emergency (flame or touch panic), the simulated door defaults to `UNLOCKED` automatically to ensure classroom evacuation safety.

