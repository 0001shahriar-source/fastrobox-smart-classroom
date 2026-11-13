let currentLang = 'EN';
let currentUser = { username: 'admin', role: 'Admin', name: 'System Administrator' };
let debounceTimer = null;

const i18n = {
  EN: {
    mainTitle: 'SmartClassroom Cloud',
    subTitle: 'FastRobox 1.0 Techathon Grand Finale (BUBT)',
    doorTitle: 'Simulated Classroom Door',
    btnUnlock: 'Remote UNLOCK',
    btnLock: 'Remote LOCK',
    sensorTitle: 'Environment & Safety Sensors',
    tempTitle: 'TEMPERATURE',
    humTitle: 'HUMIDITY & TREND',
    flameTitle: 'FLAME SENSOR',
    panicTitle: 'TOUCH PANIC PAD',
    healthTitle: 'Device Health & Telemetry',
    aiTitle: 'Smart Classroom AI Assistant',
    aiDesc: "Ask questions in English or Bangla about today's temperature, alarms, or attendance.",
    historyTitle: 'Searchable Audit History & Event Records',
    historySub: 'Tamper-evident log of all RFID attendance, safety alerts, and door transitions',
    wipeData: 'Wipe Test Data',
    fireAlert: 'CRITICAL FIRE ALERT DETECTED!',
    panicAlert: 'TOUCH PANIC WIRE ACTIVATED!'
  },
  BN: {
    mainTitle: 'স্মার্ট ক্লাসরুম ক্লাউড প্ল্যাটফর্ম',
    subTitle: 'ফাস্টরোবক্স ১.০ টেকথন গ্র্যান্ড ফিনালে (বিইউবিটি)',
    doorTitle: 'সিমুলেটেড ক্লাসরুম দরজা',
    btnUnlock: 'রিমোট আনলক',
    btnLock: 'রিমোট লক',
    sensorTitle: 'পরিবেশ ও নিরাপত্তা সেন্সর',
    tempTitle: 'তাপমাত্রা',
    humTitle: 'আর্দ্রতা ও ট্রেন্ড',
    flameTitle: 'অগ্নি শনাক্তকরণ সেন্সর',
    panicTitle: 'টাচ প্যানিক প্যাড',
    healthTitle: 'ডিভাইস হেলথ ও টেলিমেট্রি',
    aiTitle: 'স্মার্ট ক্লাসরুম এআই সহকারী',
    aiDesc: 'আজকের তাপমাত্রা, অ্যালার্ম বা উপস্থিতি সম্পর্কে বাংলা বা ইংরেজিতে প্রশ্ন করুন।',
    historyTitle: 'অনুসন্ধানযোগ্য অডিট ইতিহাস ও ইভেন্ট রেকর্ড',
    historySub: 'সব আরএফআইডি উপস্থিতি, সুরক্ষা অ্যালার্ট এবং দরজার ট্রানজিশনের সুরক্ষিত লগ',
    wipeData: 'টেস্ট ডাটা মুছুন',
    fireAlert: 'জরুরি অগ্নিকাণ্ড শনাক্ত হয়েছে!',
    panicAlert: 'টাচ প্যানিক ওয়্যার সক্রিয় হয়েছে!'
  }
};

function toggleLanguage() {
  currentLang = (currentLang === 'EN') ? 'BN' : 'EN';
  applyLanguage();
}

function applyLanguage() {
  const t = i18n[currentLang];
  document.getElementById('t_mainTitle').textContent = t.mainTitle;
  document.getElementById('t_subTitle').textContent = t.subTitle;
  document.getElementById('t_doorTitle').textContent = t.doorTitle;
  document.getElementById('t_btnUnlock').textContent = t.btnUnlock;
  document.getElementById('t_btnLock').textContent = t.btnLock;
  document.getElementById('t_sensorTitle').textContent = t.sensorTitle;
  document.getElementById('t_tempTitle').textContent = t.tempTitle;
  document.getElementById('t_humTitle').textContent = t.humTitle;
  document.getElementById('t_flameTitle').textContent = t.flameTitle;
  document.getElementById('t_panicTitle').textContent = t.panicTitle;
  document.getElementById('t_healthTitle').textContent = t.healthTitle;
  document.getElementById('t_aiTitle').textContent = t.aiTitle;
  document.getElementById('t_aiDesc').textContent = t.aiDesc;
  document.getElementById('t_historyTitle').textContent = t.historyTitle;
  document.getElementById('t_historySub').textContent = t.historySub;
  document.getElementById('t_wipeData').textContent = t.wipeData;
  document.getElementById('t_fireAlert').textContent = t.fireAlert;
  document.getElementById('t_panicAlert').textContent = t.panicAlert;
  document.getElementById('langToggleBtn').textContent = (currentLang === 'EN') ? '🌐 বাংলা' : '🌐 English';
}

async function fetchStatus() {
  try {
    const res = await fetch('/api/status');
    const data = await res.json();

    // 1. Connection Indicator
    const connDot = document.getElementById('connDot');
    const connText = document.getElementById('connText');
    if (data.isOnline) {
      connDot.className = 'dot dot-online';
      connText.textContent = 'DEVICE ONLINE';
    } else {
      connDot.className = 'dot dot-offline';
      connText.textContent = 'DEVICE OFFLINE';
    }

    // 2. Door State & Visuals
    const doorBadge = document.getElementById('doorBadge');
    const doorIconBox = document.getElementById('doorIconBox');
    const doorEmoji = document.getElementById('doorEmoji');
    const doorHeadline = document.getElementById('doorStateHeadline');
    const doorSubtext = document.getElementById('doorSubtext');

    if (data.doorState === 'UNLOCKED') {
      doorBadge.className = 'badge badge-unlocked';
      doorBadge.textContent = 'UNLOCKED';
      doorIconBox.className = 'door-icon-box door-unlocked';
      doorEmoji.textContent = '🔓';
      doorHeadline.textContent = 'Door is Currently Unlocked';
      doorSubtext.textContent = 'Green Light Active | Entry Allowed';
    } else {
      doorBadge.className = 'badge badge-locked';
      doorBadge.textContent = 'LOCKED';
      doorIconBox.className = 'door-icon-box door-locked';
      doorEmoji.textContent = '🔒';
      doorHeadline.textContent = 'Door is Securely Locked';
      doorSubtext.textContent = 'Red Light Active | Access via White Card';
    }

    // 3. Environmental Sensors
    document.getElementById('tempDisplay').textContent = Number(data.temperature).toFixed(1);
    document.getElementById('humDisplay').textContent = Number(data.humidity).toFixed(0);
    document.getElementById('heatIndexDisplay').textContent = 'Feels like: ' + Number(data.heatIndex).toFixed(1) + ' °C';
    document.getElementById('trendDisplay').textContent = 'Trend: ' + (data.heatTrend || 'STABLE');

    // 4. Safety Banners
    const fireBanner = document.getElementById('fireAlertBanner');
    const flameVal = document.getElementById('flameDisplay');
    const safetyBadge = document.getElementById('safetyBadge');

    if (data.fireLatched || data.flame) {
      fireBanner.classList.remove('hidden');
      flameVal.textContent = 'FIRE ALERT!';
      flameVal.className = 'metric-val danger-text';
      safetyBadge.className = 'badge badge-danger';
      safetyBadge.textContent = 'FIRE EVACUATE';
    } else {
      fireBanner.classList.add('hidden');
      flameVal.textContent = 'SAFE';
      flameVal.className = 'metric-val safe-text';
      if (!data.panic) {
        safetyBadge.className = 'badge badge-safe';
        safetyBadge.textContent = 'SAFE';
      }
    }

    const panicBanner = document.getElementById('panicAlertBanner');
    const panicVal = document.getElementById('panicDisplay');
    if (data.panic) {
      panicBanner.classList.remove('hidden');
      panicVal.textContent = 'PANIC!';
      panicVal.className = 'metric-val danger-text';
      safetyBadge.className = 'badge badge-danger';
      safetyBadge.textContent = 'PANIC ACTIVE';
    } else {
      panicBanner.classList.add('hidden');
      panicVal.textContent = 'NORMAL';
      panicVal.className = 'metric-val safe-text';
    }

    // 5. Device Health Fields
    document.getElementById('roomIdDisplay').textContent = data.roomId;
    document.getElementById('rssiDisplay').textContent = data.rssi + ' dBm';
    document.getElementById('memDisplay').textContent = data.heapKb + ' KB / ' + data.psramKb + ' KB';
    document.getElementById('chipTempDisplay').textContent = Number(data.chipTemp).toFixed(1) + ' °C';
    document.getElementById('fwDisplay').textContent = 'v' + data.fwVersion + ' (' + data.lastResetReason + ')';

  } catch (err) {
    console.warn('Status poll failed:', err);
  }
}

async function triggerDoor(action) {
  try {
    await fetch('/api/door', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ action })
    });
    fetchStatus();
    fetchEvents();
  } catch (e) {
    alert('Failed to send door action: ' + e.message);
  }
}

async function resetFire() {
  try {
    await fetch('/api/reset-fire', { method: 'POST' });
    fetchStatus();
    fetchEvents();
  } catch (e) {
    alert('Failed to reset fire: ' + e.message);
  }
}

function debounceFetchEvents() {
  clearTimeout(debounceTimer);
  debounceTimer = setTimeout(fetchEvents, 300);
}

async function fetchEvents() {
  const search = document.getElementById('searchInput').value;
  const type = document.getElementById('typeFilter').value;
  const tbody = document.getElementById('eventsTableBody');

  try {
    const url = `/api/events?search=${encodeURIComponent(search)}&type=${encodeURIComponent(type)}&limit=50`;
    const res = await fetch(url);
    const data = await res.json();

    if (!data.events || data.events.length === 0) {
      tbody.innerHTML = '<tr><td colspan="6" class="text-center" style="color:#64748b;padding:24px;">No event records found matching filter.</td></tr>';
      return;
    }

    tbody.innerHTML = data.events.map(e => {
      const timeStr = new Date(e.createdAt).toLocaleTimeString();
      let badgeClass = 'badge-info';
      if (e.type === 'ATTENDANCE' || e.type === 'CARD_TAP') badgeClass = 'badge-unlocked';
      else if (e.type === 'FIRE' || e.type === 'PANIC') badgeClass = 'badge-locked';

      return `
        <tr>
          <td class="font-mono">${timeStr}</td>
          <td><span class="badge ${badgeClass}">${e.type}</span></td>
          <td>${e.detail || '-'}</td>
          <td><span class="badge ${e.doorState === 'UNLOCKED' ? 'badge-unlocked' : 'badge-locked'}">${e.doorState || 'N/A'}</span></td>
          <td>${e.temp ? Number(e.temp).toFixed(1) + ' °C' : '--'}</td>
          <td><span style="font-size:0.75rem;color:#94a3b8;">${e.source || 'LIVE'}</span></td>
        </tr>
      `;
    }).join('');

  } catch (err) {
    console.error('Failed to load events:', err);
  }
}

// -------------------------------------------------------------
// AI Assistant (Level 6 Task 10)
// -------------------------------------------------------------
async function askAiAssistant() {
  const input = document.getElementById('aiInput');
  const question = input.value.trim();
  if (!question) return;

  const box = document.getElementById('aiResponseBox');
  box.innerHTML = '⏳ <em>Consulting stored classroom logs...</em>';

  try {
    const res = await fetch('/api/ai-assistant', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ question })
    });
    const d = await res.json();
    box.innerHTML = `🤖 <strong>Answer:</strong> ${d.answer}`;
  } catch (err) {
    box.innerHTML = `❌ Error: ${err.message}`;
  }
}

function quickAi(q) {
  document.getElementById('aiInput').value = q;
  askAiAssistant();
}

// -------------------------------------------------------------
// Authentication & Role Switching Modal
// -------------------------------------------------------------
function openLoginModal() {
  document.getElementById('loginModal').classList.remove('hidden');
}

function closeLoginModal() {
  document.getElementById('loginModal').classList.add('hidden');
  document.getElementById('loginErrorMsg').style.display = 'none';
}

function prefillLogin(u, p) {
  document.getElementById('loginUsername').value = u;
  document.getElementById('loginPassword').value = p;
  document.getElementById('loginErrorMsg').style.display = 'none';
}

async function performLogin() {
  const username = document.getElementById('loginUsername').value.trim();
  const password = document.getElementById('loginPassword').value.trim();
  const errMsg = document.getElementById('loginErrorMsg');

  try {
    const res = await fetch('/api/auth/login', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ username, password })
    });
    const d = await res.json();

    if (!res.ok) {
      errMsg.textContent = d.error || 'Login failed';
      errMsg.style.display = 'block';
      return;
    }

    currentUser = d.user;
    document.getElementById('currentUserName').textContent = `${d.user.role}: ${d.user.name}`;
    closeLoginModal();
    alert(`Logged in as ${d.user.role} (${d.user.name})`);

  } catch (err) {
    errMsg.textContent = err.message;
    errMsg.style.display = 'block';
  }
}

async function purgePrivacyData() {
  if (confirm('Privacy Rule Compliance: Permanently delete all collected test records and face logs?')) {
    try {
      const res = await fetch('/api/purge-test-data', { method: 'POST' });
      const d = await res.json();
      alert(d.message);
      fetchStatus();
      fetchEvents();
    } catch (e) {
      alert('Purge error: ' + e.message);
    }
  }
}

// Initialization
setInterval(fetchStatus, 1500);
setInterval(fetchEvents, 3000);
fetchStatus();
fetchEvents();
