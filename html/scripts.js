// ======= API HELPERS (adjust paths to match your ESP32) =======
async function apiGet(path) {
  const res = await fetch(path);
  if (!res.ok) throw new Error(`GET ${path} -> ${res.status}`);
  return res.json();
}

async function apiPostJson(path, obj) {
  const res = await fetch(path, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(obj)
  });
  const text = await res.text();
  return { status: res.status, text };
}

// ====== UI NAVIGATION (Section Switching) ======
function showSection(id, menuItem) {
  // switch visible section
  document.querySelectorAll('.section').forEach(sec => {
    sec.classList.toggle('active', sec.id === id);
  });

  // highlight menu
  document.querySelectorAll('.menu li').forEach(li => {
    li.classList.toggle('active', li === menuItem);
  });
}

// ====== UTILITY FUNCTION (For button testing) ======
function showAlert() {
  alert("Function triggered!");
}

// ====== Wi-Fi Function  ======
function clearWifi() {
  document.getElementById('wifi_ssid').value = '';
  document.getElementById('wifi_pass').value = '';
}

async function getWifi() {
  let ssid = ''; // Initialize variables to hold the fetched data
  let pass = '';
  try {
    const res = await apiGet('/api/getwifi');
    if (res.status !== 200) alert('Failed to get wi-fi. response: ' + res.status);
    else {
      const data = await res.json();
      ssid = data.ssid;
      pass = data.pass;
      alert('Get Wi-Fi successfully!');
    }
  } catch (err) {
    alert('Error get wifi settings: ' + err.message);
    console.error(err);
  }

  document.getElementById('wifi_ssid').value = ssid;
  document.getElementById('wifi_pass').value = pass;
  
}

async function saveWifi() {
  // 1. Get values from the input fields
  const ssid = document.getElementById('wifi_ssid').value;
  const pass = document.getElementById('wifi_pass').value;

  // 2. Construct the data payload (JSON is standard for modern APIs)
  const payload = {
    ssid: ssid,
    pass: pass
  };

  // 3. Send the POST request using the fetch API
  //const res = await apiPostJson('/api/setwifi', payload);
  try {
    const res = await apiPostJson('/api/setwifi', payload);
    if (res.status !== 200) alert('Failed to save wi-fi. response: ' + res.status);
    else {
      alert('Wi-Fi settings saved successfully!');
      clearWifi();
    }
  } catch (err) {
    alert('Error saving settings: ' + err.message);
    console.error(err);
  }
}

// function enable Modbus settings
function toggleModbus(enabled) {
  const modbuscf = document.getElementById('modbus_config');
  modbuscf.style.display = enabled ? 'block' : 'none';
}
  
async function saveMqtt() {
  const payload = {
    host: document.getElementById('mqtt_host').value,
    port: Number(document.getElementById('mqtt_port').value),
    client_id: document.getElementById('mqtt_client_id').value,
    username: document.getElementById('mqtt_username').value,
    password: document.getElementById('mqtt_password').value,
  };
  const res = await apiPostJson('/api/setmqtt', payload);
  alert(res.status === 200 ? 'MQTT settings saved successfully!' : 'Failed to save MQTT settings. response: ' + res.status);
}

async function loadMqtt() {
  const data = await apiGet('/api/getmqtt');
  document.getElementById('mqtt_host').value = data.host || '';
  document.getElementById('mqtt_port').value = data.port || 1883;
  document.getElementById('mqtt_client_id').value = data.client_id || '';
  document.getElementById('mqtt_username').value = data.username || '';
  document.getElementById('mqtt_password').value = data.password || '';
}

async function uploadModbusConfig() {
  const file = document.getElementById('mb_file').files[0];
  if (!file) {
    alert('Please select a Modbus configuration file to upload.');
    return;
  }
  try {
    const buffer = await file.arrayBuffer();
    const workbook = XLSX.read(buffer, { type: 'array' });
    // ===== sheet : serial (key/value rows) =====
    const ws_serial = workbook.Sheets['serial'];
    if (!ws_serial) {
      throw new Error('Missing Sheet: serial');
    }
    const serialRows = XLSX.utils.sheet_to_json(ws_serial, { header: ['key', 'value'] });
    const serial = {};
    serialRows.forEach(row => { if (row.key) serial[row.key] = row.value; });

    // ===== sheet : devices ======
    const ws_devices = workbook.Sheets['devices'];
    if (!ws_devices) {
      throw new Error('Missing Sheets: devices');
    }
    const devicesRows = XLSX.utils.sheet_to_json(ws_devices);

    // ===== sheet : points ======
    const ws_points = workbook.Sheets['points'];
    if (!ws_points) {
      throw new Error('Missing Sheet: point');
    }
    const pointsRows = XLSX.utils.sheet_to_json(ws_points);

    // ===== ประกอบเป็น nested object =====
    const devices = devicesRows.map(dev => ({
      slave_id : Number(dev.slave_id),
      name : String(dev.name || ''),
      access : String(dev.access || ''),
      points : pointsRows.filter(p => Number(p.slave_id) === Number(dev.slave_id)).map(p => ({
        cid : Number(p.cid),
        name : String(p.name || ''),
        unit : String(p.unit || ''),
        reg_type : String(p.reg_type || 'H'),
        reg_addr : Number(p.reg_addr),
        reg_size : Number(p.reg_size),
        datatype : String(p.datatype || 'UB'),
        scale : Number(p.scale || 1),
        min: Number(p.min || 0),
        max: Number(p.max || 0),
    }))
  }));
  const config = {
    serial: {
      baud : Number(serial.baud) || 9600,
      parity : String(serial.parity) || 'N',
      data_bits : Number(serial.data_bits) || 8,
      stop_bits: Number(serial.stop_bits) || 1,
  },
    devices: devices,
  };

  // ===== validate ต่ำ ======
  if (devices.length === 0) throw new Error ('No devices found');
  for (const d of devices) {
    if (d.points.length === 0 ) throw new Error(`Device ${d.slave_id} has no points`);
  } 

  // ===== POST JSON to ESP32 =====
  const res = await fetch('/api/upload-mb',{
    method: 'POST',
    headers: { 'Content-type': 'application/json'},
    body: JSON.stringify(config),
  });
  if(!res.ok) throw new Error(`Upload failed: ${res.status}`);
 alert(`Uploaded ${devices.length} device(s), ${devices.reduce((s,d)=>s+d.points.length,0)} point(s)`);
  } catch (err) {
    alert ('Error: '+ err.message);
    console.error(err);
  }
}

document.addEventListener('DOMContentLoaded', () => {
  // Initialization: Ensure the correct section is shown on load
  const activeMenuItem = document.querySelector('.menu li.active');
  if (activeMenuItem) {
    showSection(activeMenuItem.getAttribute('data-section'), activeMenuItem);
  }
});