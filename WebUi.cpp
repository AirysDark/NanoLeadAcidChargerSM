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
input{box-sizing:border-box;width:180px;max-width:100%;border:1px solid #46515d;border-radius:9px;background:#0f1419;color:#fff;padding:11px;font-size:1rem;margin:4px}
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
    <div class="label">Internal Temperature Calibration</div>
    <div id="tsyncState" class="value small">OFF</div>
    <div id="tsyncInstruction" class="instruction">Put the external temperature sensor beside the Nano inside the case, then press START TEMP SYNC.</div>
    <div class="row">Current external - Nano difference: <b id="tsyncDelta">--</b></div>
    <div class="row">Point 1: <b id="tp1">--</b></div><div id="tp1s" class="foot">Samples: 0 / 60</div>
    <div class="row">Point 2: <b id="tp2">--</b></div><div id="tp2s" class="foot">Samples: 0 / 60</div>
    <div class="row">Point 3: <b id="tp3">--</b></div><div id="tp3s" class="foot">Samples: 0 / 60</div>
    <pre id="tempResultCode" class="code" style="display:none"></pre>
    <button id="syncStart" onclick="cmd('TSYNC START')">START TEMP SYNC</button>
    <button id="syncStop" onclick="cmd('TSYNC STOP')">CANCEL TEMP SYNC</button>
    <div class="foot">Point 1 is measured with charging held OFF. The Nano then returns to AUTO, waits for real charging and a temperature rise, collects Point 2, waits for another rise, then collects Point 3. Each point averages 60 readings. It stops automatically and gives the exact calibration lines to reflash.</div>
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
  </div>

  <div class="card wide"><div class="label">Nano UART</div><div id="link" class="value small">--</div><div id="age" class="foot"></div></div>
  <div class="card wide"><div class="label">Network</div><div class="value small ok">HOTSPOT</div><div id="ips" class="foot"></div></div>
  <div class="card wide"><div class="label">Controls</div><button id="auto" onclick="cmd('AUTO')">AUTO</button><button id="stop" onclick="cmd('STOP')">STOP CHARGING</button><button id="refresh" onclick="refreshNow()">REFRESH</button><div id="cmdResult" class="foot"></div></div>
  <div class="card wide"><div class="label">Last Nano line</div><pre id="raw">--</pre></div>
</div>
<div class="foot">Connect directly to the ESP8266 hotspot and open 192.168.4.1.</div>

<script>
const intervalMs=%REFRESH_MS%;
const tempSamples=60;
function fmt(v,suffix,digits=2){return(v===null||v===undefined)?'INVALID':Number(v).toFixed(digits)+suffix}
function setText(id,t){document.getElementById(id).textContent=t}
function pointText(d,i){const ok=d.tempPointValid[i]&&d.tempPointRawValid[i];return ok?(Number(d.tempPointExternalC[i]).toFixed(2)+' °C / raw '+Number(d.tempPointRaw[i]).toFixed(2)):'--'}

async function refreshNow(){
 try{
  const r=await fetch('/api/status?t='+Date.now(),{cache:'no-store'});
  const d=await r.json();
  setText('bat',fmt(d.batteryVolts,' V',2));
  setText('bt',d.batteryTempValid?fmt(d.batteryTempC,' °C',1):'INVALID');
  setText('nt',d.nanoTempValid?fmt(d.nanoTempC,' °C',1):'INVALID');
  const ch=document.getElementById('charger');ch.textContent=d.chargerOn?'ON':'OFF';ch.className='value '+(d.chargerOn?'ok':'warn');
  setText('state',d.state);setText('mode','Mode: '+d.mode);

  const tp=d.tempSyncPhase||'OFF';
  const ts=document.getElementById('tsyncState');ts.textContent=tp;ts.className='value small '+(tp==='COMPLETE'?'ok':(d.tempSyncActive?'warn':''));
  setText('tsyncDelta',d.tempSyncDeltaValid?fmt(d.tempSyncDeltaC,' °C',2):'--');
  setText('tp1',pointText(d,0));setText('tp1s','Samples: '+d.tempPointSamples[0]+' / '+tempSamples);
  setText('tp2',pointText(d,1));setText('tp2s','Samples: '+d.tempPointSamples[1]+' / '+tempSamples);
  setText('tp3',pointText(d,2));setText('tp3s','Samples: '+d.tempPointSamples[2]+' / '+tempSamples);

  let ti='Put the external temperature sensor beside the Nano inside the case, then press START TEMP SYNC.';
  if(tp==='POINT1')ti='Point 1 of 3: charger is held OFF while 60 cold/baseline samples are collected.';
  if(tp==='WAIT_POINT2')ti='Point 1 complete. Charger is back in AUTO. Let the battery charge; waiting for real CHARGING and about a 2 °C temperature rise.';
  if(tp==='POINT2')ti='Point 2 of 3: warmer charging point reached. Collecting 60 samples while charging.';
  if(tp==='WAIT_POINT3')ti='Point 2 complete. Keep charging. Waiting for another temperature rise before Point 3.';
  if(tp==='POINT3')ti='Point 3 of 3: higher-temperature point reached. Collecting the final 60 samples while charging.';
  if(tp==='COMPLETE')ti='TEMP SYNC complete. Copy all four code lines below into NanoLeadAcidCharger/PinsAndConfig.h and reflash the Nano. Then move the external sensor back to the battery.';
  setText('tsyncInstruction',ti);

  const tcode=document.getElementById('tempResultCode');
  if(tp==='COMPLETE'&&d.tempSyncReady&&d.tempCalRawValid&&d.tempCalCValid&&d.tempCountsPerCValid){
    tcode.style.display='block';
    tcode.textContent=
      'constexpr float INTERNAL_TEMP_CAL_RAW = '+Number(d.tempCalRaw).toFixed(2)+'f;\n'+
      'constexpr float INTERNAL_TEMP_CAL_C = '+Number(d.tempCalC).toFixed(2)+'f;\n'+
      'constexpr float INTERNAL_TEMP_COUNTS_PER_C = '+Number(d.tempCountsPerC).toFixed(4)+'f;\n'+
      'constexpr float INTERNAL_TEMP_CALIBRATION_OFFSET_C = 0.0f;';
  }else{tcode.style.display='none';tcode.textContent='';}

  const vp=d.voltageCalPhase||'OFF';
  const vs=document.getElementById('vcalState');vs.textContent=vp;vs.className='value small '+(vp==='COMPLETE'?'ok':(d.voltageCalActive?'warn':''));
  setText('vcalNano',fmt(d.batteryVolts,' V',3));
  setText('vcalSamples',d.voltageCalSamples+' / 3');
  setText('vcalTarget',d.voltageCalTargetValid?fmt(d.voltageCalTargetV,' V',3):'--');

  let vi='Press START VOLTAGE CAL, then measure the battery directly at its terminals with your multimeter.';let wantsInput=false;
  if(vp==='INPUT1'){vi='Reading 1 of 3: enter the multimeter voltage now.';wantsInput=true;}
  if(vp==='WAIT_RISE2')vi='Reading 1 saved. Leave it charging and wait for the voltage to rise.';
  if(vp==='INPUT2'){vi='Reading 2 of 3: measure again and enter the multimeter voltage now.';wantsInput=true;}
  if(vp==='WAIT_RISE3')vi='Reading 2 saved. Keep waiting for another voltage rise.';
  if(vp==='INPUT3'){vi='Reading 3 of 3: measure again and enter the final multimeter voltage.';wantsInput=true;}
  if(vp==='COMPLETE')vi='VOLTAGE CAL complete. Copy both lines below into PinsAndConfig.h and reflash the Nano.';
  setText('vcalInstruction',vi);
  document.getElementById('vcalInputRow').style.display=wantsInput?'block':'none';

  const vcode=document.getElementById('voltResultCode');
  if(vp==='COMPLETE'&&d.voltageCalScaleValid&&d.voltageCalOffsetValid){
    vcode.style.display='block';
    vcode.textContent='constexpr float BATTERY_VOLTAGE_CALIBRATION = '+Number(d.voltageCalScale).toFixed(6)+'f;\nconstexpr float BATTERY_VOLTAGE_OFFSET_VOLTS = '+Number(d.voltageCalOffsetV).toFixed(4)+'f;';
  }else{vcode.style.display='none';vcode.textContent='';}

  document.getElementById('syncStart').disabled=d.tempSyncActive||d.voltageCalActive;
  document.getElementById('syncStop').disabled=!d.tempSyncActive;
  document.getElementById('vcalStart').disabled=d.voltageCalActive||d.tempSyncActive;
  document.getElementById('vcalStop').disabled=!d.voltageCalActive;

  const ln=document.getElementById('link');ln.textContent=d.nanoConnected?'CONNECTED':'OFFLINE';ln.className='value small '+(d.nanoConnected?'ok':'bad');
  setText('age',d.statusAgeMs===null?'No status received':'Last status '+d.statusAgeMs+' ms ago');
  setText('ips','Hotspot IP: '+d.hotspotIp);setText('raw',d.lastNanoLine||'--');
 }catch(e){const ln=document.getElementById('link');ln.textContent='WEB UPDATE ERROR';ln.className='value small bad';setText('age',String(e))}
}

async function cmd(c){
 const o=document.getElementById('cmdResult');o.textContent='Sending '+c+'...';
 try{const r=await fetch('/api/command?cmd='+encodeURIComponent(c),{method:'POST',cache:'no-store'});const d=await r.json();o.textContent=d.ok?'Sent: '+d.command:'Error: '+d.error;setTimeout(refreshNow,350)}catch(e){o.textContent='Command failed: '+e}
}

async function submitVcal(){
 const input=document.getElementById('vactual');const v=Number(input.value);const o=document.getElementById('cmdResult');
 if(!Number.isFinite(v)||v<8||v>16){o.textContent='Enter the multimeter voltage between 8.00 and 16.00 V.';return;}
 await cmd('VCAL SAMPLE '+v.toFixed(3));input.value='';
}

refreshNow();setInterval(refreshNow,intervalMs);
</script>
</body>
</html>
)HTML";

}

WebUi::WebUi(NanoLink& nanoLink, NetworkManager& network)
  : _nano(nanoLink), _network(network), _server(WEB_SERVER_PORT) {}

void WebUi::begin() {
  _server.on("/", HTTP_GET, [this]() { handleRoot(); });
  _server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  _server.on("/api/command", HTTP_POST, [this]() { handleCommand(); });
  _server.onNotFound([this]() { handleNotFound(); });
  _server.begin();
  if (ENABLE_DEBUG) Serial.println(F("Web server started"));
}

void WebUi::update() { _server.handleClient(); }

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
