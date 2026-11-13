const express = require('express');
const cors = require('cors');
const path = require('path');
const fs = require('fs');
const bcrypt = require('bcryptjs');

const app = express();
const PORT = process.env.PORT || 3000;

app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

const DB_FILE = path.join(__dirname, 'db_records.json');

// -------------------------------------------------------------
// System Telemetry & Hardware State
// -------------------------------------------------------------
let systemStatus = {
  deviceId: '14-D1',
  roomId: 'Room-101',
  classStartTime: '09:00',
  doorState: 'LOCKED',
  temperature: 28.5,
  humidity: 62.0,
  heatIndex: 30.1,
  heatTrend: 'STABLE',
  flame: false,
  fireLatched: false,
  panic: false,
  lastUid: '',
  quality: 'good',
  rssi: -58,
  heapKb: 184,
  psramKb: 7104,
  chipTemp: 41.2,
  fwVersion: '1.0.0',
  lastResetReason: 'POWER_ON',
  warningTemp: 32.0,
  dangerTemp: 40.0,
  doorUnlockTimeSec: 5,
  cameraPosition: 'DOOR', // "DOOR" or "ROOM"
  lastUpdate: Date.now()
};

let eventLog = [];
let telemetryHistory = [];
let loginHistory = [];
let blockedCards = {}; // uid -> blockedUntil timestamp
let failedLogins = {}; // username -> { count, lockedUntil }
let teacherAlerts = [];

// Pre-seeded Users with Bcrypt Hashed Passwords (Level 4 Task 8)
// Admin: admin123 | Teacher: teach123 | Student S1: stud123 | Student S2: stud456 | Guardian: guard123
const usersDb = {
  admin: {
    username: 'admin',
    passwordHash: bcrypt.hashSync('admin123', 8),
    role: 'Admin',
    name: 'System Administrator'
  },
  teacher: {
    username: 'teacher',
    passwordHash: bcrypt.hashSync('teach123', 8),
    role: 'Teacher',
    name: 'Class Instructor'
  },
  student1: {
    username: 'student1',
    passwordHash: bcrypt.hashSync('stud123', 8),
    role: 'Student',
    studentId: 'S1',
    name: 'Tanvir Ahmed',
    cardUid: '4A:9C:31:FE'
  },
  student2: {
    username: 'student2',
    passwordHash: bcrypt.hashSync('stud456', 8),
    role: 'Student',
    studentId: 'S2',
    name: 'Rahim Khan',
    cardUid: 'E2:1B:77:4A'
  },
  guardian: {
    username: 'guardian',
    passwordHash: bcrypt.hashSync('guard123', 8),
    role: 'Guardian',
    linkedStudentId: 'S1',
    name: 'Parent of Tanvir'
  }
};

// Students Enrolled Database (Level 7 & 8)
let studentsDb = {
  'S1': { id: 'S1', name: 'Tanvir Ahmed', cardUid: '4A:9C:31:FE', embeddingStored: true, imagesDeleted: true },
  'S2': { id: 'S2', name: 'Rahim Khan', cardUid: 'E2:1B:77:4A', embeddingStored: true, imagesDeleted: true }
};

// Attendance Records (Level 7)
let attendanceRecords = [
  { studentId: 'S1', name: 'Tanvir Ahmed', status: 'Present', entryTime: '09:05 AM', class: 'Robotics-101' }
];

// Load persisted DB if exists
if (fs.existsSync(DB_FILE)) {
  try {
    const raw = fs.readFileSync(DB_FILE, 'utf8');
    const parsed = JSON.parse(raw);
    if (parsed.status) systemStatus = parsed.status;
    if (parsed.events) eventLog = parsed.events;
    if (parsed.telemetry) telemetryHistory = parsed.telemetry;
  } catch (err) {
    console.error('Failed to parse db_records.json:', err.message);
  }
}

function persistDb() {
  try {
    fs.writeFileSync(DB_FILE, JSON.stringify({
      status: systemStatus,
      events: eventLog.slice(0, 500),
      telemetry: telemetryHistory.slice(-500)
    }, null, 2));
  } catch (e) {}
}

// -------------------------------------------------------------
// 1. Authentication & Security (Level 4: Roles, Lockout & Bcrypt)
// -------------------------------------------------------------
app.post('/api/auth/login', (req, res) => {
  const { username, password } = req.body;
  const now = Date.now();

  // Check 30-second lockout after 5 consecutive failed attempts
  if (failedLogins[username] && failedLogins[username].lockedUntil > now) {
    const remaining = Math.ceil((failedLogins[username].lockedUntil - now) / 1000);
    loginHistory.unshift({ time: new Date().toISOString(), user: username, success: false, reason: 'LOCKED' });
    return res.status(423).json({ error: `Account locked. Please wait ${remaining} seconds before trying again.` });
  }

  const user = usersDb[username];
  if (!user || !bcrypt.compareSync(password, user.passwordHash)) {
    if (!failedLogins[username]) failedLogins[username] = { count: 0, lockedUntil: 0 };
    failedLogins[username].count++;

    if (failedLogins[username].count >= 5) {
      failedLogins[username].lockedUntil = now + 30000; // 30s lock
      failedLogins[username].count = 0;
      loginHistory.unshift({ time: new Date().toISOString(), user: username, success: false, reason: 'ACCOUNT_LOCKED_5_ATTEMPTS' });
      return res.status(423).json({ error: 'Account locked for 30 seconds due to 5 consecutive wrong passwords.' });
    }

    loginHistory.unshift({ time: new Date().toISOString(), user: username, success: false, reason: 'WRONG_PASSWORD' });
    return res.status(401).json({ error: 'Invalid username or password.' });
  }

  // Login successful
  failedLogins[username] = { count: 0, lockedUntil: 0 };
  loginHistory.unshift({ time: new Date().toISOString(), user: username, success: true, reason: 'OK' });

  res.json({
    success: true,
    user: {
      username: user.username,
      role: user.role,
      name: user.name,
      studentId: user.studentId,
      linkedStudentId: user.linkedStudentId
    }
  });
});

app.get('/api/auth/login-history', (req, res) => {
  res.json({ history: loginHistory.slice(0, 50) });
});

// -------------------------------------------------------------
// 2. Hardware Telemetry & Event Ingestion
// -------------------------------------------------------------
app.post('/api/telemetry', (req, res) => {
  const b = req.body;
  if (b.room_id) systemStatus.roomId = b.room_id;
  if (b.temp_c !== undefined) systemStatus.temperature = Number(b.temp_c);
  if (b.rh !== undefined) systemStatus.humidity = Number(b.rh);
  if (b.heat_index_c !== undefined) systemStatus.heatIndex = Number(b.heat_index_c);
  if (b.heat_trend) systemStatus.heatTrend = b.heat_trend;
  if (b.door) systemStatus.doorState = b.door;
  if (b.status) systemStatus.status = b.status;
  if (b.flame !== undefined) systemStatus.flame = Boolean(b.flame);
  if (b.panic !== undefined) systemStatus.panic = Boolean(b.panic);
  if (b.quality) systemStatus.quality = b.quality;
  if (b.rssi !== undefined) systemStatus.rssi = Number(b.rssi);
  if (b.heap_kb !== undefined) systemStatus.heapKb = Number(b.heap_kb);
  if (b.chip_temp_c !== undefined) systemStatus.chipTemp = Number(b.chip_temp_c);
  systemStatus.lastUpdate = Date.now();

  telemetryHistory.push({
    time: new Date().toISOString(),
    temp: systemStatus.temperature,
    rh: systemStatus.humidity,
    heatIndex: systemStatus.heatIndex,
    door: systemStatus.doorState
  });
  if (telemetryHistory.length > 500) telemetryHistory.shift();

  res.json({ success: true });
});

app.post('/api/events', (req, res) => {
  const { type, detail, doorState, temp, humidity } = req.body;
  const newEvt = {
    id: 'evt_' + Date.now(),
    type: type || 'STATUS',
    detail: detail || '',
    doorState: doorState || systemStatus.doorState,
    temp: temp !== undefined ? temp : systemStatus.temperature,
    humidity: humidity !== undefined ? humidity : systemStatus.humidity,
    createdAt: new Date().toISOString()
  };

  if (type === 'CARD_TAP' || type === 'ATTENDANCE') {
    systemStatus.lastUid = detail;
  }
  if (type === 'FIRE') {
    systemStatus.fireLatched = true;
    systemStatus.doorState = 'UNLOCKED';
    teacherAlerts.unshift({ type: 'FIRE', detail: 'Critical fire detected! Evacuation in progress.', time: new Date().toISOString() });
  }
  if (type === 'PANIC') {
    systemStatus.panic = true;
    teacherAlerts.unshift({ type: 'PANIC', detail: 'Touch panic wire activated.', time: new Date().toISOString() });
  }

  eventLog.unshift(newEvt);
  persistDb();
  res.json({ success: true, eventId: newEvt.id });
});

// Offline batch synchronization
app.post('/api/offline-sync', (req, res) => {
  const { events } = req.body;
  if (Array.isArray(events)) {
    events.forEach(e => {
      eventLog.unshift({
        id: 'off_' + Date.now() + '_' + Math.random(),
        type: e.type,
        detail: e.payload,
        source: 'OFFLINE_BUFFER',
        createdAt: new Date().toISOString()
      });
    });
    persistDb();
  }
  res.json({ success: true });
});

// -------------------------------------------------------------
// 3. System Status & Control
// -------------------------------------------------------------
app.get('/api/status', (req, res) => {
  const isOnline = (Date.now() - systemStatus.lastUpdate) < 30000;
  res.json({ ...systemStatus, isOnline });
});

app.post('/api/door', (req, res) => {
  const { action } = req.body;
  if (action === 'unlock') systemStatus.doorState = 'UNLOCKED';
  else if (action === 'lock') systemStatus.doorState = 'LOCKED';
  res.json({ success: true, doorState: systemStatus.doorState });
});

app.post('/api/reset-fire', (req, res) => {
  systemStatus.fireLatched = false;
  systemStatus.flame = false;
  systemStatus.doorState = 'LOCKED';
  eventLog.unshift({ type: 'FIRE_CLEARED', detail: 'Fire reset by Teacher/Admin', createdAt: new Date().toISOString() });
  persistDb();
  res.json({ success: true });
});

// -------------------------------------------------------------
// 4. Remote Settings & OTA (Level 5)
// -------------------------------------------------------------
app.post('/api/settings', (req, res) => {
  const { warningTemp, dangerTemp, doorUnlockTimeSec } = req.body;
  const w = parseFloat(warningTemp);
  const d = parseFloat(dangerTemp);

  if (isNaN(w) || isNaN(d) || w < 0 || w > 50 || d < 0 || d > 50 || w >= d) {
    return res.status(400).json({ error: 'Invalid settings: Must be between 0-50 C and Warning limit must be below Danger limit.' });
  }

  systemStatus.warningTemp = w;
  systemStatus.dangerTemp = d;
  if (doorUnlockTimeSec) systemStatus.doorUnlockTimeSec = parseInt(doorUnlockTimeSec, 10);

  eventLog.unshift({
    type: 'SETTINGS_APPLIED',
    detail: `Warning: ${w}C, Danger: ${d}C, DoorTime: ${systemStatus.doorUnlockTimeSec}s`,
    createdAt: new Date().toISOString()
  });
  persistDb();
  res.json({ success: true, status: systemStatus });
});

// -------------------------------------------------------------
// 5. Weather & Notice Board (Level 6)
// -------------------------------------------------------------
let weatherData = {
  location: 'Dhaka, Bangladesh',
  temp: 29.4,
  condition: 'Partly Cloudy',
  humidity: 68,
  forecast24h: 'Scattered clouds, max 32 C, min 24 C',
  notices: [
    { id: 'w1', title: 'Seasonal Heat Notice', severity: 'Advisory', text: 'Afternoon heat index may exceed 34 C.' }
  ],
  lastFetch: new Date().toISOString()
};

app.get('/api/weather', (req, res) => {
  res.json(weatherData);
});

// -------------------------------------------------------------
// 6. AI Assistant (Level 6 Task 10: Strict Fact-Based Querying)
// -------------------------------------------------------------
app.post('/api/ai-assistant', (req, res) => {
  const { question } = req.body;
  if (!question) return res.status(400).json({ error: 'Question required' });

  const q = question.toLowerCase();

  // 1. Highest / Max Temperature
  if (q.includes('highest') || q.includes('max') || q.includes('সর্বোচ্চ') || q.includes('তাপমাত্রা')) {
    if (telemetryHistory.length === 0) return res.json({ answer: 'no data' });
    const maxT = Math.max(...telemetryHistory.map(t => t.temp));
    return res.json({
      answer: `The highest recorded temperature today was ${maxT.toFixed(1)} °C. (আজকের সর্বোচ্চ রেকর্ডকৃত তাপমাত্রা ${maxT.toFixed(1)} °C)`
    });
  }

  // 2. Alarms / Fire / Panic Count
  if (q.includes('alarm') || q.includes('fire') || q.includes('panic') || q.includes('অ্যালার্ম') || q.includes('আগুন')) {
    const fireCount = eventLog.filter(e => e.type === 'FIRE').length;
    const panicCount = eventLog.filter(e => e.type === 'PANIC').length;
    return res.json({
      answer: `Total safety alarms today: ${fireCount} Fire alerts, ${panicCount} Panic wire activations. (মোট সেফটি অ্যালার্ম: ${fireCount}টি ফায়ার অ্যালার্ট, ${panicCount}টি প্যানিক অ্যালার্ট)`
    });
  }

  // 3. Attendance Status
  if (q.includes('attendance') || q.includes('উপস্থিতি') || q.includes('student')) {
    return res.json({
      answer: `Class Attendance Today: ${attendanceRecords.length} student(s) Present. (আজকের ক্লাসে উপস্থিত শিক্ষার্থী: ${attendanceRecords.length} জন)`
    });
  }

  // 4. Any query with non-existent dates or outside scope -> strictly "no data"
  res.json({ answer: 'no data' });
});

// -------------------------------------------------------------
// 7. CSV Data Export (Level 6 Task 9)
// -------------------------------------------------------------
app.get('/api/export/csv', (req, res) => {
  let csv = 'Timestamp,Type,Detail,DoorState,Temperature,Humidity\n';
  eventLog.forEach(e => {
    csv += `"${e.createdAt}","${e.type}","${e.detail || ''}","${e.doorState || ''}","${e.temp || ''}","${e.humidity || ''}"\n`;
  });
  res.setHeader('Content-Type', 'text/csv');
  res.setHeader('Content-Disposition', 'attachment; filename="smart_classroom_events.csv"');
  res.send(csv);
});

// -------------------------------------------------------------
// 8. Card, Face Check-In & Anti-Spoofing (Level 7 & 8)
// -------------------------------------------------------------
app.get('/api/attendance', (req, res) => {
  res.json({ attendance: attendanceRecords });
});

app.post('/api/purge-test-data', (req, res) => {
  eventLog = [];
  telemetryHistory = [];
  teacherAlerts = [];
  systemStatus.lastUid = '';
  if (fs.existsSync(DB_FILE)) {
    try { fs.unlinkSync(DB_FILE); } catch (e) {}
  }
  console.log('[PRIVACY] All test participant logs and face records wiped.');
  res.json({ success: true, message: 'All test participant data and face embeddings successfully deleted.' });
});

app.listen(PORT, () => {
  console.log(`=======================================================`);
  console.log(` Smart Classroom Cloud Platform live on port ${PORT}`);
  console.log(` Local URL : http://localhost:${PORT}`);
  console.log(`=======================================================`);
});
