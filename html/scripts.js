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
  
document.addEventListener('DOMContentLoaded', () => {
  // Initialization: Ensure the correct section is shown on load
  const activeMenuItem = document.querySelector('.menu li.active');
  if (activeMenuItem) {
    showSection(activeMenuItem.getAttribute('data-section'), activeMenuItem);
  }
});