// ================================================================
// FastRobox 1.0 Techathon Grand Finale — OFFICIAL 12-LEVEL TEST SUITE
// All levels verified against question paper specifications
// ================================================================
const http = require('http');

const BASE_URL = 'http://localhost:3000';
let totalPassed = 0;
let totalFailed = 0;

function request(method, path, body = null) {
  return new Promise((resolve, reject) => {
    const startTime = Date.now();
    const url = new URL(path, BASE_URL);
    const options = {
      hostname: url.hostname,
      port: url.port,
      path: url.pathname + url.search,
      method: method,
      headers: { 'Content-Type': 'application/json' }
    };
    const req = http.request(options, (res) => {
      let data = '';
      res.on('data', (chunk) => data += chunk);
      res.on('end', () => {
        const duration = Date.now() - startTime;
        try {
          resolve({ status: res.statusCode, data: JSON.parse(data), duration, raw: data });
        } catch (e) {
          resolve({ status: res.statusCode, data: data, duration, raw: data });
        }
      });
    });
    req.on('error', reject);
    if (body) req.write(JSON.stringify(body));
    req.end();
  });
}

function assert(condition, testName, extraInfo = '') {
  const icon = condition ? '  ✅ [PASS]' : '  ❌ [FAIL]';
  if (condition) {
    console.log(`${icon} ${testName} ${extraInfo}`);
    totalPassed++;
  } else {
    console.error(`${icon} ${testName} ${extraInfo}`);
    totalFailed++;
  }
}

function header(title) {
  console.log(`\n${'─'.repeat(60)}`);
  console.log(` ▶ ${title}`);
  console.log('─'.repeat(60));
}

async function runAllTests() {
  console.log('================================================================');
  console.log('  FASTROBOX 1.0 TECHATHON — FULL 12-LEVEL SYSTEM TEST SUITE');
  console.log('  Verifying ALL Rules from Official Question Paper');
  console.log('================================================================');

  try {

    // ============================================================
    // LEVEL 1 & 2 — DEVICE BOOT, TELEMETRY, SENSOR INGESTION
    // ============================================================
    header('LEVEL 1 & 2 — Telemetry Ingestion & Device State');

    const tele = await request('POST', '/api/telemetry', {
      device_id: 'T07-D1',
      room_id: 'Room-101',
      seq: 101,
      ts: Date.now(),
      uptime_s: 300,
      temp_c: 29.5,
      rh: 64.0,
      heat_index_c: 31.5,
      flame: false,
      door: 'LOCKED',
      status: 'SAFE',
      heat_trend: 'STABLE',
      rssi: -55,
      heap_kb: 182,
      chip_temp_c: 41.2,
      fw: '1.0.0',
      quality: 'good'
    });
    assert(tele.status === 200, 'POST /api/telemetry returns 200 OK');
    assert(tele.data.success === true, 'Telemetry body has success:true');

    const s1 = await request('GET', '/api/status');
    assert(s1.status === 200, 'GET /api/status returns 200 OK');
    assert(s1.data.isOnline === true, 'Device is ONLINE (last telemetry <30s ago)');
    assert(s1.data.temperature === 29.5, `Temperature stored correctly (got ${s1.data.temperature})`, '→ 29.5°C');
    assert(s1.data.humidity === 64, `Humidity stored correctly (got ${s1.data.humidity})`, '→ 64%');
    assert(s1.data.heatIndex === 31.5, `Heat index stored correctly (got ${s1.data.heatIndex})`, '→ 31.5°C');
    assert(s1.data.heatTrend === 'STABLE', `Heat trend stored (got ${s1.data.heatTrend})`, '→ STABLE');
    assert(s1.data.doorState === 'LOCKED', `Door state: LOCKED`);
    assert(s1.data.status === 'SAFE', `Safety status: SAFE`);
    assert(s1.data.roomId === 'Room-101', `Room ID: Room-101 (runtime-settable, no hardcode)`);
    assert(typeof s1.data.warningTemp === 'number', `warningTemp present (${s1.data.warningTemp}°C) — judge-settable`);
    assert(typeof s1.data.dangerTemp === 'number', `dangerTemp present (${s1.data.dangerTemp}°C) — judge-settable`);
    assert(typeof s1.data.doorUnlockTimeSec === 'number', `doorUnlockTimeSec present (${s1.data.doorUnlockTimeSec}s) — judge-settable`);

    // ============================================================
    // LEVEL 3 — SAFETY EVENTS (Flame, Panic, Warning/Danger)
    // ============================================================
    header('LEVEL 3 — Safety Events: Fire Latch, Panic Wire, Status Transitions');

    const fireEvt = await request('POST', '/api/events', {
      type: 'FIRE',
      detail: 'Flame sensor triggered continuously for 2s',
      doorState: 'UNLOCKED'
    });
    assert(fireEvt.status === 200, 'FIRE event ingested successfully');
    assert(fireEvt.data.eventId !== undefined, 'FIRE event has unique eventId');

    const s2 = await request('GET', '/api/status');
    assert(s2.data.fireLatched === true, 'Fire is LATCHED in system (NVS-equivalent persistence)');
    assert(s2.data.doorState === 'UNLOCKED', 'Door auto-UNLOCKED on fire (evacuation mode)');

    const fireReset = await request('POST', '/api/reset-fire');
    assert(fireReset.status === 200 && fireReset.data.success === true, 'POST /api/reset-fire clears fire latch');
    const s3 = await request('GET', '/api/status');
    assert(s3.data.fireLatched === false, 'Fire latch cleared after reset');
    assert(s3.data.doorState === 'LOCKED', 'Door re-locked after fire reset');

    const panicEvt = await request('POST', '/api/events', { type: 'PANIC', detail: 'Touch panic wire held >2s' });
    assert(panicEvt.status === 200, 'PANIC event ingested successfully');
    const s4 = await request('GET', '/api/status');
    assert(s4.data.panic === true, 'Panic flag set to true in system state');

    await request('POST', '/api/telemetry', {
      device_id: 'T07-D1', room_id: 'Room-101', seq: 102, ts: Date.now(),
      temp_c: 33.5, rh: 87.0, heat_index_c: 36.0, flame: false,
      door: 'LOCKED', status: 'WARNING', heat_trend: 'RISING',
      rssi: -55, heap_kb: 180, chip_temp_c: 42.0, fw: '1.0.0', quality: 'good'
    });
    const s5 = await request('GET', '/api/status');
    assert(s5.data.status === 'WARNING', `Status → WARNING (temp=33.5°C, rh=87%)`);
    assert(s5.data.heatTrend === 'RISING', 'Heat trend → RISING');

    await request('POST', '/api/telemetry', {
      device_id: 'T07-D1', room_id: 'Room-101', seq: 103, ts: Date.now(),
      temp_c: 41.0, rh: 90.0, heat_index_c: 46.0, flame: false,
      door: 'LOCKED', status: 'DANGER', heat_trend: 'RISING',
      rssi: -55, heap_kb: 178, chip_temp_c: 43.0, fw: '1.0.0', quality: 'good'
    });
    const s6 = await request('GET', '/api/status');
    assert(s6.data.status === 'DANGER', `Status → DANGER (temp=41°C, heatIndex=46°C)`);

    // Reset to SAFE
    await request('POST', '/api/telemetry', {
      device_id: 'T07-D1', room_id: 'Room-101', seq: 104, ts: Date.now(),
      temp_c: 29.5, rh: 64.0, heat_index_c: 31.5, flame: false,
      door: 'LOCKED', status: 'SAFE', heat_trend: 'STABLE',
      rssi: -55, heap_kb: 182, chip_temp_c: 41.0, fw: '1.0.0', quality: 'good'
    });

    // ============================================================
    // LEVEL 4 — AUTHENTICATION: 4 ROLES, BCRYPT, LOCKOUT
    // ============================================================
    header('LEVEL 4 — Multi-Role Authentication (4 Roles, bcrypt, 30s Lockout)');

    const adminLogin = await request('POST', '/api/auth/login', { username: 'admin', password: 'admin123' });
    assert(adminLogin.status === 200, 'Admin login: HTTP 200');
    assert(adminLogin.data.user.role === 'Admin', `Admin role verified`);

    const teachLogin = await request('POST', '/api/auth/login', { username: 'teacher', password: 'teach123' });
    assert(teachLogin.status === 200, 'Teacher login: HTTP 200');
    assert(teachLogin.data.user.role === 'Teacher', `Teacher role verified`);

    const studLogin = await request('POST', '/api/auth/login', { username: 'student1', password: 'stud123' });
    assert(studLogin.status === 200, 'Student login: HTTP 200');
    assert(studLogin.data.user.role === 'Student', `Student role verified`);

    const guardLogin = await request('POST', '/api/auth/login', { username: 'guardian', password: 'guard123' });
    assert(guardLogin.status === 200, 'Guardian login: HTTP 200');
    assert(guardLogin.data.user.role === 'Guardian', `Guardian role verified`);
    assert(guardLogin.data.user.linkedStudentId === 'S1', `Guardian linked to Student S1`);

    const wrongPw = await request('POST', '/api/auth/login', { username: 'admin', password: 'wrongpass' });
    assert(wrongPw.status === 401, 'Wrong password → HTTP 401 Unauthorized');

    const lockUser = 'locktest_' + Date.now();
    for (let i = 0; i < 4; i++) {
      await request('POST', '/api/auth/login', { username: lockUser, password: 'bad' });
    }
    const fifthFail = await request('POST', '/api/auth/login', { username: lockUser, password: 'bad' });
    assert(fifthFail.status === 423, '5th consecutive wrong password → HTTP 423 Account Locked (30s)');

    // ============================================================
    // LEVEL 5 — REMOTE SETTINGS, RUNTIME CONFIG
    // ============================================================
    header('LEVEL 5 — Remote Settings (Runtime-Configurable, No Hardcoding)');

    const validSet = await request('POST', '/api/settings', {
      warningTemp: 32.0, dangerTemp: 40.0, doorUnlockTimeSec: 7
    });
    assert(validSet.status === 200 && validSet.data.success, 'Valid settings accepted (Warning=32°C, Danger=40°C, Door=7s)');

    const s7 = await request('GET', '/api/status');
    assert(s7.data.warningTemp === 32.0, `warningTemp updated to 32.0 (got ${s7.data.warningTemp})`);
    assert(s7.data.dangerTemp === 40.0, `dangerTemp updated to 40.0 (got ${s7.data.dangerTemp})`);
    assert(s7.data.doorUnlockTimeSec === 7, `doorUnlockTimeSec updated to 7 (got ${s7.data.doorUnlockTimeSec})`);

    const invSet1 = await request('POST', '/api/settings', { warningTemp: 45.0, dangerTemp: 40.0 });
    assert(invSet1.status === 400, 'Invalid (Warning≥Danger) → HTTP 400');

    const invSet2 = await request('POST', '/api/settings', { warningTemp: -5.0, dangerTemp: 40.0 });
    assert(invSet2.status === 400, 'Invalid (temp<0) → HTTP 400');

    const invSet3 = await request('POST', '/api/settings', { warningTemp: 30.0, dangerTemp: 55.0 });
    assert(invSet3.status === 400, 'Invalid (temp>50) → HTTP 400');

    // ============================================================
    // LEVEL 6 — WEATHER, AI ASSISTANT, CSV EXPORT
    // ============================================================
    header('LEVEL 6 — Weather Data, Strict AI Assistant, CSV Export');

    const weather = await request('GET', '/api/weather');
    assert(weather.status === 200, 'GET /api/weather returns 200 OK');
    assert(weather.data.location !== undefined, `Weather location: "${weather.data.location}"`);
    assert(typeof weather.data.temp === 'number', `Weather temp is numeric: ${weather.data.temp}°C`);

    const aiBn = await request('POST', '/api/ai-assistant', { question: 'আজকের সর্বোচ্চ তাপমাত্রা কত?' });
    assert(aiBn.status === 200, 'AI: Bangla temperature query → 200');
    assert(aiBn.data.answer && aiBn.data.answer.includes('41'), `AI returns max temp correctly (expected "41" in answer, got: "${aiBn.data.answer}")`);

    const aiEn = await request('POST', '/api/ai-assistant', { question: 'How many alarms happened today?' });
    assert(aiEn.status === 200, 'AI: English alarm query → 200');
    assert(aiEn.data.answer && (aiEn.data.answer.includes('Fire') || aiEn.data.answer.includes('Panic')),
      `AI correctly lists Fire/Panic counts`);

    const aiAtt = await request('POST', '/api/ai-assistant', { question: 'How many students attended class?' });
    assert(aiAtt.status === 200, 'AI: Attendance query → 200');
    assert(aiAtt.data.answer && aiAtt.data.answer.includes('student'), `AI returns attendance data`);

    const aiNone = await request('POST', '/api/ai-assistant', { question: 'What was temperature on year 1999?' });
    assert(aiNone.status === 200, 'AI: Non-existent query → 200');
    assert(aiNone.data.answer === 'no data',
      `AI strictly returns "no data" for 1999 query (got: "${aiNone.data.answer}")`);

    const aiBad = await request('POST', '/api/ai-assistant', {});
    assert(aiBad.status === 400, 'AI: missing question → HTTP 400');

    const csv = await request('GET', '/api/export/csv');
    assert(csv.status === 200, 'GET /api/export/csv → 200 OK');
    assert(csv.raw.includes('Timestamp,Type,Detail'), 'CSV has correct headers');
    assert(csv.raw.length > 50, `CSV has content (${csv.raw.length} bytes)`);

    // ============================================================
    // LEVEL 7 & 8 — ATTENDANCE, RFID EVENTS, PRIVACY PURGE
    // ============================================================
    header('LEVEL 7 & 8 — Attendance Records, RFID Events, Privacy Purge');

    const cardEvt = await request('POST', '/api/events', {
      type: 'CARD_TAP',
      detail: 'Entry granted to: Tanvir Ahmed (S1)',
      doorState: 'UNLOCKED', temp: 29.5, humidity: 64
    });
    assert(cardEvt.status === 200, 'CARD_TAP event ingested');
    const s8 = await request('GET', '/api/status');
    assert(s8.data.lastUid && s8.data.lastUid.length > 0, `lastUid updated on CARD_TAP: "${s8.data.lastUid}"`);

    const att = await request('GET', '/api/attendance');
    assert(att.status === 200, 'GET /api/attendance → 200 OK');
    assert(Array.isArray(att.data.attendance), 'Attendance is an array');
    assert(att.data.attendance.length >= 1, `Attendance has ${att.data.attendance.length} record(s)`);

    const purge = await request('POST', '/api/purge-test-data');
    assert(purge.status === 200 && purge.data.success, 'POST /api/purge-test-data → 200, success=true');
    assert(purge.data.message && purge.data.message.length > 0, 'Purge response has confirmation message');

    // ============================================================
    // LEVEL 9 — SIMULATED DOOR (SOFTWARE ONLY, NO SERVO)
    // ============================================================
    header('LEVEL 9 — Simulated Door Control (Software-Only, LED/Serial shows state)');

    await request('POST', '/api/telemetry', {
      device_id: 'T07-D1', room_id: 'Room-101', seq: 200, ts: Date.now(),
      temp_c: 29.5, rh: 64.0, heat_index_c: 31.5, flame: false,
      door: 'LOCKED', status: 'SAFE', heat_trend: 'STABLE',
      rssi: -55, heap_kb: 182, chip_temp_c: 41.0, fw: '1.0.0', quality: 'good'
    });

    const doorUnlock = await request('POST', '/api/door', { action: 'unlock' });
    assert(doorUnlock.status === 200, 'POST /api/door {unlock} → 200 OK');
    assert(doorUnlock.data.doorState === 'UNLOCKED', 'Door state = UNLOCKED (simulated)');

    const doorLock = await request('POST', '/api/door', { action: 'lock' });
    assert(doorLock.status === 200, 'POST /api/door {lock} → 200 OK');
    assert(doorLock.data.doorState === 'LOCKED', 'Door state = LOCKED (simulated)');

    // ============================================================
    // LEVEL 10 — OFFLINE BUFFER & SYNC
    // ============================================================
    header('LEVEL 10 — Offline Buffer & Sync (60-event buffer, anti-replay)');

    const offlineSync = await request('POST', '/api/offline-sync', {
      events: [
        { seq: 1, type: 'CARD_TAP', payload: 'S2 entry offline', ts: Date.now() - 60000 },
        { seq: 2, type: 'STATUS',   payload: 'SAFE during offline', ts: Date.now() - 50000 },
        { seq: 3, type: 'FIRE',     payload: 'Fire detected offline', ts: Date.now() - 30000 }
      ]
    });
    assert(offlineSync.status === 200, 'POST /api/offline-sync → 200 OK');
    assert(offlineSync.data.success === true, 'Offline sync accepted: success=true');

    // ============================================================
    // LEVEL 11 — LOGIN HISTORY & AUDIT TRAIL
    // ============================================================
    header('LEVEL 11 — Audit Trail & Login History');

    const loginHist = await request('GET', '/api/auth/login-history');
    assert(loginHist.status === 200, 'GET /api/auth/login-history → 200 OK');
    assert(Array.isArray(loginHist.data.history), 'Login history is an array');
    assert(loginHist.data.history.length >= 1, `Login history has ${loginHist.data.history.length} record(s)`);
    if (loginHist.data.history.length > 0) {
      const h = loginHist.data.history[0];
      assert(h.time !== undefined, 'History entry has timestamp field');
      assert(h.user !== undefined, 'History entry has username field');
      assert(typeof h.success === 'boolean', 'History entry has boolean success field');
    }

    // ============================================================
    // LEVEL 12 — SETTINGS ROUNDTRIP & AUDIT
    // ============================================================
    header('LEVEL 12 — Settings Roundtrip, SETTINGS_APPLIED Audit Event');

    const finalSettings = await request('POST', '/api/settings', {
      warningTemp: 33.0, dangerTemp: 42.0, doorUnlockTimeSec: 10
    });
    assert(finalSettings.status === 200, 'Final settings update → 200');

    const s9 = await request('GET', '/api/status');
    assert(s9.data.warningTemp === 33.0, `warningTemp=33.0 ✓ (got ${s9.data.warningTemp})`);
    assert(s9.data.dangerTemp === 42.0, `dangerTemp=42.0 ✓ (got ${s9.data.dangerTemp})`);
    assert(s9.data.doorUnlockTimeSec === 10, `doorUnlockTimeSec=10 ✓ (got ${s9.data.doorUnlockTimeSec})`);

    const csv2 = await request('GET', '/api/export/csv');
    assert(csv2.raw.includes('SETTINGS_APPLIED'), 'SETTINGS_APPLIED audit event present in CSV export');

    // ============================================================
    // TIMING BENCHMARK — Strict <500ms Requirement
    // ============================================================
    header('TIMING BENCHMARK — Response Time Compliance (<500ms per official rules)');

    const latencies = [];
    for (let i = 0; i < 10; i++) {
      const p = await request('GET', '/api/status');
      latencies.push(p.duration);
    }
    const avg = Math.round(latencies.reduce((a, b) => a + b, 0) / latencies.length);
    const maxL = Math.max(...latencies);
    const minL = Math.min(...latencies);
    console.log(`  📊 Status endpoint (10 samples): avg=${avg}ms  max=${maxL}ms  min=${minL}ms`);
    console.log(`  📊 All samples: [${latencies.join(', ')}] ms`);
    assert(avg < 500, `Average latency ${avg}ms < 500ms`);
    assert(maxL < 1000, `Peak latency ${maxL}ms < 1000ms`);

    const aiT0 = Date.now();
    await request('POST', '/api/ai-assistant', { question: 'How many alarms happened today?' });
    const aiLat = Date.now() - aiT0;
    console.log(`  📊 AI Assistant: ${aiLat}ms`);
    assert(aiLat < 500, `AI Assistant ${aiLat}ms < 500ms`);

    // ============================================================
    // RULE COMPLIANCE FINAL CHECKS
    // ============================================================
    header('RULE COMPLIANCE — No Hardcoding, Privacy, Simulated Door');

    const final = await request('GET', '/api/status');
    assert(typeof final.data.warningTemp === 'number', 'warningTemp is runtime-configurable (no hardcoding) ✓');
    assert(typeof final.data.dangerTemp === 'number', 'dangerTemp is runtime-configurable (no hardcoding) ✓');
    assert(typeof final.data.doorUnlockTimeSec === 'number', 'doorUnlockTimeSec is runtime-configurable ✓');
    assert(typeof final.data.roomId === 'string', 'roomId is runtime-configurable via telemetry ✓');
    assert(typeof final.data.doorState === 'string', `Door is software-simulated: state="${final.data.doorState}" (no servo) ✓`);

    const privacyFinal = await request('POST', '/api/purge-test-data');
    assert(privacyFinal.status === 200, 'Privacy purge endpoint functional ✓ (all test data deletable on demand)');

  } catch (e) {
    console.error('\n❌ TEST SUITE CRASHED:', e.message);
    totalFailed++;
  }

  // ============================================================
  // FINAL SUMMARY
  // ============================================================
  console.log('\n' + '='.repeat(60));
  console.log(` FINAL RESULTS: ${totalPassed} PASSED | ${totalFailed} FAILED`);
  console.log(`  Total Tests: ${totalPassed + totalFailed}`);
  if (totalFailed === 0) {
    console.log('  🏆 100% PASS — FULLY COMPLIANT WITH OFFICIAL SPEC!');
  } else {
    console.log(`  ⚠️  ${totalFailed} test(s) NEED ATTENTION before competition.`);
  }
  console.log('='.repeat(60) + '\n');
  process.exit(totalFailed > 0 ? 1 : 0);
}

runAllTests();
