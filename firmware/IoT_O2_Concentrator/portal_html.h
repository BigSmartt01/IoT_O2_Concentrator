#ifndef PORTAL_HTML_H
#define PORTAL_HTML_H

#include <Arduino.h>

// Captive-portal HTML (PROGMEM to save RAM)
static const char PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>O2 Controller</title>
<style>
  *{box-sizing:border-box;margin:0;padding:0}
  body{font-family:system-ui,-apple-system,sans-serif;background:#0f172a;color:#e2e8f0;
       max-width:420px;margin:0 auto;padding:1.25rem;line-height:1.4}
  h1{font-size:1.35rem;margin-bottom:1rem;color:#38bdf8;text-align:center}
  .card{background:#1e293b;border-radius:12px;padding:1rem;margin-bottom:1rem}
  .card h2{font-size:0.95rem;color:#94a3b8;margin-bottom:0.75rem;text-transform:uppercase;letter-spacing:.04em}
  label.mode{display:flex;align-items:center;gap:.6rem;padding:.55rem 0;cursor:pointer;font-size:1rem}
  input[type=radio]{width:1.1rem;height:1.1rem;accent-color:#38bdf8}
  #sliders{display:none}
  .slider-row{margin-bottom:1rem}
  .slider-row:last-child{margin-bottom:0}
  .slider-row .top{display:flex;justify-content:space-between;margin-bottom:.35rem;font-size:.95rem}
  .slider-row .val{color:#38bdf8;font-weight:600;font-variant-numeric:tabular-nums}
  input[type=range]{width:100%;accent-color:#38bdf8;height:1.5rem}
  .hint{font-size:.8rem;color:#64748b;text-align:center;margin-top:.5rem}
  .number-input{display:flex;gap:0.5rem;align-items:center;flex-wrap:wrap}
  .number-input input{flex:1;padding:0.5rem;border-radius:6px;border:1px solid #334155;
                       background:#0f172a;color:#e2e8f0;font-size:1rem;min-width:200px}
  .number-input button{padding:0.5rem 1rem;background:#38bdf8;border:none;border-radius:6px;
                        color:#0f172a;font-weight:bold;cursor:pointer}
  .number-display{margin-top:0.5rem;font-size:0.9rem;color:#94a3b8}
</style>
</head>
<body>
  <h1>O2 Controller</h1>
  <div class="card">
    <h2>Input Mode</h2>
    <label class="mode"><input type="radio" name="mode" value="pot" checked> Potentiometer</label>
    <label class="mode"><input type="radio" name="mode" value="wifi"> WiFi</label>
    <label class="mode"><input type="radio" name="mode" value="sensor"> Sensor</label>
  </div>
  <div class="card" id="sliders">
    <h2>WiFi Demo Values</h2>
    <div class="slider-row">
      <div class="top"><span>O2 Purity</span><span class="val" id="purityVal">85.0</span></div>
      <input type="range" id="purity" min="21.0" max="95.6" step="0.1" value="85.0">
    </div>
    <div class="slider-row">
      <div class="top"><span>Flow (LPM)</span><span class="val" id="flowVal">5.0</span></div>
      <input type="range" id="flow" min="0.0" max="20.0" step="0.1" value="5.0">
    </div>
    <div class="slider-row">
      <div class="top"><span>Temperature (&deg;C)</span><span class="val" id="tempVal">25.0</span></div>
      <input type="range" id="temp" min="5.0" max="55.0" step="0.1" value="25.0">
    </div>
  </div>
  <!-- NEW: Caregiver Number card -->
  <div class="card">
    <h2>Caregiver Number</h2>
    <div class="number-input">
      <input type="tel" id="caregiverInput" placeholder="+234..." value="">
      <button id="setNumberBtn">Set</button>
    </div>
    <div class="number-display">Current: <span id="currentNumber">(unknown)</span></div>
  </div>
  <p class="hint">Connect to AP &ldquo;O2-Controller&rdquo; &middot; open 192.168.4.1</p>
<script>
const sliders=document.getElementById('sliders');
const purity=document.getElementById('purity');
const flow=document.getElementById('flow');
const temp=document.getElementById('temp');
const purityVal=document.getElementById('purityVal');
const flowVal=document.getElementById('flowVal');
const tempVal=document.getElementById('tempVal');

function fmt(n){return Number(n).toFixed(1)}

function setModeUI(mode){
  document.querySelectorAll('input[name=mode]').forEach(r=>{r.checked=(r.value===mode)});
  sliders.style.display=(mode==='wifi')?'block':'none';
}

function sendMode(mode){
  fetch('/setMode?mode='+encodeURIComponent(mode)).catch(()=>{});
  setModeUI(mode);
}

function sendUpdate(){
  const p=fmt(purity.value),f=fmt(flow.value),t=fmt(temp.value);
  purityVal.textContent=p;
  flowVal.textContent=f;
  tempVal.textContent=t;
  fetch('/update?purity='+p+'&flow='+f+'&temp='+t).catch(()=>{});
}

document.querySelectorAll('input[name=mode]').forEach(r=>{
  r.addEventListener('change',()=>sendMode(r.value));
});
[purity,flow,temp].forEach(el=>{
  el.addEventListener('input',sendUpdate);
});

// Restore mode/values from device on load
fetch('/status').then(r=>r.json()).then(d=>{
  setModeUI(d.mode||'pot');
  if(typeof d.o2==='number'){purity.value=d.o2;purityVal.textContent=fmt(d.o2)}
  if(typeof d.flow==='number'){flow.value=d.flow;flowVal.textContent=fmt(d.flow)}
  if(typeof d.temp==='number'){temp.value=d.temp;tempVal.textContent=fmt(d.temp)}
  // caregiver
  if(d.caregiver){ 
    document.getElementById('currentNumber').textContent = d.caregiver;
    document.getElementById('caregiverInput').value = d.caregiver;
  }
}).catch(()=>{});

// Set caregiver number
document.getElementById('setNumberBtn').addEventListener('click', function(){
  const num = document.getElementById('caregiverInput').value.trim();
  if(num === '') return;
  fetch('/setNumber?num='+encodeURIComponent(num))
    .then(r=>r.text())
    .then(resp=>{
      if(resp==='OK'){
        document.getElementById('currentNumber').textContent = num;
        alert('Number updated successfully');
      } else {
        alert('Error: '+resp);
      }
    })
    .catch(()=>alert('Request failed'));
});
</script>
</body>
</html>
)rawliteral";

#endif // PORTAL_HTML_H