let debounceTimer = null;

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
      connText.textContent = 'DEVICE OFFLINE / TIMEOUT';
    }

    // 2. Room & Class Info
    document.getElementById('roomIdDisplay').textContent = data.roomId;
    document.getElementById('classTimeDisplay').textContent = data.classStartTime;
    if (data.lastUid) {
      document.getElementById('rfidDisplay').textContent = data.lastUid;
    }

    // 3. Simulated Door State
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
      doorSubtext.textContent = 'RGB Indicator: Solid Green | Authorized Entry Allowed';
    } else {
      doorBadge.className = 'badge badge-locked';
      doorBadge.textContent = 'LOCKED';
      doorIconBox.className = 'door-icon-box door-locked';
      doorEmoji.textContent = '🔒';
      doorHeadline.textContent = 'Door is Securely Locked';
      doorSubtext.textContent = 'RGB Indicator: Solid Red | Access via White RFID Card';
    }

    // 4. Sensors
    document.getElementById('tempDisplay').textContent = Number(data.temperature).toFixed(1);
    document.getElementById('humDisplay').textContent = Number(data.humidity).toFixed(1);

    const flameBanner = document.getElementById('fireAlertBanner');
    const flameVal = document.getElementById('flameDisplay');
    const safetyBadge = document.getElementById('safetyBadge');

    if (data.flame) {
      flameBanner.classList.remove('hidden');
      flameVal.textContent = 'FIRE ALERT!';
      flameVal.className = 'metric-val danger-text';
      safetyBadge.className = 'badge badge-danger';
      safetyBadge.textContent = 'FIRE EVACUATION';
    } else {
      flameBanner.classList.add('hidden');
      flameVal.textContent = 'SAFE';
      flameVal.className = 'metric-val safe-text';
      if (!data.panic) {
        safetyBadge.className = 'badge badge-safe';
        safetyBadge.textContent = 'ALL SAFE';
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

  } catch (err) {
    console.warn('Status poll failed:', err);
  }
}

async function triggerDoor(action) {
  try {
    const res = await fetch('/api/door', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ action })
    });
    await res.json();
    fetchStatus();
    fetchEvents();
  } catch (err) {
    alert('Failed to send door command: ' + err.message);
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
      let catBadgeClass = 'badge-info';
      if (e.type === 'ATTENDANCE') catBadgeClass = 'badge-unlocked';
      else if (e.type === 'FLAME_ALERT' || e.type === 'PANIC_ALERT') catBadgeClass = 'badge-locked';

      return `
        <tr>
          <td class="font-mono">${timeStr}</td>
          <td><span class="badge ${catBadgeClass}">${e.type}</span></td>
          <td>${e.detail || '-'}</td>
          <td><span class="badge ${e.doorState === 'UNLOCKED' ? 'badge-unlocked' : 'badge-locked'}">${e.doorState || 'N/A'}</span></td>
          <td class="font-mono">${e.roomId}</td>
          <td><span style="font-size:0.75rem;color:#94a3b8;">${e.source || 'LIVE'}</span></td>
        </tr>
      `;
    }).join('');

  } catch (err) {
    console.error('Failed to load events:', err);
  }
}

function openConfigModal() {
  document.getElementById('configRoomId').value = document.getElementById('roomIdDisplay').textContent;
  document.getElementById('configClassTime').value = document.getElementById('classTimeDisplay').textContent.replace(' AM','').replace(' PM','');
  document.getElementById('configModal').classList.remove('hidden');
}

function closeConfigModal() {
  document.getElementById('configModal').classList.add('hidden');
}

async function saveConfiguration() {
  const roomId = document.getElementById('configRoomId').value.trim();
  const classStartTime = document.getElementById('configClassTime').value.trim();

  if (!roomId || !classStartTime) {
    alert('Please fill out both Room ID and Class Time.');
    return;
  }

  try {
    await fetch('/api/config', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ roomId, classStartTime })
    });
    closeConfigModal();
    fetchStatus();
  } catch (err) {
    alert('Config update failed: ' + err.message);
  }
}

// Initial Bootstrapping
setInterval(fetchStatus, 1000);
setInterval(fetchEvents, 3000);
fetchStatus();
fetchEvents();
