#pragma once
#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP8266 Wi-Fi Turbo Repeater</title>
    <style>
        :root {
            --bg: #0b1120;
            --card: #1e293b;
            --card-sub: #0f172a;
            --border: #334155;
            --text: #f8fafc;
            --text-dim: #94a3b8;
            --primary: #3b82f6;
            --primary-hover: #2563eb;
            --success: #10b981;
            --warning: #f59e0b;
            --danger: #ef4444;
        }
        * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
        body { background-color: var(--bg); color: var(--text); padding: 16px; display: flex; justify-content: center; min-height: 100vh; }
        .container { width: 100%; max-width: 500px; }
        .header { text-align: center; margin-bottom: 20px; }
        .header h1 { font-size: 1.4rem; font-weight: 700; color: #fff; margin-bottom: 4px; display: flex; align-items: center; justify-content: center; gap: 8px; }
        .header p { color: var(--text-dim); font-size: 0.825rem; }
        .card { background: var(--card); border: 1px solid var(--border); border-radius: 12px; padding: 18px; margin-bottom: 14px; box-shadow: 0 4px 6px -1px rgba(0,0,0,0.3); }
        .card-title { font-size: 1rem; font-weight: 600; margin-bottom: 12px; display: flex; justify-content: space-between; align-items: center; }
        .badge { font-size: 0.725rem; padding: 3px 8px; border-radius: 9999px; font-weight: 600; }
        .badge-success { background: rgba(16,185,129,0.2); color: var(--success); }
        .badge-warning { background: rgba(245,158,11,0.2); color: var(--warning); }
        .stat-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
        .stat-item { background: var(--card-sub); padding: 10px; border-radius: 8px; border: 1px solid var(--border); }
        .stat-label { font-size: 0.7rem; color: var(--text-dim); margin-bottom: 3px; }
        .stat-val { font-size: 0.9rem; font-weight: 600; word-break: break-all; }
        .form-group { margin-bottom: 14px; }
        label { display: block; font-size: 0.825rem; font-weight: 500; margin-bottom: 5px; color: var(--text); }
        .check-label { display: flex; align-items: center; gap: 8px; cursor: pointer; }
        .check-label input[type=checkbox] { width: auto; }
        .input-wrapper { position: relative; display: flex; align-items: center; }
        select, input { width: 100%; padding: 10px 12px; border-radius: 8px; background: var(--card-sub); border: 1px solid var(--border); color: #fff; font-size: 0.875rem; outline: none; transition: border-color 0.2s; }
        select:focus, input:focus { border-color: var(--primary); }
        input:disabled { opacity: 0.5; }
        .eye-btn { position: absolute; right: 10px; background: none; border: none; color: var(--text-dim); cursor: pointer; padding: 4px; display: flex; align-items: center; }
        .eye-btn:hover { color: #fff; }
        .hint { font-size: 0.725rem; color: var(--text-dim); margin-top: 3px; }
        button.btn-main { width: 100%; padding: 12px; background: var(--primary); color: #fff; border: none; border-radius: 8px; font-weight: 600; font-size: 0.925rem; cursor: pointer; transition: background 0.2s; display: flex; justify-content: center; align-items: center; gap: 8px; }
        button.btn-main:hover { background: var(--primary-hover); }
        button.btn-outline { width: 100%; padding: 10px; background: transparent; border: 1px solid var(--border); color: var(--text); border-radius: 8px; font-weight: 500; font-size: 0.85rem; cursor: pointer; margin-top: 8px; }
        button.btn-outline:hover { background: rgba(255,255,255,0.05); }
        button.btn-danger { width: 100%; padding: 10px; background: rgba(239,68,68,0.15); border: 1px solid var(--danger); color: #fca5a5; border-radius: 8px; font-weight: 500; font-size: 0.85rem; cursor: pointer; margin-top: 8px; }
        button.btn-danger:hover { background: var(--danger); color: #fff; }
        .scan-btn { width: auto; padding: 5px 10px; font-size: 0.75rem; background: #334155; color: #fff; border: none; border-radius: 6px; cursor: pointer; }
        .scan-btn:hover { background: #475569; }
        .spinner { border: 2px solid rgba(255,255,255,0.3); border-radius: 50%; border-top: 2px solid #fff; width: 14px; height: 14px; animation: spin 0.8s linear infinite; display: inline-block; }
        @keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }
        .alert { padding: 12px; border-radius: 8px; margin-bottom: 12px; font-size: 0.825rem; display: none; }
        .alert-info { background: rgba(59,130,246,0.15); border: 1px solid var(--primary); color: #93c5fd; }
        .alert-success { background: rgba(16,185,129,0.15); border: 1px solid var(--success); color: #6ee7b7; }
        .client-row { display: flex; justify-content: space-between; align-items: center; padding: 8px 10px; background: var(--card-sub); border: 1px solid var(--border); border-radius: 8px; margin-bottom: 6px; font-size: 0.8rem; }
        .client-row:last-child { margin-bottom: 0; }
        .client-mac { font-weight: 600; }
        .client-ip { color: var(--text-dim); }
        .empty-hint { color: var(--text-dim); font-size: 0.8rem; text-align: center; padding: 10px 0; }
    </style>
</head>
<body>
<div class="container">
    <div class="header">
        <h1>
            <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="#3b82f6" stroke-width="2.5"><path d="M5 12.55a11 11 0 0 1 14.08 0"></path><path d="M1.42 9a16 16 0 0 1 21.16 0"></path><path d="M8.53 16.11a6 6 0 0 1 6.95 0"></path><line x1="12" y1="20" x2="12.01" y2="20"></line></svg>
            Wi-Fi Turbo Repeater
        </h1>
        <p>160MHz L3 NAT Engine &bull; Zero-Sleep RF</p>
    </div>

    <!-- Status Card -->
    <div class="card">
        <div class="card-title">
            <span>Link Status</span>
            <span id="status-badge" class="badge badge-warning">Loading...</span>
        </div>
        <div class="stat-grid">
            <div class="stat-item">
                <div class="stat-label">Home Network (WAN)</div>
                <div id="stat-sta-ssid" class="stat-val">---</div>
            </div>
            <div class="stat-item">
                <div class="stat-label">Assigned IP (WAN)</div>
                <div id="stat-sta-ip" class="stat-val">---</div>
            </div>
            <div class="stat-item">
                <div class="stat-label">Wi-Fi Signal (RSSI)</div>
                <div id="stat-rssi" class="stat-val">---</div>
            </div>
            <div class="stat-item">
                <div class="stat-label">Connected Devices</div>
                <div id="stat-clients" class="stat-val">---</div>
            </div>
            <div class="stat-item">
                <div class="stat-label">Current MAC (WAN)</div>
                <div id="stat-mac" class="stat-val">---</div>
            </div>
            <div class="stat-item">
                <div class="stat-label">Uptime</div>
                <div id="stat-uptime" class="stat-val">---</div>
            </div>
        </div>
    </div>

    <!-- Connected Clients Card -->
    <div class="card">
        <div class="card-title">
            <span>Connected Clients</span>
            <span id="client-count-badge" class="badge badge-success">0</span>
        </div>
        <div id="clients-list">
            <div class="empty-hint">No devices connected yet.</div>
        </div>
    </div>

    <!-- Configuration Form -->
    <div class="card">
        <div class="card-title">
            <span>Wi-Fi Configuration</span>
            <button class="scan-btn" onclick="scanNetworks()" id="scan-btn">Scan Networks</button>
        </div>

        <div id="alert-box" class="alert"></div>

        <form id="config-form" onsubmit="saveConfig(event)">
            <div class="form-group">
                <label for="sta_ssid">1. Source Wi-Fi Network (Main Router)</label>
                <select id="sta_ssid_select" onchange="onSsidSelect(this.value)">
                    <option value="">-- Searching networks... --</option>
                </select>
                <input type="text" id="sta_ssid" name="sta_ssid" placeholder="Network name (SSID)" required style="margin-top: 6px;">
            </div>

            <div class="form-group">
                <label for="sta_pass">Source Network Password</label>
                <div class="input-wrapper">
                    <input type="password" id="sta_pass" name="sta_pass" placeholder="Wi-Fi password">
                    <button type="button" class="eye-btn" onclick="togglePass('sta_pass')">👁️</button>
                </div>
            </div>

            <div class="form-group">
                <label class="check-label">
                    <input type="checkbox" id="mac_enable" onchange="toggleMacInput()">
                    Clone / Set Custom MAC Address
                </label>
                <div class="input-wrapper" style="margin-top: 6px;">
                    <input type="text" id="custom_mac" name="custom_mac" placeholder="AA:BB:CC:DD:EE:FF" disabled>
                </div>
                <div class="hint">Overrides the MAC address used to connect upstream (see "Current MAC" above for the default). Useful if your ISP or router restricts access by device MAC.</div>
            </div>

            <hr style="border: 0; border-top: 1px solid var(--border); margin: 14px 0;">

            <div class="form-group">
                <label for="ap_ssid">2. Repeated Network Name (Extender SSID)</label>
                <input type="text" id="ap_ssid" name="ap_ssid" placeholder="E.g: MyHome_EXT" required>
                <div class="hint">Name of the Wi-Fi network the repeater will broadcast.</div>
            </div>

            <div class="form-group">
                <label for="ap_pass">Repeated Network Password</label>
                <div class="input-wrapper">
                    <input type="password" id="ap_pass" name="ap_pass" placeholder="Minimum 8 characters (empty = open)">
                    <button type="button" class="eye-btn" onclick="togglePass('ap_pass')">👁️</button>
                </div>
                <div class="hint">Leave empty for an open network, or use at least 8 characters for WPA2.</div>
            </div>

            <button type="submit" class="btn-main" id="save-btn">Save &amp; Connect</button>
            <button type="button" class="btn-outline" onclick="rebootDevice()">Restart Device</button>
            <button type="button" class="btn-danger" onclick="resetConfig()">Factory Reset</button>
        </form>
    </div>
</div>

<script>
function showAlert(text, type='alert-info') {
    const box = document.getElementById('alert-box');
    box.className = 'alert ' + type;
    box.innerText = text;
    box.style.display = 'block';
}

function togglePass(id) {
    const input = document.getElementById(id);
    input.type = input.type === 'password' ? 'text' : 'password';
}

function toggleMacInput() {
    const enabled = document.getElementById('mac_enable').checked;
    const input = document.getElementById('custom_mac');
    input.disabled = !enabled;
    if (!enabled) input.value = '';
}

function formatUptime(sec) {
    if (sec === undefined || sec === null) return '---';
    const h = Math.floor(sec / 3600);
    const m = Math.floor((sec % 3600) / 60);
    const s = Math.floor(sec % 60);
    return h + 'h ' + m + 'm ' + s + 's';
}

function updateStatus() {
    fetch('/status')
        .then(r => r.json())
        .then(data => {
            document.getElementById('stat-sta-ssid').innerText = data.sta_ssid || 'Not configured';
            document.getElementById('stat-sta-ip').innerText = data.sta_ip || '0.0.0.0';
            document.getElementById('stat-rssi').innerText = data.sta_connected ? `${data.rssi} dBm (${data.signal_pct}%)` : 'Disconnected';
            document.getElementById('stat-clients').innerText = data.ap_clients + ' device(s)';
            document.getElementById('stat-mac').innerText = data.sta_mac || '---';
            document.getElementById('stat-uptime').innerText = formatUptime(data.uptime_s);
            
            const badge = document.getElementById('status-badge');
            if (data.sta_connected) {
                badge.innerText = 'Routing NAT (Online)';
                badge.className = 'badge badge-success';
            } else {
                badge.innerText = 'Configuration Mode';
                badge.className = 'badge badge-warning';
            }
        })
        .catch(e => console.error(e));
}

function updateClients() {
    fetch('/clients')
        .then(r => r.json())
        .then(list => {
            document.getElementById('client-count-badge').innerText = list.length;
            const container = document.getElementById('clients-list');
            if (list.length === 0) {
                container.innerHTML = '<div class="empty-hint">No devices connected yet.</div>';
                return;
            }
            container.innerHTML = list.map(c =>
                `<div class="client-row"><span class="client-mac">${c.mac}</span><span class="client-ip">${c.ip}</span></div>`
            ).join('');
        })
        .catch(e => console.error(e));
}

function scanNetworks() {
    const btn = document.getElementById('scan-btn');
    btn.innerHTML = '<span class="spinner"></span> Scanning...';
    btn.disabled = true;

    fetch('/scan')
        .then(r => r.json())
        .then(networks => {
            const select = document.getElementById('sta_ssid_select');
            select.innerHTML = '<option value="">-- Select a detected network --</option>';
            networks.forEach(net => {
                const opt = document.createElement('option');
                opt.value = net.ssid;
                opt.innerText = `${net.ssid} (${net.rssi} dBm ${net.enc ? '🔒' : '🔓'})`;
                select.appendChild(opt);
            });
            btn.innerText = 'Scan Networks';
            btn.disabled = false;
        })
        .catch(e => {
            btn.innerText = 'Scan Networks';
            btn.disabled = false;
        });
}

function onSsidSelect(ssid) {
    if (!ssid) return;
    document.getElementById('sta_ssid').value = ssid;
    const apInput = document.getElementById('ap_ssid');
    if (!apInput.value || apInput.value.endsWith('_EXT')) {
        apInput.value = ssid + '_EXT';
    }
}

function saveConfig(e) {
    e.preventDefault();
    const saveBtn = document.getElementById('save-btn');
    saveBtn.innerHTML = '<span class="spinner"></span> Saving &amp; Connecting...';
    saveBtn.disabled = true;

    const macEnabled = document.getElementById('mac_enable').checked;
    const data = {
        sta_ssid: document.getElementById('sta_ssid').value,
        sta_pass: document.getElementById('sta_pass').value,
        ap_ssid: document.getElementById('ap_ssid').value,
        ap_pass: document.getElementById('ap_pass').value,
        custom_mac: macEnabled ? document.getElementById('custom_mac').value.trim() : ''
    };

    fetch('/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
    })
    .then(r => r.text().then(text => ({ ok: r.ok, text })))
    .then(res => {
        if (!res.ok) {
            showAlert(res.text || 'Error saving configuration.', 'alert-info');
            saveBtn.innerText = 'Save & Connect';
            saveBtn.disabled = false;
            return;
        }
        showAlert('Configuration saved successfully. The chip is restarting to connect to your router...', 'alert-success');
    })
    .catch(err => {
        showAlert('Configuration sent. Restarting...', 'alert-success');
    });
}

function rebootDevice() {
    if (confirm('Do you want to restart the ESP8266?')) {
        fetch('/reboot', { method: 'POST' });
        showAlert('Restarting...', 'alert-info');
    }
}

function resetConfig() {
    if (confirm('Do you want to erase all configuration and return to setup mode?')) {
        fetch('/reset', { method: 'POST' })
            .then(() => showAlert('Configuration erased. Restarting...', 'alert-info'));
    }
}

updateStatus();
updateClients();
setInterval(updateStatus, 3000);
setInterval(updateClients, 5000);
scanNetworks();
</script>
</body>
</html>
)rawliteral";
