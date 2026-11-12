#ifndef WEB_LOCAL_H
#define WEB_LOCAL_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "config.h"
#include "door.h"
#include "safety.h"
#include "rfid.h"

class LocalWebServer {
private:
  WebServer server;

  static const char INDEX_HTML[] PROGMEM;

public:
  LocalWebServer() : server(80) {}

  void begin() {
    server.on("/", HTTP_GET, [this]() {
      server.send_P(200, "text/html", INDEX_HTML);
    });

    server.on("/api/status", HTTP_GET, [this]() {
      JsonDocument doc;
      doc["room"] = Config.get().roomId;
      doc["classTime"] = Config.get().classStartTime;
      doc["door"] = Door.getStateString();
      doc["temp"] = Safety.getTemperature();
      doc["humidity"] = Safety.getHumidity();
      doc["flame"] = Safety.isFlameDetected();
      doc["panic"] = Safety.isPanicTriggered();
      doc["lastUid"] = Rfid.getLastUid();
      doc["ip"] = WiFi.localIP().toString();
      doc["uptime"] = millis() / 1000;

      String json;
      serializeJson(doc, json);
      server.send(200, "application/json", json);
    });

    server.on("/api/door", HTTP_POST, [this]() {
      if (server.hasArg("action")) {
        String action = server.arg("action");
        if (action == "unlock") {
          Door.unlock(5000);
          server.send(200, "application/json", "{\"status\":\"unlocked\"}");
          return;
        } else if (action == "lock") {
          Door.lock();
          server.send(200, "application/json", "{\"status\":\"locked\"}");
          return;
        }
      }
      server.send(400, "application/json", "{\"error\":\"invalid action\"}");
    });

    server.on("/api/config", HTTP_POST, [this]() {
      if (server.hasArg("room")) {
        Config.saveRoom(server.arg("room").c_str());
      }
      if (server.hasArg("classTime")) {
        Config.saveClassTime(server.arg("classTime").c_str());
      }
      if (server.hasArg("ssid") && server.hasArg("pass")) {
        Config.saveWifi(server.arg("ssid").c_str(), server.arg("pass").c_str());
      }
      server.send(200, "application/json", "{\"status\":\"saved\"}");
    });

    server.begin();
    Serial.println(F("[WEB] Local HTTP Status Server started on port 80"));
  }

  void update() {
    server.handleClient();
  }
};

const char LocalWebServer::INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Smart Classroom - Local Status Page</title>
  <style>
    :root {
      --bg: #0f172a;
      --card-bg: rgba(30, 41, 59, 0.7);
      --border: rgba(255, 255, 255, 0.1);
      --text: #f8fafc;
      --accent: #38bdf8;
      --green: #22c55e;
      --red: #ef4444;
      --orange: #f97316;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
    body { background: var(--bg); color: var(--text); padding: 20px; display: flex; justify-content: center; }
    .container { width: 100%; max-width: 800px; }
    header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 24px; padding-bottom: 16px; border-bottom: 1px solid var(--border); }
    h1 { font-size: 1.5rem; color: var(--accent); }
    .badge { padding: 4px 12px; border-radius: 999px; font-size: 0.85rem; font-weight: 600; text-transform: uppercase; }
    .badge-locked { background: rgba(239, 68, 68, 0.2); color: var(--red); border: 1px solid var(--red); }
    .badge-unlocked { background: rgba(34, 197, 94, 0.2); color: var(--green); border: 1px solid var(--green); }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 16px; margin-bottom: 24px; }
    .card { background: var(--card-bg); border: 1px solid var(--border); border-radius: 12px; padding: 18px; backdrop-filter: blur(8px); }
    .card h3 { font-size: 0.85rem; color: #94a3b8; margin-bottom: 8px; text-transform: uppercase; }
    .card .val { font-size: 1.8rem; font-weight: 700; color: #ffffff; }
    .alert-banner { display: none; padding: 14px; border-radius: 8px; margin-bottom: 16px; font-weight: 600; }
    .alert-fire { background: var(--red); color: #fff; animation: pulse 1s infinite; }
    .alert-panic { background: var(--orange); color: #fff; }
    @keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.6; } }
    .controls { display: flex; gap: 12px; margin-top: 16px; }
    button { flex: 1; padding: 12px; border: none; border-radius: 8px; font-weight: 600; cursor: pointer; transition: 0.2s; }
    .btn-unlock { background: var(--green); color: #fff; }
    .btn-lock { background: var(--red); color: #fff; }
    button:hover { opacity: 0.9; transform: translateY(-1px); }
  </style>
</head>
<body>
  <div class="container">
    <div id="fireBanner" class="alert-banner alert-fire">⚠️ CRITICAL: FIRE DETECTED! DOORS AUTO-UNLOCKED!</div>
    <div id="panicBanner" class="alert-banner alert-panic">🚨 PANIC WIRE TRIGGERED! ASSISTANCE REQUIRED!</div>

    <header>
      <div>
        <h1 id="roomTitle">Smart Classroom</h1>
        <p style="font-size:0.85rem;color:#94a3b8;">ESP32-S3-CAM Local Status Surface</p>
      </div>
      <div>
        <span id="doorBadge" class="badge badge-locked">LOCKED</span>
      </div>
    </header>

    <div class="grid">
      <div class="card">
        <h3>Classroom Temp</h3>
        <div class="val" id="tempVal">-- °C</div>
      </div>
      <div class="card">
        <h3>Relative Humidity</h3>
        <div class="val" id="humVal">-- %</div>
      </div>
      <div class="card">
        <h3>Flame Status</h3>
        <div class="val" id="flameVal" style="color:var(--green)">SAFE</div>
      </div>
      <div class="card">
        <h3>Last RFID Swiped</h3>
        <div class="val" id="rfidVal" style="font-size:1.2rem;word-break:break-all;">None</div>
      </div>
    </div>

    <div class="card">
      <h3>Simulated Door Remote Control</h3>
      <div class="controls">
        <button class="btn-unlock" onclick="setDoor('unlock')">MANUAL UNLOCK (5s)</button>
        <button class="btn-lock" onclick="setDoor('lock')">MANUAL LOCK</button>
      </div>
    </div>
  </div>

  <script>
    async function refreshStatus() {
      try {
        const res = await fetch('/api/status');
        const data = await res.json();
        
        document.getElementById('roomTitle').textContent = data.room + ' (Starts: ' + data.classTime + ')';
        document.getElementById('tempVal').textContent = data.temp.toFixed(1) + ' °C';
        document.getElementById('humVal').textContent = data.humidity.toFixed(1) + ' %';
        
        const badge = document.getElementById('doorBadge');
        if (data.door === 'UNLOCKED') {
          badge.textContent = 'UNLOCKED';
          badge.className = 'badge badge-unlocked';
        } else {
          badge.textContent = 'LOCKED';
          badge.className = 'badge badge-locked';
        }

        const fireBanner = document.getElementById('fireBanner');
        const flameVal = document.getElementById('flameVal');
        if (data.flame) {
          fireBanner.style.display = 'block';
          flameVal.textContent = 'FIRE ALERT!';
          flameVal.style.color = 'var(--red)';
        } else {
          fireBanner.style.display = 'none';
          flameVal.textContent = 'SAFE';
          flameVal.style.color = 'var(--green)';
        }

        const panicBanner = document.getElementById('panicBanner');
        panicBanner.style.display = data.panic ? 'block' : 'none';

        if (data.lastUid && data.lastUid.length > 0) {
          document.getElementById('rfidVal').textContent = data.lastUid;
        }
      } catch (e) {
        console.error('Fetch error:', e);
      }
    }

    async function setDoor(action) {
      await fetch('/api/door?action=' + action, { method: 'POST' });
      refreshStatus();
    }

    setInterval(refreshStatus, 1000);
    refreshStatus();
  </script>
</body>
</html>
)rawliteral";

extern LocalWebServer LocalWeb;

#endif // WEB_LOCAL_H
