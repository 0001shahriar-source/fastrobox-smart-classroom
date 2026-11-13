#ifndef WEB_LOCAL_H
#define WEB_LOCAL_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "config.h"
#include "door.h"
#include "safety.h"
#include "rfid.h"
#include "device_signals.h"
#include "cloud_sync.h"

class LocalWebServer {
private:
  WebServer server;
  bool mdnsStarted;

  static const char INDEX_HTML[] PROGMEM;

public:
  LocalWebServer() : server(80), mdnsStarted(false) {}

  void begin() {
    // Start mDNS responder (http://smartclass-t07.local)
    if (!mdnsStarted) {
      if (MDNS.begin("smartclass-t07")) {
        mdnsStarted = true;
        Serial.println(F("[NET] mDNS responder started: http://smartclass-t07.local"));
      }
    }

    server.on("/", HTTP_GET, [this]() {
      server.send_P(200, "text/html", INDEX_HTML);
    });

    server.on("/api/status", HTTP_GET, [this]() {
      JsonDocument doc;
      doc["roomId"] = Config.get().roomId;
      doc["classTime"] = Config.get().classStartTime;
      doc["door"] = Door.getStateString();
      doc["temp"] = Safety.getTemp();
      doc["humidity"] = Safety.getHumidity();
      doc["heatIndex"] = Safety.getHeatIndex();
      doc["trend"] = Safety.getTrendString();
      doc["quality"] = Safety.getQuality();
      doc["status"] = Safety.getStatusString();
      doc["flame"] = Safety.isFlamePresent();
      doc["fireLatched"] = Safety.isFireLatched();
      doc["panic"] = Safety.isPanicActive();
      doc["lastUid"] = Rfid.getLastUid();
      doc["rssi"] = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;
      doc["fw"] = "1.0.0";
      #if defined(temperatureRead)
        doc["chipTemp"] = temperatureRead();
      #else
        doc["chipTemp"] = 41.5;
      #endif
      doc["waitingRecords"] = Cloud.getPendingCount();
      doc["connState"] = (WiFi.status() == WL_CONNECTED) ? "LIVE" : "OFFLINE";
      doc["ip"] = WiFi.localIP().toString();

      String json;
      serializeJson(doc, json);
      server.send(200, "application/json", json);
    });

    server.on("/api/door", HTTP_POST, [this]() {
      if (server.hasArg("action")) {
        String action = server.arg("action");
        if (action == "unlock") {
          Door.unlock(Config.get().doorDuration);
          Signals.triggerAccessGranted(Config.get().doorDuration);
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

    server.on("/api/reset-fire", HTTP_POST, [this]() {
      Safety.resetFire();
      server.send(200, "application/json", "{\"status\":\"fire_cleared\"}");
    });

    server.begin();
    Serial.println(F("[NET] Local HTTP Status Page running on port 80"));
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
  <title>Smart Classroom - Local Status Surface</title>
  <style>
    :root {
      --bg: #090d16;
      --card-bg: rgba(22, 30, 49, 0.85);
      --border: rgba(255, 255, 255, 0.08);
      --text: #f8fafc;
      --cyan: #38bdf8;
      --green: #22c55e;
      --red: #ef4444;
      --yellow: #eab308;
      --purple: #a855f7;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
    body { background: var(--bg); color: var(--text); padding: 18px; display: flex; justify-content: center; }
    .container { width: 100%; max-width: 820px; }
    header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px; padding-bottom: 14px; border-bottom: 1px solid var(--border); }
    h1 { font-size: 1.35rem; color: var(--cyan); }
    .badge { padding: 4px 10px; border-radius: 6px; font-size: 0.78rem; font-weight: 700; text-transform: uppercase; }
    .badge-live { background: rgba(34, 197, 94, 0.2); color: var(--green); border: 1px solid var(--green); }
    .badge-offline { background: rgba(239, 68, 68, 0.2); color: var(--red); border: 1px solid var(--red); }
    .alert-banner { display: none; padding: 14px; border-radius: 8px; margin-bottom: 16px; font-weight: 700; text-align: center; }
    .alert-fire { background: var(--red); color: #fff; animation: pulse 0.8s infinite; }
    .alert-panic { background: #f97316; color: #fff; animation: pulse 1s infinite; }
    @keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.6; } }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 14px; margin-bottom: 20px; }
    .card { background: var(--card-bg); border: 1px solid var(--border); border-radius: 12px; padding: 16px; }
    .card h3 { font-size: 0.72rem; color: #94a3b8; margin-bottom: 6px; text-transform: uppercase; letter-spacing: 0.5px; }
    .card .val { font-size: 1.5rem; font-weight: 700; color: #fff; }
    .card .sub { font-size: 0.75rem; color: #64748b; margin-top: 4px; }
    .controls { display: flex; gap: 10px; margin-top: 14px; }
    button { flex: 1; padding: 10px; border: none; border-radius: 8px; font-weight: 700; cursor: pointer; transition: 0.2s; }
    .btn-green { background: var(--green); color: #fff; }
    .btn-red { background: var(--red); color: #fff; }
  </style>
</head>
<body>
  <div class="container">
    <div id="fireBanner" class="alert-banner alert-fire">🔥 FIRE – EVACUATE! Simulated Door Permanently UNLOCKED!</div>
    <div id="panicBanner" class="alert-banner alert-panic">🚨 PANIC WIRE ACTIVATED! Emergency Help Needed!</div>

    <header>
      <div>
        <h1 id="roomTitle">Smart Classroom</h1>
        <p style="font-size:0.8rem;color:#94a3b8;" id="localClock">Local Time (Asia/Dhaka): Loading...</p>
      </div>
      <div>
        <span id="connBadge" class="badge badge-live">LIVE</span>
      </div>
    </header>

    <div class="grid">
      <div class="card">
        <h3>Simulated Door</h3>
        <div class="val" id="doorVal">LOCKED</div>
        <div class="sub" id="doorSub">Access: White Card</div>
      </div>
      <div class="card">
        <h3>Classroom Heat</h3>
        <div class="val" id="tempVal">-- °C</div>
        <div class="sub" id="heatIndexVal">Feels like: -- °C</div>
      </div>
      <div class="card">
        <h3>Humidity & Trend</h3>
        <div class="val" id="humVal">-- %</div>
        <div class="sub" id="trendVal">Trend: STABLE</div>
      </div>
      <div class="card">
        <h3>Safety Status</h3>
        <div class="val" id="statusVal" style="color:var(--green)">SAFE</div>
        <div class="sub" id="qualityVal">Quality: good</div>
      </div>
      <div class="card">
        <h3>Flame Sensor</h3>
        <div class="val" id="flameVal">NO FLAME</div>
        <div class="sub">Active LOW (GPIO 21)</div>
      </div>
      <div class="card">
        <h3>Last Card Swiped</h3>
        <div class="val" id="rfidVal" style="font-size:1.1rem;word-break:break-all;">None</div>
        <div class="sub" id="rfidSub">RC522 RFID</div>
      </div>
      <div class="card">
        <h3>Wi-Fi & System</h3>
        <div class="val" id="rssiVal">-- dBm</div>
        <div class="sub" id="chipTempVal">Chip: -- °C | FW: 1.0.0</div>
      </div>
      <div class="card">
        <h3>Offline Buffer</h3>
        <div class="val" id="waitingVal">0</div>
        <div class="sub">Records awaiting sync</div>
      </div>
    </div>

    <div class="card">
      <h3>Manual Test Controls</h3>
      <div class="controls">
        <button class="btn-green" onclick="setDoor('unlock')">MANUAL UNLOCK</button>
        <button class="btn-red" onclick="setDoor('lock')">MANUAL LOCK</button>
        <button class="btn-green" style="background:#0284c7;" onclick="resetFire()">RESET FIRE</button>
      </div>
    </div>
  </div>

  <script>
    async function updateStatus() {
      try {
        const res = await fetch('/api/status');
        const d = await res.json();

        document.getElementById('roomTitle').textContent = d.roomId + ' (Class: ' + d.classTime + ')';
        document.getElementById('tempVal').textContent = Number(d.temp).toFixed(1) + ' °C';
        document.getElementById('heatIndexVal').textContent = 'Feels like: ' + Number(d.heatIndex).toFixed(1) + ' °C';
        document.getElementById('humVal').textContent = Number(d.humidity).toFixed(0) + ' %';
        document.getElementById('trendVal').textContent = 'Trend: ' + d.trend;
        document.getElementById('qualityVal').textContent = 'Quality: ' + d.quality;
        document.getElementById('doorVal').textContent = d.door;
        document.getElementById('statusVal').textContent = d.status;
        document.getElementById('rssiVal').textContent = d.rssi + ' dBm';
        document.getElementById('chipTempVal').textContent = 'Chip: ' + Number(d.chipTemp).toFixed(1) + ' °C | v' + d.fw;
        document.getElementById('waitingVal').textContent = d.waitingRecords;

        // Connection Badge
        const connBadge = document.getElementById('connBadge');
        connBadge.textContent = d.connState;
        connBadge.className = 'badge ' + (d.connState === 'LIVE' ? 'badge-live' : 'badge-offline');

        // Flame and Fire Banners
        const fireBanner = document.getElementById('fireBanner');
        const flameVal = document.getElementById('flameVal');
        if (d.fireLatched || d.flame) {
          fireBanner.style.display = 'block';
          flameVal.textContent = 'FIRE ALERT!';
          flameVal.style.color = 'var(--red)';
        } else {
          fireBanner.style.display = 'none';
          flameVal.textContent = 'NO FLAME';
          flameVal.style.color = 'var(--green)';
        }

        // Panic Banner
        document.getElementById('panicBanner').style.display = d.panic ? 'block' : 'none';

        if (d.lastUid && d.lastUid.length > 0) {
          document.getElementById('rfidVal').textContent = d.lastUid;
        }

        // Update local Asia/Dhaka clock
        const now = new Date();
        document.getElementById('localClock').textContent = 'Local Time (Asia/Dhaka): ' + now.toLocaleTimeString('en-US', { timeZone: 'Asia/Dhaka' });

      } catch (e) {
        console.warn('Local status poll error:', e);
      }
    }

    async function setDoor(action) {
      await fetch('/api/door?action=' + action, { method: 'POST' });
      updateStatus();
    }

    async function resetFire() {
      await fetch('/api/reset-fire', { method: 'POST' });
      updateStatus();
    }

    setInterval(updateStatus, 1500);
    updateStatus();
  </script>
</body>
</html>
)rawliteral";

extern LocalWebServer LocalWeb;

#endif // WEB_LOCAL_H
