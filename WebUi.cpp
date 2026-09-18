#include "WebUi.h"
#include "ChargerMonitorConfig.h"
#include "NanoLink.h"
#include "ChargerNetwork.h"

namespace {

const char PAGE_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Nano Lead-Acid Charger</title>
<style>
:root{color-scheme:dark;background:#0b0f13;color:#eef2f5;font-family:Arial,sans-serif}
*{box-sizing:border-box}body{margin:0;background:#0b0f13;color:#eef2f5}.shell{width:min(980px,100%);margin:0 auto;padding:16px}
.header{display:flex;align-items:flex-start;justify-content:space-between;gap:12px;margin-bottom:14px}h1{font-size:1.55rem;margin:0 0 4px}.sub{color:#8d9aa6;font-size:.9rem}
.status-pill{border:1px solid #34404b;background:#151b21;border-radius:999px;padding:8px 12px;font-weight:700;font-size:.82rem;white-space:nowrap}
.tabs{display:grid;grid-template-columns:repeat(5,1fr);gap:7px;margin-bottom:14px;position:sticky;top:0;z-index:5;background:#0b0f13;padding:8px 0}
.tab-btn{border:1px solid #34404b;background:#151b21;color:#aeb8c1;border-radius:10px;padding:11px 7px;margin:0;font-size:.82rem;font-weight:700;cursor:pointer}.tab-btn.active{background:#2a3641;color:#fff;border-color:#536475}
.tab-page{display:none}.tab-page.active{display:block}.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}.card{background:#151b21;border:1px solid #2c3741;border-radius:13px;padding:14px}.wide{grid-column:1/-1}
.label{font-size:.75rem;color:#8796a3;text-transform:uppercase;letter-spacing:.075em}.value{font-size:1.55rem;font-weight:700;margin-top:5px;word-break:break-word}.small{font-size:1rem}.ok{color:#73dc8d}.bad{color:#ff7b7b}.warn{color:#ffd36f}
.controls{display:flex;flex-wrap:wrap;gap:7px;margin-top:10px}button{border:0;border-radius:9px;padding:11px 15px;margin:0;font-size:.94rem;font-weight:700;cursor:pointer}button:disabled{opacity:.45;cursor:default}
.primary,#auto,#syncStart,#vcalStart,#vcalSubmit,#termSend,.flashBtn{background:#4fc36a;color:#07120a}.danger,#stop,#syncStop,#vcalStop{background:#df5757;color:white}.secondary,#refresh,#termClear{background:#36424d;color:white}.quick{background:#202a33;color:#d8e0e6;border:1px solid #394651;padding:8px 10px;font-size:.82rem}
input{border:1px solid #46515d;border-radius:9px;background:#0c1116;color:#fff;padding:11px;font-size:1rem}.file-input{display:block;width:100%;margin-top:10px}.foot{color:#84919d;font-size:.8rem;margin-top:9px}.row{margin-top:8px;font-size:.95rem}.row b{color:#eef2f5}
.instruction{margin-top:10px;padding:10px;border-radius:8px;background:#0d1217;line-height:1.45}.code{background:#080b0e;border:1px solid #343d46;border-radius:8px;padding:10px;margin-top:10px;white-space:pre-wrap;word-break:break-word;color:#aee9ba}pre{white-space:pre-wrap;word-break:break-word;margin:0;color:#aeb9c2}
.terminal{height:330px;overflow:auto;background:#050708;border:1px solid #2f3a43;border-radius:10px;padding:12px;color:#b7f5c5;font:13px/1.45 monospace;white-space:pre-wrap;word-break:break-word}.terminal-row{display:grid;grid-template-columns:1fr auto;gap:8px;margin-top:10px}.terminal-row input{width:100%;font-family:monospace}.quick-row{display:flex;flex-wrap:wrap;gap:6px;margin-top:10px}.hint{color:#7f8b96;font-size:.8rem;line-height:1.45;margin-top:8px}
.system-line{display:flex;justify-content:space-between;gap:10px;padding:8px 0;border-bottom:1px solid #26313a}.system-line:last-child{border-bottom:0}.system-line span:first-child{color:#8796a3}.system-line span:last-child{text-align:right;font-weight:700}.flash-result{margin-top:10px;font-weight:700;word-break:break-word}
@media(max-width:700px){.tabs{grid-template-columns:repeat(3,1fr)}}@media(max-width:520px){.grid{grid-template-columns:1fr}.tabs{grid-template-columns:repeat(2,1fr)}.header{align-items:center}.value{font-size:1.35rem}.terminal{height:290px}.terminal-row{grid-template-columns:1fr}.terminal-row button{width:100%}}
</style>
</head>
<body>
<div class="shell">
 <div class="header"><div><h1>Nano Lead-Acid Charger</h1><div class="sub">ESP32-WROOM direct hotspot control panel</div></div><div id="topLink" class="status-pill warn">NANO --</div></div>
 <div class="tabs">
  <button class="tab-btn active" data-tab="dashboard" onclick="showTab('dashboard')">DASHBOARD</button>
  <button class="tab-btn" data-tab="calibration" onclick="showTab('calibration')">CALIBRATION</button>
  <button class="tab-btn" data-tab="terminal" onclick="showTab('terminal')">TERMINAL</button>
  <button class="tab-btn" data-tab="firmware" onclick="showTab('firmware')">FIRMWARE</button>
  <button class="tab-btn" data-tab="system" onclick="showTab('system')">SYSTEM</button>
 </div>

 <section id="tab-dashboard" class="tab-page active"><div class="grid">
  <div class="card"><div class="label">Battery</div><div id="bat" class="value">--</div></div>
  <div class="card"><div class="label">Charger</div><div id="charger" class="value">--</div></div>
  <div class="card"><div class="label">External temp sensor</div><div id="bt" class="value">--</div></div>
  <div class="card"><div class="label">Nano / charger temp</div><div id="nt" class="value">--</div></div>
  <div class="card wide"><div class="label">State</div><div id="state" class="value small">--</div><div id="mode" class="foot"></div></div>
  <div class="card wide"><div class="label">Charging controls</div><div class="controls"><button id="auto" onclick="cmd('AUTO')">AUTO</button><button id="stop" onclick="cmd('STOP')">STOP CHARGING</button><button id="refresh" onclick="refreshNow()">REFRESH</button></div><div id="cmdResult" class="foot">Ready.</div></div>
 </div></section>

 <section id="tab-calibration" class="tab-page"><div class="grid">
  <div class="card wide"><div class="label">Internal Temperature Calibration</div><div id="tsyncState" class="value small">OFF</div><div id="tsyncInstruction" class="instruction">Put the external temperature sensor beside the Nano inside the case, then press START TEMP SYNC.</div><div class="row">Current external - Nano difference: <b id="tsyncDelta">--</b></div><div class="row">Point 1: <b id="tp1">--</b></div><div id="tp1s" class="foot">Samples: 0 / 60</div><div class="row">Point 2: <b id="tp2">--</b></div><div id="tp2s" class="foot">Samples: 0 / 60</div><div class="row">Point 3: <b id="tp3">--</b></div><div id="tp3s" class="foot">Samples: 0 / 60</div><pre id="tempResultCode" class="code" style="display:none"></pre><div class="controls"><button id="syncStart" onclick="cmd('TSYNC START')">START TEMP SYNC</button><button id="syncStop" onclick="cmd('TSYNC STOP')">CANCEL TEMP SYNC</button></div><div class="foot">Point 1 is measured with charging held OFF. It then returns to AUTO, waits for real charging and a temperature rise, records Point 2, then Point 3. Each point averages 60 readings.</div></div>
  <div class="card wide"><div class="label">Battery Voltage Divider Calibration</div><div id="vcalState" class="value small">OFF</div><div id="vcalInstruction" class="instruction">Press START VOLTAGE CAL, then measure the battery directly at its terminals with your multimeter.</div><div class="row">Nano currently reports: <b id="vcalNano">--</b></div><div class="row">Saved readings: <b id="vcalSamples">0 / 3</b></div><div class="row">Next Nano target: <b id="vcalTarget">--</b></div><div id="vcalInputRow" style="display:none;margin-top:8px"><input id="vactual" type="number" min="8" max="16" step="0.01" inputmode="decimal" placeholder="Actual volts e.g. 12.07"><button id="vcalSubmit" onclick="submitVcal()">SAVE READING</button></div><pre id="voltResultCode" class="code" style="display:none"></pre><div class="controls"><button id="vcalStart" onclick="cmd('VCAL START')">START VOLTAGE CAL</button><button id="vcalStop" onclick="cmd('VCAL STOP')">CANCEL VOLTAGE CAL</button></div></div>
 </div></section>

 <section id="tab-terminal" class="tab-page"><div class="card"><div class="label">Nano command terminal</div><div class="foot">Type a Nano command below and press SEND or Enter.</div><div id="terminalOutput" class="terminal">Nano terminal ready. Type HELP to list commands.</div><div class="terminal-row"><input id="terminalInput" type="text" autocomplete="off" autocapitalize="characters" spellcheck="false" placeholder="HELP" maxlength="47"><button id="termSend" onclick="sendTerminal()">SEND</button></div><div class="quick-row"><button class="quick" onclick="terminalQuick('HELP')">HELP</button><button class="quick" onclick="terminalQuick('PING')">PING</button><button class="quick" onclick="terminalQuick('STATUS')">STATUS</button><button class="quick" onclick="terminalQuick('BATTERY')">BATTERY</button><button class="quick" onclick="terminalQuick('TEMP')">TEMP</button><button class="quick" onclick="terminalQuick('STATE')">STATE</button><button class="quick" onclick="terminalQuick('AUTO')">AUTO</button><button class="quick" onclick="terminalQuick('STOP')">STOP</button><button id="termClear" onclick="clearTerminal()">CLEAR</button></div><div class="hint">Accepted commands: PING, STATUS, BATTERY, TEMP, STATE, TSYNC START/STATUS/STOP, VCAL START/STATUS/STOP, VCAL SAMPLE &lt;volts&gt;, AUTO, STOP and HELP.</div></div></section>

 <section id="tab-firmware" class="tab-page"><div class="grid">
  <div class="card wide"><div class="label">ESP32-WROOM monitor firmware</div><div class="instruction">Choose a NanoLeadAcidChargerSM .bin. It is written to the ESP32-WROOM OTA slot and the ESP restarts automatically.</div><input id="espFile" class="file-input" type="file" accept=".bin,application/octet-stream"><div class="controls"><button id="espFlash" class="flashBtn" onclick="uploadFirmware('esp')">FLASH ESP32-WROOM .BIN</button></div></div>
  <div class="card wide"><div class="label">Arduino Nano charger firmware</div><div class="instruction">Choose a NanoLeadAcidCharger .bin. The ESP sends STOP, resets the Nano into its bootloader, writes and verifies the firmware, then restarts the Nano.</div><div class="foot warn">Nano web flashing requires the separate programming wiring: ESP32 GPIO27 TX1 -> Nano D0/RX; Nano D1/TX -> divider -> ESP32 GPIO26 RX1; ESP32 GPIO25 -> reset MOSFET; common ground. Maximum Nano .bin size is 30720 bytes.</div><input id="nanoFile" class="file-input" type="file" accept=".bin,application/octet-stream"><div class="controls"><button id="nanoFlash" class="flashBtn" onclick="uploadFirmware('nano')">FLASH NANO .BIN</button></div></div>
  <div class="card wide"><div class="label">Firmware update status</div><div id="flashResult" class="flash-result">Ready.</div><div class="foot">Keep power connected for the entire update. Use only firmware built for the correct target device.</div></div>
 </div></section>

 <section id="tab-system" class="tab-page"><div class="grid"><div class="card wide"><div class="label">Connection</div><div class="system-line"><span>Nano UART</span><span id="link">--</span></div><div class="system-line"><span>Status age</span><span id="age">--</span></div><div class="system-line"><span>Network mode</span><span class="ok">HOTSPOT</span></div><div class="system-line"><span>Hotspot IP</span><span id="ips">--</span></div></div><div class="card wide"><div class="label">Last Nano line</div><pre id="raw">--</pre></div><div class="card wide"><div class="label">Legacy updater page</div><div class="controls"><button class="secondary" onclick="location.href='/firmware'">OPEN STANDALONE UPDATER</button></div></div></div></section>
 <div class="foot">Hotspot: NanoCharger &nbsp;|&nbsp; Web: 192.168.4.1</div>
</div>
<script>
const intervalMs=%REFRESH_MS%;const tempSamples=60;let lastNanoLineSeen='';let terminalLines=['Nano terminal ready. Type HELP to list commands.'];let flashBusy=false;
function fmt(v,suffix,digits=2){return(v===null||v===undefined)?'INVALID':Number(v).toFixed(digits)+suffix}
function setText(id,t){const e=document.getElementById(id);if(e)e.textContent=t}
function pointText(d,i){const ok=d.tempPointValid[i]&&d.tempPointRawValid[i];return ok?(Number(d.tempPointExternalC[i]).toFixed(2)+' °C / raw '+Number(d.tempPointRaw[i]).toFixed(2)):'--'}
function showTab(name){document.querySelectorAll('.tab-page').forEach(e=>e.classList.remove('active'));document.querySelectorAll('.tab-btn').forEach(e=>e.classList.remove('active'));document.getElementById('tab-'+name).classList.add('active');const b=document.querySelector('.tab-btn[data-tab="'+name+'"]');if(b)b.classList.add('active');if(name==='terminal')setTimeout(()=>document.getElementById('terminalInput').focus(),50)}
function renderTerminal(){const out=document.getElementById('terminalOutput');out.textContent=terminalLines.join('\n');out.scrollTop=out.scrollHeight}
function appendTerminal(line){if(!line)return;terminalLines.push(line);if(terminalLines.length>80)terminalLines.splice(0,terminalLines.length-80);renderTerminal()}
function clearTerminal(){terminalLines=[];renderTerminal()}
function setFlashBusy(v){flashBusy=v;document.getElementById('espFlash').disabled=v;document.getElementById('nanoFlash').disabled=v}
function setFlashResult(text,cls=''){const e=document.getElementById('flashResult');e.textContent=text;e.className='flash-result '+cls}
async function uploadFirmware(kind){if(flashBusy)return;const input=document.getElementById(kind==='esp'?'espFile':'nanoFile');if(!input.files||input.files.length===0){setFlashResult('Choose a .bin file first.','bad');return}const file=input.files[0];if(!file.name.toLowerCase().endsWith('.bin')){setFlashResult('Firmware file must end in .bin.','bad');return}if(file.size===0){setFlashResult('Firmware file is empty.','bad');return}if(kind==='nano'&&file.size>30720){setFlashResult('Nano .bin is too large. Maximum is 30720 bytes.','bad');return}const data=new FormData();data.append('firmware',file,file.name);setFlashBusy(true);setFlashResult(kind==='esp'?'Uploading ESP32-WROOM firmware...':'Uploading and programming Nano firmware...','warn');try{const r=await fetch('/firmware/'+kind,{method:'POST',body:data,cache:'no-store'});const d=await r.json();if(!d.ok){setFlashResult('Update failed: '+d.message,'bad');setFlashBusy(false);return}if(kind==='esp'){setFlashResult(d.message+' ESP32-WROOM is rebooting. Reconnect to NanoCharger if needed.','ok');setTimeout(()=>{location.href='/'},6000)}else{setFlashResult(d.message,'ok');setFlashBusy(false);setTimeout(refreshNow,1000)}}catch(e){setFlashResult('Update connection error: '+String(e),'bad');setFlashBusy(false)}}
async function refreshNow(){try{const r=await fetch('/api/status?t='+Date.now(),{cache:'no-store'});const d=await r.json();setText('bat',fmt(d.batteryVolts,' V',2));setText('bt',d.batteryTempValid?fmt(d.batteryTempC,' °C',1):'INVALID');setText('nt',(d.nanoTempValid?fmt(d.nanoTempC,' °C',1):'INVALID')+' (raw '+Number(d.nanoTempRaw||0)+')');const ch=document.getElementById('charger');ch.textContent=d.chargerOn?'ON':'OFF';ch.className='value '+(d.chargerOn?'ok':'warn');setText('state',d.state);setText('mode','Mode: '+d.mode);const top=document.getElementById('topLink');top.textContent=d.nanoConnected?'NANO CONNECTED':'NANO OFFLINE';top.className='status-pill '+(d.nanoConnected?'ok':'bad');const tp=d.tempSyncPhase||'OFF';const ts=document.getElementById('tsyncState');ts.textContent=tp;ts.className='value small '+(tp==='COMPLETE'?'ok':(d.tempSyncActive?'warn':''));setText('tsyncDelta',d.tempSyncDeltaValid?fmt(d.tempSyncDeltaC,' °C',2):'--');setText('tp1',pointText(d,0));setText('tp1s','Samples: '+d.tempPointSamples[0]+' / '+tempSamples);setText('tp2',pointText(d,1));setText('tp2s','Samples: '+d.tempPointSamples[1]+' / '+tempSamples);setText('tp3',pointText(d,2));setText('tp3s','Samples: '+d.tempPointSamples[2]+' / '+tempSamples);
let ti='Put the external temperature sensor beside the Nano inside the case, then press START TEMP SYNC.';if(tp==='POINT1')ti='Point 1 of 3: charger is held OFF while 60 cold/baseline samples are collected.';if(tp==='WAIT_POINT2')ti='Point 1 complete. Charger is back in AUTO. Waiting for real CHARGING and about a 2 °C temperature rise.';if(tp==='POINT2')ti='Point 2 of 3: warmer charging point reached. Collecting 60 samples while charging.';if(tp==='WAIT_POINT3')ti='Point 2 complete. Keep charging. Waiting for another temperature rise before Point 3.';if(tp==='POINT3')ti='Point 3 of 3: collecting the final 60 samples while charging.';if(tp==='COMPLETE')ti='TEMP SYNC complete. Copy the four code lines below into PinsAndConfig.h and reflash the Nano.';setText('tsyncInstruction',ti);
const tcode=document.getElementById('tempResultCode');if(tp==='COMPLETE'&&d.tempSyncReady&&d.tempCalRawValid&&d.tempCalCValid&&d.tempCountsPerCValid){tcode.style.display='block';tcode.textContent='constexpr float INTERNAL_TEMP_CAL_RAW = '+Number(d.tempCalRaw).toFixed(2)+'f;\nconstexpr float INTERNAL_TEMP_CAL_C = '+Number(d.tempCalC).toFixed(2)+'f;\nconstexpr float INTERNAL_TEMP_COUNTS_PER_C = '+Number(d.tempCountsPerC).toFixed(4)+'f;\nconstexpr float INTERNAL_TEMP_CALIBRATION_OFFSET_C = 0.0f;'}else{tcode.style.display='none';tcode.textContent=''}
const vp=d.voltageCalPhase||'OFF';const vs=document.getElementById('vcalState');vs.textContent=vp;vs.className='value small '+(vp==='COMPLETE'?'ok':(d.voltageCalActive?'warn':''));setText('vcalNano',fmt(d.batteryVolts,' V',3));setText('vcalSamples',d.voltageCalSamples+' / 3');setText('vcalTarget',d.voltageCalTargetValid?fmt(d.voltageCalTargetV,' V',3):'--');let vi='Press START VOLTAGE CAL, then measure the battery directly at its terminals with your multimeter.';let wantsInput=false;if(vp==='INPUT1'){vi='Reading 1 of 3: enter the multimeter voltage now.';wantsInput=true}if(vp==='WAIT_RISE2')vi='Reading 1 saved. Leave it charging and wait for the voltage to rise.';if(vp==='INPUT2'){vi='Reading 2 of 3: measure again and enter the multimeter voltage now.';wantsInput=true}if(vp==='WAIT_RISE3')vi='Reading 2 saved. Keep waiting for another voltage rise.';if(vp==='INPUT3'){vi='Reading 3 of 3: enter the final multimeter voltage.';wantsInput=true}if(vp==='COMPLETE')vi='VOLTAGE CAL complete. Copy both lines below into PinsAndConfig.h and reflash the Nano.';setText('vcalInstruction',vi);document.getElementById('vcalInputRow').style.display=wantsInput?'block':'none';const vcode=document.getElementById('voltResultCode');if(vp==='COMPLETE'&&d.voltageCalScaleValid&&d.voltageCalOffsetValid){vcode.style.display='block';vcode.textContent='constexpr float BATTERY_VOLTAGE_CALIBRATION = '+Number(d.voltageCalScale).toFixed(6)+'f;\nconstexpr float BATTERY_VOLTAGE_OFFSET_VOLTS = '+Number(d.voltageCalOffsetV).toFixed(4)+'f;'}else{vcode.style.display='none';vcode.textContent=''}document.getElementById('syncStart').disabled=d.tempSyncActive||d.voltageCalActive;document.getElementById('syncStop').disabled=!d.tempSyncActive;document.getElementById('vcalStart').disabled=d.voltageCalActive||d.tempSyncActive;document.getElementById('vcalStop').disabled=!d.voltageCalActive;const ln=document.getElementById('link');ln.textContent=d.nanoConnected?'CONNECTED':'OFFLINE';ln.className=d.nanoConnected?'ok':'bad';setText('age',d.statusAgeMs===null?'No status received':d.statusAgeMs+' ms');setText('ips',d.hotspotIp);setText('raw',d.lastNanoLine||'--');if(d.lastNanoLine&&d.lastNanoLine!==lastNanoLineSeen){lastNanoLineSeen=d.lastNanoLine;appendTerminal('< '+d.lastNanoLine)}}catch(e){const top=document.getElementById('topLink');top.textContent='WEB UPDATE ERROR';top.className='status-pill bad';const ln=document.getElementById('link');ln.textContent='WEB UPDATE ERROR';ln.className='bad';setText('age',String(e))}}
async function cmd(c,terminalEcho=false){c=String(c||'').trim();if(!c)return false;const o=document.getElementById('cmdResult');o.textContent='Sending '+c+'...';if(terminalEcho)appendTerminal('> '+c.toUpperCase());try{const r=await fetch('/api/command?cmd='+encodeURIComponent(c),{method:'POST',cache:'no-store'});const d=await r.json();if(d.ok){o.textContent='Sent: '+d.command;if(terminalEcho)appendTerminal('[ESP] sent '+d.command);setTimeout(refreshNow,150);setTimeout(refreshNow,400);setTimeout(refreshNow,800);return true}o.textContent='Error: '+d.error;if(terminalEcho)appendTerminal('[ESP] ERROR '+d.error);return false}catch(e){o.textContent='Command failed: '+e;if(terminalEcho)appendTerminal('[ESP] command failed: '+e);return false}}
async function sendTerminal(){const input=document.getElementById('terminalInput');const c=input.value.trim();if(!c)return;input.value='';await cmd(c,true);input.focus()}function terminalQuick(c){document.getElementById('terminalInput').value=c;sendTerminal()}document.getElementById('terminalInput').addEventListener('keydown',e=>{if(e.key==='Enter'){e.preventDefault();sendTerminal()}});
async function submitVcal(){const input=document.getElementById('vactual');const v=Number(input.value);const o=document.getElementById('cmdResult');if(!Number.isFinite(v)||v<8||v>16){o.textContent='Enter the multimeter voltage between 8.00 and 16.00 V.';return}await cmd('VCAL SAMPLE '+v.toFixed(3));input.value=''}
renderTerminal();refreshNow();setInterval(refreshNow,intervalMs);
</script>
</body>
</html>
)HTML";

}

WebUi::WebUi(NanoLink& nanoLink, ChargerNetwork& network)
  : _nano(nanoLink),
    _network(network),
    _server(WEB_SERVER_PORT),
    _firmwareUpdate(nanoLink) {}

void WebUi::begin() {
  _server.on("/", HTTP_GET, [this]() { handleRoot(); });
  _server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  _server.on("/api/command", HTTP_POST, [this]() { handleCommand(); });
  _firmwareUpdate.begin(_server);
  _server.onNotFound([this]() { handleNotFound(); });
  _server.begin();
  if (ENABLE_DEBUG) Serial.println(F("Web server started"));
}

void WebUi::update() {
  _server.handleClient();
  _firmwareUpdate.update();
}

void WebUi::handleRoot() {
  String page = FPSTR(PAGE_HTML);
  page.replace("%REFRESH_MS%", String(WEB_REFRESH_MS));
  _server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  _server.send(200, "text/html", page);
}

void WebUi::handleStatus() {
  _server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  _server.send(200, "application/json", buildStatusJson());
}

void WebUi::handleCommand() {
  if (!_server.hasArg("cmd")) {
    _server.send(400, "application/json", "{\"ok\":false,\"error\":\"MISSING_COMMAND\"}");
    return;
  }

  String command = _server.arg("cmd");
  command.trim();
  command.toUpperCase();

  const bool allowed =
      command == "PING" || command == "STATUS" || command == "BATTERY" ||
      command == "TEMP" || command == "STATE" ||
      command == "TSYNC START" || command == "TSYNC STATUS" || command == "TSYNC STOP" ||
      command == "VCAL START" || command == "VCAL STATUS" || command == "VCAL STOP" ||
      command.startsWith("VCAL SAMPLE ") ||
      command == "STOP" || command == "AUTO" || command == "HELP";

  if (!allowed) {
    _server.send(403, "application/json", "{\"ok\":false,\"error\":\"COMMAND_NOT_ALLOWED\"}");
    return;
  }

  _nano.sendCommand(command);
  _server.send(200, "application/json", String("{\"ok\":true,\"command\":\"") + jsonEscape(command) + "\"}");
}

void WebUi::handleNotFound() {
  _server.sendHeader("Location", String("http://") + _network.hotspotIP().toString() + "/", true);
  _server.send(302, "text/plain", "");
}

String WebUi::buildStatusJson() const {
  const ChargerStatus& s = _nano.status();
  String json;
  json.reserve(1200);
  json += '{';
  json += "\"nanoConnected\":"; json += _nano.connected() ? "true" : "false";
  json += ",\"statusAgeMs\":"; if (s.receivedAtMs == 0) json += "null"; else json += String(_nano.statusAgeMs());
  json += ",\"batteryVolts\":"; if (s.valid) json += String(s.batteryVolts, 2); else json += "null";
  json += ",\"batteryTempValid\":"; json += s.batteryTempValid ? "true" : "false";
  json += ",\"batteryTempC\":"; if (s.batteryTempValid) json += String(s.batteryTempC, 1); else json += "null";
  json += ",\"nanoTempValid\":"; json += s.nanoTempValid ? "true" : "false";
  json += ",\"nanoTempC\":"; if (s.nanoTempValid) json += String(s.nanoTempC, 1); else json += "null";
  json += ",\"nanoTempRaw\":"; json += String(s.nanoTempRawAdc);
  json += ",\"chargerOn\":"; json += s.chargerOn ? "true" : "false";
  json += ",\"state\":\""; json += jsonEscape(s.state); json += '"';
  json += ",\"mode\":\""; json += jsonEscape(s.mode); json += '"';

  json += ",\"tempSyncActive\":"; json += s.tempSyncActive ? "true" : "false";
  json += ",\"tempSyncPhase\":\""; json += jsonEscape(s.tempSyncPhase); json += '"';
  json += ",\"tempSyncDeltaValid\":"; json += s.tempSyncDeltaValid ? "true" : "false";
  json += ",\"tempSyncDeltaC\":"; if (s.tempSyncDeltaValid) json += String(s.tempSyncDeltaC, 2); else json += "null";

  json += ",\"tempPointValid\":[";
  for (uint8_t i=0;i<3;++i){if(i)json+=',';json+=s.tempPointValid[i]?"true":"false";} json += ']';
  json += ",\"tempPointExternalC\":[";
  for (uint8_t i=0;i<3;++i){if(i)json+=',';if(s.tempPointValid[i])json+=String(s.tempPointExternalC[i],2);else json+="null";} json += ']';
  json += ",\"tempPointRawValid\":[";
  for (uint8_t i=0;i<3;++i){if(i)json+=',';json+=s.tempPointRawValid[i]?"true":"false";} json += ']';
  json += ",\"tempPointRaw\":[";
  for (uint8_t i=0;i<3;++i){if(i)json+=',';if(s.tempPointRawValid[i])json+=String(s.tempPointRaw[i],2);else json+="null";} json += ']';
  json += ",\"tempPointSamples\":[";
  for (uint8_t i=0;i<3;++i){if(i)json+=',';json+=String(s.tempPointSamples[i]);} json += ']';

  json += ",\"tempSyncReady\":"; json += s.tempSyncReady ? "true" : "false";
  json += ",\"tempCalRawValid\":"; json += s.tempCalRawValid ? "true" : "false";
  json += ",\"tempCalRaw\":"; if (s.tempCalRawValid) json += String(s.tempCalRaw, 2); else json += "null";
  json += ",\"tempCalCValid\":"; json += s.tempCalCValid ? "true" : "false";
  json += ",\"tempCalC\":"; if (s.tempCalCValid) json += String(s.tempCalC, 2); else json += "null";
  json += ",\"tempCountsPerCValid\":"; json += s.tempCountsPerCValid ? "true" : "false";
  json += ",\"tempCountsPerC\":"; if (s.tempCountsPerCValid) json += String(s.tempCountsPerC, 4); else json += "null";

  json += ",\"voltageCalActive\":"; json += s.voltageCalActive ? "true" : "false";
  json += ",\"voltageCalPhase\":\""; json += jsonEscape(s.voltageCalPhase); json += '"';
  json += ",\"voltageCalSamples\":"; json += String(s.voltageCalSamples);
  json += ",\"voltageCalTargetValid\":"; json += s.voltageCalTargetValid ? "true" : "false";
  json += ",\"voltageCalTargetV\":"; if (s.voltageCalTargetValid) json += String(s.voltageCalTargetV, 3); else json += "null";
  json += ",\"voltageCalReady\":"; json += s.voltageCalReady ? "true" : "false";
  json += ",\"voltageCalScaleValid\":"; json += s.voltageCalScaleValid ? "true" : "false";
  json += ",\"voltageCalScale\":"; if (s.voltageCalScaleValid) json += String(s.voltageCalScale, 6); else json += "null";
  json += ",\"voltageCalOffsetValid\":"; json += s.voltageCalOffsetValid ? "true" : "false";
  json += ",\"voltageCalOffsetV\":"; if (s.voltageCalOffsetValid) json += String(s.voltageCalOffsetV, 4); else json += "null";

  json += ",\"lastNanoLine\":\""; json += jsonEscape(_nano.lastLine()); json += '"';
  json += ",\"hotspotIp\":\""; json += _network.hotspotIP().toString(); json += '"';
  json += '}';
  return json;
}

String WebUi::jsonEscape(const String& input) {
  String out;
  out.reserve(input.length() + 8);
  for (size_t i = 0; i < input.length(); ++i) {
    const char c = input.charAt(i);
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"':  out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: if (static_cast<uint8_t>(c) >= 0x20) out += c; break;
    }
  }
  return out;
}
