#include "WebUi.h"
#include "Config.h"
#include "NanoLink.h"
#include "NetworkManager.h"

namespace {

const char PAGE_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Nano Lead-Acid Charger</title>
<style>
:root{color-scheme:dark;background:#101317;color:#eef2f5;font-family:Arial,sans-serif}
body{margin:0;padding:18px;max-width:780px;margin-inline:auto}
h1{font-size:1.45rem;margin:0 0 4px}.sub{color:#9aa7b2;margin-bottom:18px}
.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}
.card{background:#191e24;border:1px solid #303841;border-radius:12px;padding:14px}.wide{grid-column:1/-1}
.label{font-size:.78rem;color:#97a6b2;text-transform:uppercase;letter-spacing:.06em}.value{font-size:1.55rem;font-weight:700;margin-top:5px;word-break:break-word}.small{font-size:1rem}
.ok{color:#71dc8c}.bad{color:#ff7878}.warn{color:#ffd36a}
button{border:0;border-radius:10px;padding:12px 18px;margin:4px;font-size:1rem;font-weight:700;cursor:pointer}button:disabled{opacity:.45;cursor:default}
#auto,#syncStart,#vcalStart,#vcalSubmit{background:#4fc36a;color:#07120a}#stop,#syncStop,#vcalStop{background:#e05252;color:white}#refresh{background:#39434d;color:white}
input{box-sizing:border-box;width:170px;max-width:100%;border:1px solid #46515d;border-radius:9px;background:#0f1419;color:#fff;padding:11px;font-size:1rem;margin:4px}
.foot{color:#87939e;font-size:.8rem;margin-top:10px}.row{margin-top:7px;font-size:.95rem}.row b{color:#eef2f5}
.instruction{margin-top:10px;padding:10px;border-radius:8px;background:#11161b;line-height:1.4}
.code{background:#0a0d10;border:1px solid #343d46;border-radius:8px;padding:10px;margin-top:10px;white-space:pre-wrap;word-break:break-word;color:#aee9ba}
pre{white-space:pre-wrap;word-break:break-word;margin:0;color:#aeb9c2}@media(max-width:520px){.grid{grid-template-columns:1fr}}
</style>
</head>
<body>
<h1>Nano Lead-Acid Charger</h1>
<div class="sub">ESP8266 direct hotspot monitor</div>
<div class="grid">
  <div class="card"><div class="label">Battery</div><div id="bat" class="value">--</div></div>
  <div class="card"><div class="label">Charger</div><div id="charger" class="value">--</div></div>
  <div class="card"><div class="label">External temp sensor</div><div id="bt" class="value">--</div></div>
  <div class="card"><div class="label">Nano / charger temp</div><div id="nt" class="value">--</div></div>
  <div class="card wide"><div class="label">State</div><div id="state" class="value small">--</div><div id="mode" class="foot"></div></div>

  <div class="card wide">
    <div class="label">Temperature Sync</div>
    <div id="tsyncState" class="value small">OFF</div>
    <div id="tsyncInstruction" class="instruction">Put the external sensor beside the Nano inside the case, then press START TEMP SYNC.</div>
    <div class="row">Current sensor difference: <b id="tsyncDelta">--</b></div>
    <div class="row">Stage 1 / charger OFF average: <b id="tsyncBase">--</b></div>
    <div id="tsyncBaseSamples" class="foot">Baseline samples: 0 / 60</div>
    <div class="row">Stage 2 / charging average: <b id="tsyncCharge">--</b></div>
    <div id="tsyncChargeSamples" class="foot">Charging samples: 0 / 60</div>
    <div class="row">Final correction: <b id="tsyncFinal">--</b></div>
    <div class="row">Recommended Nano offset: <b id="tsyncNew">--</b></div>
    <pre id="tempResultCode" class="code" style="display:none"></pre>
    <button id="syncStart" onclick="cmd('TSYNC START')">START TEMP SYNC</button>
    <button id="syncStop" onclick="cmd('TSYNC STOP')">CANCEL TEMP SYNC</button>
    <div class="foot">Stage 1 collects 60 samples with charging held OFF. Stage 2 waits for real CHARGING and collects another 60. It then stops automatically.</div>
  </div>

  <div class="card wide">
    <div class="label">Battery Voltage Divider Calibration</div>
    <div id="vcalState" class="value small">OFF</div>
    <div id="vcalInstruction" class="instruction">Press START VOLTAGE CAL, then measure the battery directly at its terminals with your multimeter.</div>
    <div class="row">Nano currently reports: <b id="vcalNano">--</b></div>
    <div class="row">Saved readings: <b id="vcalSamples">0 / 3</b></div>
    <div class="row">Next Nano target: <b id="vcalTarget">--</b></div>
    <div id="vcalInputRow" style="display:none;margin-top:8px">
      <input id="vactual" type="number" min="8" max="16" step="0.01" inputmode="decimal" placeholder="Actual volts e.g. 12.07">
      <button id="vcalSubmit" onclick="submitVcal()">SAVE READING</button>
    </div>
    <pre id="voltResultCode" class="code" style="display:none"></pre>
    <button id="vcalStart" onclick="cmd('VCAL START')">START VOLTAGE CAL</button>
    <button id="vcalStop" onclick="cmd('VCAL STOP')">CANCEL VOLTAGE CAL</button>
    <div class="foot">The Nano stores its own voltage at the exact moment you submit each multimeter reading. After reading 1 it waits for the battery voltage to rise, asks for reading 2, waits for another rise, then asks for reading 3. It calculates a 3-point correction and gives you the exact two lines to put into PinsAndConfig.h before reflashing.</div>
  </div>

  <div class="card wide"><div class="label">Nano UART</div><div id="link" class="value small">--</div><div id="age" class="foot"></div></div>
  <div class="card wide"><div class="label">Network</div><div class="value small ok">HOTSPOT</div><div id="ips" class="foot"></div></div>
  <div class="card wide"><div class="label">Controls</div><button id="auto" onclick="cmd('AUTO')">AUTO</button><button id="stop" onclick="cmd('STOP')">STOP CHARGING</button><button id="refresh" onclick="refreshNow()">REFRESH</button><div id="cmdResult" class="foot"></div></div>
  <div class="card wide"><div class="label">Last Nano line</div><pre id="raw">--</pre></div>
</div>
<div class="foot">Connect directly to the ESP8266 hotspot and open 192.168.4.1.</div>

<script>
const intervalMs=%REFRESH_MS%;
const tempTargetSamples=60;
function fmt(v,suffix,digits=2){return (v===null||v===undefined)?'INVALID':Number(v).toFixed(digits)+suffix}
function setText(id,t){document.getElementById(id).textContent=t}

async function refreshNow(){
 try{
  const r=await fetch('/api/status?t='+Date.now(),{cache:'no-store'});
  const d=await r.json();

  setText('bat',fmt(d.batteryVolts,' V',2));
  setText('bt',d.batteryTempValid?fmt(d.batteryTempC,' °C',1):'INVALID');
  setText('nt',d.nanoTempValid?fmt(d.nanoTempC,' °C',1):'INVALID');
  const ch=document.getElementById('charger');
  ch.textContent=d.chargerOn?'ON':'OFF';
  ch.className='value '+(d.chargerOn?'ok':'warn');
  setText('state',d.state);
  setText('mode','Mode: '+d.mode);

  // Temperature sync UI.
  const tp=d.tempSyncPhase||'OFF';
  const ts=document.getElementById('tsyncState');
  ts.textContent=tp;
  ts.className='value small '+(tp==='COMPLETE'?'ok':(d.tempSyncActive?'warn':''));
  setText('tsyncDelta',d.tempSyncDeltaValid?fmt(d.tempSyncDeltaC,' °C',2):'--');
  setText('tsyncBase',d.tempSyncBaselineValid?fmt(d.tempSyncBaselineC,' °C',2):'--');
  setText('tsyncBaseSamples','Baseline samples: '+d.tempSyncBaselineSamples+' / '+tempTargetSamples);
  setText('tsyncCharge',d.tempSyncChargeValid?fmt(d.tempSyncChargeC,' °C',2):'--');
  setText('tsyncChargeSamples','Charging samples: '+d.tempSyncChargeSamples+' / '+tempTargetSamples);
  setText('tsyncFinal',d.tempSyncFinalValid?fmt(d.tempSyncFinalC,' °C',2):'--');
  setText('tsyncNew',d.tempSyncNewOffsetValid?fmt(d.tempSyncNewOffsetC,' °C',2):'--');

  let ti='Put the external sensor beside the Nano inside the case, then press START TEMP SYNC.';
  if(tp==='BASELINE') ti='Stage 1 of 2: charger is held OFF while 60 baseline samples are collected.';
  if(tp==='WAIT_CHARGE') ti='Stage 1 complete. Connect a battery that needs charging. The Nano is back in AUTO and is waiting for CHARGER=ON.';
  if(tp==='CHARGING') ti='Stage 2 of 2: charging detected. Leave the sensor beside the Nano while 60 charging samples are collected.';
  if(tp==='COMPLETE') ti='TEMP SYNC complete. Copy the code line below into NanoLeadAcidCharger/PinsAndConfig.h and reflash the Nano.';
  setText('tsyncInstruction',ti);

  const tcode=document.getElementById('tempResultCode');
  if(tp==='COMPLETE'&&d.tempSyncNewOffsetValid){
    tcode.style.display='block';
    tcode.textContent='constexpr float INTERNAL_TEMP_CALIBRATION_OFFSET_C = '+Number(d.tempSyncNewOffsetC).toFixed(2)+'f;';
  }else{tcode.style.display='none';tcode.textContent='';}

  // Voltage calibration UI.
  const vp=d.voltageCalPhase||'OFF';
  const vs=document.getElementById('vcalState');
  vs.textContent=vp;
  vs.className='value small '+(vp==='COMPLETE'?'ok':(d.voltageCalActive?'warn':''));
  setText('vcalNano',fmt(d.batteryVolts,' V',3));
  setText('vcalSamples',d.voltageCalSamples+' / 3');
  setText('vcalTarget',d.voltageCalTargetValid?fmt(d.voltageCalTargetV,' V',3):'--');

  let vi='Press START VOLTAGE CAL, then measure the battery directly at its terminals with your multimeter.';
  let wantsInput=false;
  if(vp==='INPUT1'){vi='Reading 1 of 3: measure the battery at its terminals with the multimeter and enter the actual voltage now.';wantsInput=true;}
  if(vp==='WAIT_RISE2') vi='Reading 1 saved. Leave the battery charging and wait. The Nano will ask for reading 2 after its measured voltage reaches the target shown above.';
  if(vp==='INPUT2'){vi='Reading 2 of 3: voltage has risen enough. Measure the battery again and enter the multimeter voltage now.';wantsInput=true;}
  if(vp==='WAIT_RISE3') vi='Reading 2 saved. Keep waiting while the battery voltage rises again. The Nano will ask for reading 3 automatically.';
  if(vp==='INPUT3'){vi='Reading 3 of 3: measure the battery again and enter the multimeter voltage now. This final entry calculates the calibration.';wantsInput=true;}
  if(vp==='COMPLETE') vi='VOLTAGE CAL complete. Copy BOTH code lines below into NanoLeadAcidCharger/PinsAndConfig.h, replace the old values, then reflash the Nano.';
  setText('vcalInstruction',vi);

  const inputRow=document.getElementById('vcalInputRow');
  inputRow.style.display=wantsInput?'block':'none';
  document.getElementById('vcalSubmit').disabled=!wantsInput;

  const vcode=document.getElementById('voltResultCode');
  if(vp==='COMPLETE'&&d.voltageCalScaleValid&&d.voltageCalOffsetValid){
    vcode.style.display='block';
    vcode.textContent=
      'constexpr float BATTERY_VOLTAGE_CALIBRATION = '+Number(d.voltageCalScale).toFixed(6)+'f;\n'+
      'constexpr float BATTERY_VOLTAGE_OFFSET_VOLTS = '+Number(d.voltageCalOffsetV).toFixed(4)+'f;';
  }else{vcode.style.display='none';vcode.textContent='';}

  document.getElementById('syncStart').disabled=d.tempSyncActive||d.voltageCalActive;
  document.getElementById('syncStop').disabled=!d.tempSyncActive;
  document.getElementById('vcalStart').disabled=d.voltageCalActive||d.tempSyncActive;
  document.getElementById('vcalStop').disabled=!d.voltageCalActive;

  const ln=document.getElementById('link');
  ln.textContent=d.nanoConnected?'CONNECTED':'OFFLINE';
  ln.className='value small '+(d.nanoConnected?'ok':'bad');
  setText('age',d.statusAgeMs===null?'No status received':'Last status '+d.statusAgeMs+' ms ago');
  setText('ips','Hotspot IP: '+d.hotspotIp);
  setText('raw',d.lastNanoLine||'--');
 }catch(e){
  const ln=document.getElementById('link');
  ln.textContent='WEB UPDATE ERROR';
  ln.className='value small bad';
  setText('age',String(e));
 }
}

async function cmd(c){
 const o=document.getElementById('cmdResult');
 o.textContent='Sending '+c+'...';
 try{
  const r=await fetch('/api/command?cmd='+encodeURIComponent(c),{method:'POST',cache:'no-store'});
  const d=await r.json();
  o.textContent=d.ok?'Sent: '+d.command:'Error: '+d.error;
  setTimeout(refreshNow,350);
 }catch(e){o.textContent='Command failed: '+e}
}

async function submitVcal(){
 const input=document.getElementById('vactual');
 const v=Number(input.value);
 const o=document.getElementById('cmdResult');
 if(!Number.isFinite(v)||v<8||v>16){o.textContent='Enter the multimeter voltage between 8.00 and 16.00 V.';return;}
 await cmd('VCAL SAMPLE '+v.toFixed(3));
 input.value='';
}

refreshNow();
setInterval(refreshNow,intervalMs);
</script>
</body>
</html>
)HTML";

}  // namespace

WebUi::WebUi(NanoLink& nanoLink, NetworkManager& network)
  : _nano(nanoLink),
    _network(network),
    _server(WEB_SERVER_PORT) {
}

void WebUi::begin() {
  _server.on("/", HTTP_GET, [this]() { handleRoot(); });
  _server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  _server.on("/api/command", HTTP_POST, [this]() { handleCommand(); });
  _server.onNotFound([this]() { handleNotFound(); });
  _server.begin();

  if (ENABLE_DEBUG) Serial.println(F("Web server started"));
}

void WebUi::update() {
  _server.handleClient();
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
  _server.send(200,
               "application/json",
               String("{\"ok\":true,\"command\":\"") + jsonEscape(command) + "\"}");
}

void WebUi::handleNotFound() {
  _server.sendHeader("Location", String("http://") + _network.hotspotIP().toString() + "/", true);
  _server.send(302, "text/plain", "");
}

String WebUi::buildStatusJson() const {
  const ChargerStatus& s = _nano.status();

  String json;
  json.reserve(1100);
  json += '{';

  json += "\"nanoConnected\":";
  json += _nano.connected() ? "true" : "false";

  json += ",\"statusAgeMs\":";
  if (s.receivedAtMs == 0) json += "null";
  else json += String(_nano.statusAgeMs());

  json += ",\"batteryVolts\":";
  if (s.valid) json += String(s.batteryVolts, 3);
  else json += "null";

  json += ",\"batteryTempValid\":";
  json += s.batteryTempValid ? "true" : "false";
  json += ",\"batteryTempC\":";
  if (s.batteryTempValid) json += String(s.batteryTempC, 1);
  else json += "null";

  json += ",\"nanoTempValid\":";
  json += s.nanoTempValid ? "true" : "false";
  json += ",\"nanoTempC\":";
  if (s.nanoTempValid) json += String(s.nanoTempC, 1);
  else json += "null";

  json += ",\"chargerOn\":";
  json += s.chargerOn ? "true" : "false";
  json += ",\"state\":\"";
  json += jsonEscape(s.state);
  json += "\",\"mode\":\"";
  json += jsonEscape(s.mode);
  json += '"';

  // Temperature sync.
  json += ",\"tempSyncActive\":";
  json += s.tempSyncActive ? "true" : "false";
  json += ",\"tempSyncPhase\":\"";
  json += jsonEscape(s.tempSyncPhase);
  json += '"';
  json += ",\"tempSyncDeltaValid\":";
  json += s.tempSyncDeltaValid ? "true" : "false";
  json += ",\"tempSyncDeltaC\":";
  if (s.tempSyncDeltaValid) json += String(s.tempSyncDeltaC, 2); else json += "null";
  json += ",\"tempSyncBaselineValid\":";
  json += s.tempSyncBaselineValid ? "true" : "false";
  json += ",\"tempSyncBaselineC\":";
  if (s.tempSyncBaselineValid) json += String(s.tempSyncBaselineC, 2); else json += "null";
  json += ",\"tempSyncBaselineSamples\":";
  json += String(s.tempSyncBaselineSamples);
  json += ",\"tempSyncChargeValid\":";
  json += s.tempSyncChargeValid ? "true" : "false";
  json += ",\"tempSyncChargeC\":";
  if (s.tempSyncChargeValid) json += String(s.tempSyncChargeC, 2); else json += "null";
  json += ",\"tempSyncChargeSamples\":";
  json += String(s.tempSyncChargeSamples);
  json += ",\"tempSyncFinalValid\":";
  json += s.tempSyncFinalValid ? "true" : "false";
  json += ",\"tempSyncFinalC\":";
  if (s.tempSyncFinalValid) json += String(s.tempSyncFinalC, 2); else json += "null";
  json += ",\"tempSyncReady\":";
  json += s.tempSyncReady ? "true" : "false";
  json += ",\"tempSyncNewOffsetValid\":";
  json += s.tempSyncNewOffsetValid ? "true" : "false";
  json += ",\"tempSyncNewOffsetC\":";
  if (s.tempSyncNewOffsetValid) json += String(s.tempSyncNewOffsetC, 2); else json += "null";

  // Voltage divider calibration.
  json += ",\"voltageCalActive\":";
  json += s.voltageCalActive ? "true" : "false";
  json += ",\"voltageCalPhase\":\"";
  json += jsonEscape(s.voltageCalPhase);
  json += '"';
  json += ",\"voltageCalSamples\":";
  json += String(s.voltageCalSamples);
  json += ",\"voltageCalTargetValid\":";
  json += s.voltageCalTargetValid ? "true" : "false";
  json += ",\"voltageCalTargetV\":";
  if (s.voltageCalTargetValid) json += String(s.voltageCalTargetV, 3); else json += "null";
  json += ",\"voltageCalReady\":";
  json += s.voltageCalReady ? "true" : "false";
  json += ",\"voltageCalScaleValid\":";
  json += s.voltageCalScaleValid ? "true" : "false";
  json += ",\"voltageCalScale\":";
  if (s.voltageCalScaleValid) json += String(s.voltageCalScale, 6); else json += "null";
  json += ",\"voltageCalOffsetValid\":";
  json += s.voltageCalOffsetValid ? "true" : "false";
  json += ",\"voltageCalOffsetV\":";
  if (s.voltageCalOffsetValid) json += String(s.voltageCalOffsetV, 4); else json += "null";

  json += ",\"lastNanoLine\":\"";
  json += jsonEscape(_nano.lastLine());
  json += '"';
  json += ",\"hotspotIp\":\"";
  json += _network.hotspotIP().toString();
  json += '"';

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
      default:
        if (static_cast<uint8_t>(c) >= 0x20) out += c;
        break;
    }
  }

  return out;
}
