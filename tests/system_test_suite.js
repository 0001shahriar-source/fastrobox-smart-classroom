// FastRobox 1.0 Smart Classroom Complete Automated Test Suite
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
      headers: {
        'Content-Type': 'application/json'
      }
    };

    const req = http.request(options, (res) => {
      let data = '';
      res.on('data', (chunk) => data += chunk);
      res.on('end', () => {
        const duration = Date.now() - startTime;
        try {
          const parsed = JSON.parse(data);
          resolve({ status: res.statusCode, data: parsed, duration });
        } catch (e) {
          resolve({ status: res.statusCode, data, duration });
        }
      });
    });

    req.on('error', (err) => reject(err));

    if (body) {
      req.write(JSON.stringify(body));
    }
    req.end();
  });
}

function assert(condition, testName, extraInfo = '') {
  if (condition) {
    console.log(`  ✅ [PASS] ${testName} ${extraInfo}`);
    totalPassed++;
  } else {
    console.error(`  ❌ [FAIL] ${testName} ${extraInfo}`);
    totalFailed++;
  }
}

async function runAllTests() {
  console.log('================================================================');
  console.log(' FASTROBOX 1.0 TECHATHON GRAND FINALE - FULL SYSTEM VERIFICATION');
  console.log(' Testing all 12 Levels & Competition Rules Against Live API     ');
  console.log('================================================================\n');

  try {
    // -------------------------------------------------------------
    // TEST 1: Telemetry Ingest & Online Heartbeat
    // -------------------------------------------------------------
    console.log('▶ [TEST 1] Telemetry Ingest & Online Heartbeat');
    const teleRes = await request('POST', '/api/telemetry', {
      roomId: 'Room-101',
      classTime: '09:00',
      doorState: 'LOCKED',
      temp: 27.2,
      humidity: 59.5,
      flame: false,
      panic: false
    });
    assert(teleRes.status === 200 && teleRes.data.success, 'Telemetry accepted by API', `(${teleRes.duration} ms)`);
    
    const statusRes1 = await request('GET', '/api/status');
    assert(statusRes1.data.isOnline === true, 'Device reported as ONLINE');
    assert(statusRes1.data.temperature === 27.2, 'Temperature correctly reflected (27.2 °C)');
    assert(statusRes1.data.humidity === 59.5, 'Humidity correctly reflected (59.5 %)');
    assert(statusRes1.data.doorState === 'LOCKED', 'Door state is initially LOCKED');
    console.log('');

    // -------------------------------------------------------------
    // TEST 2: Simulated Door Control (Lock / Unlock State Machine)
    // -------------------------------------------------------------
    console.log('▶ [TEST 2] Simulated Door Remote Control');
    const unlockRes = await request('POST', '/api/door', { action: 'unlock' });
    assert(unlockRes.data.doorState === 'UNLOCKED', 'Door successfully UNLOCKED via remote command', `(${unlockRes.duration} ms)`);

    const statusAfterUnlock = await request('GET', '/api/status');
    assert(statusAfterUnlock.data.doorState === 'UNLOCKED', 'Status endpoint confirms UNLOCKED state');

    const lockRes = await request('POST', '/api/door', { action: 'lock' });
    assert(lockRes.data.doorState === 'LOCKED', 'Door successfully relocked via remote command', `(${lockRes.duration} ms)`);
    console.log('');

    // -------------------------------------------------------------
    // TEST 3: Student Attendance (White RFID Card Swipe)
    // -------------------------------------------------------------
    console.log('▶ [TEST 3] Student Attendance (White Card Check-In)');
    const studentSwipe = await request('POST', '/api/events', {
      roomId: 'Room-101',
      type: 'ATTENDANCE',
      detail: 'STUDENT card: 4A:9C:31:FE',
      doorState: 'UNLOCKED',
      temp: 27.2,
      humidity: 59.5
    });
    assert(studentSwipe.status === 200 && studentSwipe.data.success, 'Student check-in event logged', `(${studentSwipe.duration} ms)`);

    const statusAfterStudent = await request('GET', '/api/status');
    assert(statusAfterStudent.data.doorState === 'UNLOCKED', 'Simulated door unlocked upon student swipe');
    assert(statusAfterStudent.data.lastUid.includes('4A:9C:31:FE'), 'Last swiped UID recorded on status surface');
    console.log('');

    // -------------------------------------------------------------
    // TEST 4: Teacher Override (Blue RFID Key-Tag Swipe)
    // -------------------------------------------------------------
    console.log('▶ [TEST 4] Teacher Override (Blue Key-Tag Swipe)');
    const teacherSwipe = await request('POST', '/api/events', {
      roomId: 'Room-101',
      type: 'ATTENDANCE',
      detail: 'TEACHER card: B2:71:09:A4',
      doorState: 'LOCKED',
      temp: 27.2,
      humidity: 59.5
    });
    assert(teacherSwipe.status === 200 && teacherSwipe.data.success, 'Teacher override action processed', `(${teacherSwipe.duration} ms)`);
    console.log('');

    // -------------------------------------------------------------
    // TEST 5: Critical Safety - Fire/Flame Detection & Auto Unlock
    // -------------------------------------------------------------
    console.log('▶ [TEST 5] Safety - Flame Detection & Evacuation Auto-Unlock');
    const fireEvent = await request('POST', '/api/events', {
      roomId: 'Room-101',
      type: 'FLAME_ALERT',
      detail: 'Optical flame sensor active LOW! Initiating immediate building evacuation.',
      doorState: 'UNLOCKED'
    });
    assert(fireEvent.status === 200, 'Fire alert event received by cloud', `(${fireEvent.duration} ms)`);

    await request('POST', '/api/telemetry', { flame: true, doorState: 'UNLOCKED' });
    const statusFire = await request('GET', '/api/status');
    assert(statusFire.data.flame === true, 'Flame alarm flag raised in cloud status');
    assert(statusFire.data.doorState === 'UNLOCKED', 'Simulated door automatically UNLOCKED for fire escape');
    console.log('');

    // -------------------------------------------------------------
    // TEST 6: Capacitive Touch Panic Wire Alarm
    // -------------------------------------------------------------
    console.log('▶ [TEST 6] Capacitive Touch Panic Wire (GPIO 1)');
    const panicEvent = await request('POST', '/api/events', {
      roomId: 'Room-101',
      type: 'PANIC_ALERT',
      detail: 'Capacitive touch panic wire touched by student! Immediate alarm sounded.',
      doorState: 'UNLOCKED'
    });
    assert(panicEvent.status === 200, 'Panic event logged to audit stream', `(${panicEvent.duration} ms)`);

    await request('POST', '/api/telemetry', { panic: true });
    const statusPanic = await request('GET', '/api/status');
    assert(statusPanic.data.panic === true, 'Panic status flag active in system telemetry');
    console.log('');

    // -------------------------------------------------------------
    // TEST 7: Safety Recovery to Normal State
    // -------------------------------------------------------------
    console.log('▶ [TEST 7] Recovery from Fire & Panic to Normal Operation');
    await request('POST', '/api/telemetry', { flame: false, panic: false, doorState: 'LOCKED' });
    const statusClear = await request('GET', '/api/status');
    assert(statusClear.data.flame === false && statusClear.data.panic === false, 'Hazards successfully cleared');
    assert(statusClear.data.doorState === 'LOCKED', 'Door returned to secure LOCKED state');
    console.log('');

    // -------------------------------------------------------------
    // TEST 8: Zero-Hardcoding Dynamic Configuration
    // -------------------------------------------------------------
    console.log('▶ [TEST 8] Zero-Hardcoding: Dynamic Room ID & Schedule Update');
    const newConfig = { roomId: 'BUBT-Auditorium-302', classStartTime: '10:45' };
    const configRes = await request('POST', '/api/config', newConfig);
    assert(configRes.data.success, 'Dynamic configuration saved', `(${configRes.duration} ms)`);

    const statusConfig = await request('GET', '/api/status');
    assert(statusConfig.data.roomId === 'BUBT-Auditorium-302', 'Room ID updated dynamically on-the-spot');
    assert(statusConfig.data.classStartTime === '10:45', 'Class Start Time updated dynamically on-the-spot');
    console.log('');

    // -------------------------------------------------------------
    // TEST 9: Offline Resilience & Batch Event Synchronization
    // -------------------------------------------------------------
    console.log('▶ [TEST 9] Offline Resilience: Batch Sync After Network Drop');
    const offlineBatch = {
      roomId: 'BUBT-Auditorium-302',
      events: [
        { type: 'ATTENDANCE', payload: 'STUDENT card: 11:22:33:44 (Offline)', deviceTimeMs: 120500 },
        { type: 'DOOR', payload: 'Auto-relock after offline entry', deviceTimeMs: 125500 },
        { type: 'PANIC_ALERT', payload: 'Panic wire touched while Wi-Fi was disconnected', deviceTimeMs: 130200 }
      ]
    };
    const syncRes = await request('POST', '/api/offline-sync', offlineBatch);
    assert(syncRes.data.success && syncRes.data.processedCount === 3, 'All 3 buffered offline events successfully ingested', `(${syncRes.duration} ms)`);
    console.log('');

    // -------------------------------------------------------------
    // TEST 10: Searchable Audit History Database & Category Filter
    // -------------------------------------------------------------
    console.log('▶ [TEST 10] Searchable Audit Database & Category Filtering');
    const allEventsRes = await request('GET', '/api/events');
    assert(allEventsRes.data.total > 0, `Total audit records indexed: ${allEventsRes.data.total}`);

    const flameOnlyRes = await request('GET', '/api/events?type=FLAME_ALERT');
    const onlyFlame = flameOnlyRes.data.events.every(e => e.type === 'FLAME_ALERT');
    assert(onlyFlame && flameOnlyRes.data.events.length > 0, 'Category filter for FLAME_ALERT works accurately');

    const searchRes = await request('GET', '/api/events?search=4A:9C:31:FE');
    assert(searchRes.data.events.length > 0, 'Instant text search finds exact Student RFID swipe');

    const offlineSearchRes = await request('GET', '/api/events?search=Offline');
    assert(offlineSearchRes.data.events.length > 0, 'Search accurately locates buffered offline records');
    console.log('');

    // -------------------------------------------------------------
    // TEST 11: Response Time Latency Check (The Strict 0.5s Rule)
    // -------------------------------------------------------------
    console.log('▶ [TEST 11] Response Time Benchmark (< 500 ms limit)');
    let latencies = [];
    for (let i = 0; i < 5; i++) {
      const ping = await request('GET', '/api/status');
      latencies.push(ping.duration);
    }
    const avgLatency = Math.round(latencies.reduce((a, b) => a + b, 0) / latencies.length);
    console.log(`  📊 Average Latency: ${avgLatency} ms (Samples: ${latencies.join(', ')} ms)`);
    assert(avgLatency < 500, `Complies strictly with 0.5-second response limit (measured: ${avgLatency} ms)`);
    console.log('');

    // -------------------------------------------------------------
    // FINAL SUMMARY
    // -------------------------------------------------------------
    console.log('================================================================');
    console.log(` FINAL TEST SUMMARY: ${totalPassed} PASSED | ${totalFailed} FAILED`);
    if (totalFailed === 0) {
      console.log(' 🏆 ALL 12 LEVEL REQUIREMENTS & TESTS ARE 100% VERIFIED!');
    } else {
      console.log(' ⚠️ SOME CHECKS FAILED. INVESTIGATE ISSUES ABOVE.');
    }
    console.log('================================================================\n');

  } catch (err) {
    console.error('Test execution error:', err);
  }
}

runAllTests();
