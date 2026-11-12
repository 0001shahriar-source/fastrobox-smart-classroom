# Judging Strategy & Competition Playbook
**Event:** FastRobox 1.0 Techathon Grand Finale (BUBT)  
**Total Marks:** 500 | **Duration:** 7 Hours | **Judging Calls:** Maximum 4

---

## 1. Judging Call Optimization (Max 4 Calls)

Under the competition rules:
* You only get **4 judging calls**.
* You must call **consecutive levels** (e.g., Level 1, 2, 3 together).
* If any level fails in the middle of a call, the call terminates immediately, and subsequent levels are NOT judged!
* 1st Attempt pass = **100% marks**. 2nd Attempt pass = **70% marks**. Failing twice = **Permanent Closure** of that level and all future levels.

### Recommended Call Strategy:
* **Call 1 (Hour 2.5): Levels 1, 2, 3**
  - Foundational sensor reading, door simulation, and local status web page.
* **Call 2 (Hour 4.0): Levels 4, 5, 6**
  - RFID attendance (White card student check-in, Blue tag teacher mode) & cloud sync.
* **Call 3 (Hour 5.5): Levels 7, 8, 9**
  - Camera vision, surveillance streaming, and touch panic wire safety.
* **Call 4 (Hour 6.5): Levels 10, 11, 12**
  - Offline resilience, remote dashboard OTA/settings update, full integrated smart classroom demo.

---

## 2. The 0.5-Second Rule & Timing Tests
* Judges use a stopwatch to measure response times (e.g., RFID swipe to door unlock, flame detection to buzzer alert).
* If your response is within **0.5 seconds** of the allowable limit, judges re-measure once and take the better score.
* **Our firmware optimization:**
  - Non-blocking asynchronous event loops.
  - Interrupts / immediate polling on Flame Sensor (GPIO 21) and Touch (GPIO 1).
  - Target response time: **< 150 ms** (far safer than the 0.5s limit!).

---

## 3. Strict Rules Checklist

1. **NO Hardcoding:**
   - Wi-Fi SSID, Wi-Fi Password, Room ID, Class Schedule must NEVER be hardcoded.
   - Use our Serial CLI commands (`SET_WIFI:ssid,pass`, `SET_ROOM:Room-A`) or the local config web portal (`http://192.168.4.1` or local IP).
   - *Any hardcoded value discovered during judging will result in an immediate FAIL.*

2. **NO Secrets in Code or Git:**
   - No hardcoded API keys or credentials committed to GitHub.

3. **Display Surfaces Ready on Screen:**
   Before calling the judges, have these 4 surfaces visible across split screens / monitors:
   1. **Arduino Serial Monitor** (115200 baud).
   2. **Local Status Page** (`http://<esp32-ip>/` in browser).
   3. **Live Web Dashboard** (Public HTTPS URL).
   4. **Hardware Rig** (RGB LED, Buzzer, RFID cards ready).

---

## 4. Evidence Package for Every Level (Within 30 mins)

For every level passed, you must submit:
1. **Video (`<TeamID>_L<NN>_video.mp4`):**
   - Resolution: $\ge 720p$, clear voice.
   - Start video by speaking: *"Team ID [XYZ], Level [NN]"*.
   - Keep a visible stopwatch running on a phone/laptop screen.
   - Max 10 minutes, max 3 unedited clips.
2. **Write-up (`<TeamID>_L<NN>_writeup.pdf`):**
   - Max 2 pages.
   - 5 mandatory headings:
     1. Design
     2. Where it shows
     3. Test results
     4. Problems and fixes
     5. Extra item
3. **Git Tag:**
   - `git tag level-01`, `git tag level-02`, etc.
   - Push tags to GitHub: `git push origin --tags`.

