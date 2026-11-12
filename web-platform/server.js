const express = require('express');
const cors = require('cors');
const path = require('path');
const fs = require('fs');

const app = express();
const PORT = process.env.PORT || 3000;

app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

// In-Memory Database with optional JSON file backup
const DB_FILE = path.join(__dirname, 'db_records.json');

let systemStatus = {
  roomId: 'Room-101',
  classStartTime: '09:00',
  doorState: 'LOCKED',
  temperature: 26.5,
  humidity: 62.0,
  flame: false,
  panic: false,
  lastUid: '',
  lastUpdate: Date.now()
};

let eventLog = [];

// Load existing records if available
if (fs.existsSync(DB_FILE)) {
  try {
    const raw = fs.readFileSync(DB_FILE, 'utf8');
    const parsed = JSON.parse(raw);
    if (parsed.status) systemStatus = parsed.status;
    if (parsed.events) eventLog = parsed.events;
  } catch (err) {
    console.error('Failed to parse db_records.json:', err.message);
  }
}

function persistDb() {
  try {
    fs.writeFileSync(DB_FILE, JSON.stringify({ status: systemStatus, events: eventLog }, null, 2));
  } catch (err) {
    console.error('Failed to persist db:', err.message);
  }
}

// 1. Telemetry ingest from ESP32
app.post('/api/telemetry', (req, res) => {
  const { roomId, classTime, doorState, temp, humidity, flame, panic } = req.body;
  if (roomId) systemStatus.roomId = roomId;
  if (classTime) systemStatus.classStartTime = classTime;
  if (doorState) systemStatus.doorState = doorState;
  if (temp !== undefined) systemStatus.temperature = Number(temp);
  if (humidity !== undefined) systemStatus.humidity = Number(humidity);
  if (flame !== undefined) systemStatus.flame = Boolean(flame);
  if (panic !== undefined) systemStatus.panic = Boolean(panic);
  systemStatus.lastUpdate = Date.now();

  res.json({ success: true, timestamp: Date.now() });
});

// 2. Immediate Event ingest (Attendance, Flame, Panic, Door changes)
app.post('/api/events', (req, res) => {
  const { roomId, type, detail, doorState, temp, humidity } = req.body;
  const newEvent = {
    id: 'evt_' + Date.now() + '_' + Math.floor(Math.random() * 1000),
    roomId: roomId || systemStatus.roomId,
    type: type || 'GENERAL',
    detail: detail || '',
    doorState: doorState || systemStatus.doorState,
    temp: temp !== undefined ? temp : systemStatus.temperature,
    humidity: humidity !== undefined ? humidity : systemStatus.humidity,
    source: 'LIVE_DEVICE',
    createdAt: new Date().toISOString()
  };

  if (doorState) systemStatus.doorState = doorState;
  if (type === 'ATTENDANCE' && detail) {
    systemStatus.lastUid = detail;
  }

  eventLog.unshift(newEvent);
  if (eventLog.length > 500) eventLog.pop(); // keep latest 500 records
  persistDb();

  console.log(`[EVENT RECEIVED] [${newEvent.type}] ${newEvent.detail} (${newEvent.createdAt})`);
  res.json({ success: true, eventId: newEvent.id });
});

// 3. Offline Resilience Sync endpoint (Flushes queued offline events from ESP32)
app.post('/api/offline-sync', (req, res) => {
  const { roomId, events } = req.body;
  if (!Array.isArray(events)) {
    return res.status(400).json({ error: 'events array required' });
  }

  console.log(`[OFFLINE BATCH] Received ${events.length} buffered offline events from ${roomId}`);

  events.forEach(evt => {
    eventLog.unshift({
      id: 'offline_' + Date.now() + '_' + Math.floor(Math.random() * 1000),
      roomId: roomId || systemStatus.roomId,
      type: evt.type,
      detail: evt.payload,
      source: 'OFFLINE_BUFFERED',
      deviceTimeMs: evt.deviceTimeMs,
      createdAt: new Date().toISOString()
    });
  });

  persistDb();
  res.json({ success: true, processedCount: events.length });
});

// 4. Status endpoint for Web Dashboard
app.get('/api/status', (req, res) => {
  const isOnline = (Date.now() - systemStatus.lastUpdate) < 15000;
  res.json({ ...systemStatus, isOnline });
});

// 5. Searchable & Filterable Event Database
app.get('/api/events', (req, res) => {
  const { search, type, limit } = req.query;
  let results = [...eventLog];

  if (type && type !== 'ALL') {
    results = results.filter(e => e.type.toUpperCase() === type.toUpperCase());
  }

  if (search) {
    const q = search.toLowerCase();
    results = results.filter(e => 
      e.detail.toLowerCase().includes(q) || 
      e.type.toLowerCase().includes(q) ||
      e.createdAt.toLowerCase().includes(q)
    );
  }

  const max = limit ? parseInt(limit, 10) : 50;
  res.json({ total: results.length, events: results.slice(0, max) });
});

// 6. Remote Door Control Action
app.post('/api/door', (req, res) => {
  const { action } = req.body;
  if (action === 'unlock') {
    systemStatus.doorState = 'UNLOCKED';
    eventLog.unshift({
      id: 'cmd_' + Date.now(),
      roomId: systemStatus.roomId,
      type: 'REMOTE_COMMAND',
      detail: 'Dashboard Remote UNLOCK command issued',
      doorState: 'UNLOCKED',
      createdAt: new Date().toISOString()
    });
  } else if (action === 'lock') {
    systemStatus.doorState = 'LOCKED';
    eventLog.unshift({
      id: 'cmd_' + Date.now(),
      roomId: systemStatus.roomId,
      type: 'REMOTE_COMMAND',
      detail: 'Dashboard Remote LOCK command issued',
      doorState: 'LOCKED',
      createdAt: new Date().toISOString()
    });
  }
  persistDb();
  res.json({ success: true, doorState: systemStatus.doorState });
});

// 7. Dynamic Configuration Update from Dashboard
app.post('/api/config', (req, res) => {
  const { roomId, classStartTime } = req.body;
  if (roomId) systemStatus.roomId = roomId;
  if (classStartTime) systemStatus.classStartTime = classStartTime;
  persistDb();
  res.json({ success: true, status: systemStatus });
});

app.listen(PORT, () => {
  console.log(`=======================================================`);
  console.log(` Smart Classroom Cloud Platform live on port ${PORT}`);
  console.log(` Local URL : http://localhost:${PORT}`);
  console.log(`=======================================================`);
});
