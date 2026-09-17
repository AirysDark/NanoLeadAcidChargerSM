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
body{margin:0;padding:18px;max-width:760px;margin-inline:auto}
h1{font-size:1.45rem;margin:0 0 4px}.sub{color:#9aa7b2;margin-bottom:18px}
.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}.card{background:#191e24;border:1px solid #303841;border-radius:12px;padding:14px}.wide{grid-column:1/-1}.label{font-size:.78rem;color:#97a6b2;text-transform:uppercase;letter-spacing:.06em}.value{font-size:1.55rem;font-weight:700;margin-top:5px;word-break:break-word}.small{font-size:1rem}.ok{color:#71dc8c}.bad{color:#ff7878}.warn{color:#ffd36a}button{border:0;border-radius:10px;padding:12px 18px;margin:4px;font-size:1rem;font-weight:700;cursor:pointer}button:disabled{opacity:.45;cursor:default}#auto,#syncStart{background:#4fc36a;color:#07120a}#stop,#syncStop{background:#e05252;color:white}#refresh{background:#39434d;color:white}.foot{color:#87939e;font-size:.8rem;margin-top:10px}.syncrow{margin-top:7px;font-size:.95rem}.syncrow b{color:#eef2f5}.instruction{margin-top:10px;padding:10px;border-radius:8px;background:#11161b;line-height:1.4}.code{background:#0a0d10;border:1px solid #343d46;border-radius:8px;padding:10px;margin-top:10px;white-space:pre-wrap;word-break:break-word;color:#aee9ba}pre{white-space:pre-wrap;word-break:break-word;margin:0;color:#aeb9c2}@media(max-width:520px){.grid{grid-template-columns:1fr}}
</style>
</head>
<body>
<h1>Nano Lead-Acid Charger</h1>
<div class="sub">ESP8266 direct hotspot monitor</div>
<div class="grid">
  <div class="card"><div class="label">Battery</div><div id="bat" class="value">--</div></div>
  <div class="card"><div class="label">Charger</div><div id="charger" class="value">--</div></div>
  <div class="card"><div class="label">External temp sensor</div><div id="bt" class="value">--</div><div class="foot">Put this sensor beside the Nano during TEMP SYNC.</div></div>
  <div class="card"><div class="label">Nano / charger temp</div><div id="nt" class="value">--</div></div>
  <div class="card wide"><div class="label">State</div><div id="state" class="value small">--</div><div id="mode" class="foot"></div></div>

  <div class="card wide">
    <div class="label">Temperature Sync</div>
    <div id="tsyncState" class="value small">OFF</div>
    <div id="tsyncInstruction" class="instruction">Put the external sensor beside the Nano inside the case, then press START TEMP SYNC.</div>
    <div class="syncrow">Current sensor difference: <b id="tsyncDelta">--</b></div>
    <div class="syncrow">Stage 1 / charger OFF average: <b id="tsyncBase">--</b></div>
    <div id="tsyncBaseSamples" class="foot">Baseline samples: 0 / 60</div>
    <div class="syncrow">Stage 2 / charging average: <b id="tsyncCharge">--</b></div>
    <div id="tsyncChargeSamples" class="foot">Charging samples: 0 / 60</div>
    <div class="syncrow">Final correction: <b id="tsyncFinal">--</b></div>
    <div class="syncrow">Recommended Nano offset: <b id="tsyncNew">--</b></div>
    <pre id="resultCode" class="code" style="display:none"></pre>
    <button id="syncStart" onclick="cmd('TSYNC START')">START TEMP SYNC</button>
    <button id="syncStop" onclick="cmd('TSYNC STOP')">CANCEL TEMP SYNC</button>
    <div class="foot">The Nano automatically holds charging OFF for the first 60 samples. It then returns to AUTO and waits until it really reports CHARGING before collecting the second 60 samples. The test stops automatically when both stages are complete.</div>
  </div>

  <div class="card wide"><div class="label">Nano UART</div><div id="link" class="value small">--</div><div id="age" class="foot"></div></div>
  <div class="card wide"><div class="label">Network</div><div class="value small ok">HOTSPOT</div><div id="ips" class="foot"></div></div>
  <div class="card wide"><div class="label">Controls</div><button id="auto" onclick="cmd('AUTO')">AUTO</button><button id="stop" onclick="cmd('STOP')">STOP CHARGING</button><button id="refresh" onclick="refreshNow()">REFRESH</button><div id="cmdResult" class="foot"></div></div>
  <div class="card wide"><div class="label">Last Nano line</div><pre id="raw">--</pre></div>
</div>
<div class="foot">Connect directly to the ESP8266 hotspot and open 192.168.4.1.</div>
<script>
const intervalMs = %REFRESH_MS%;
const targetSamples = 60;
function fmt(v,suffix,digits=2){return (v===null||v===undefined)?'INVALID':Number(v).toFixed(digits)+suffix}
function setText(id,t){document.getElementById(id).textContent=t}
async function refreshNow(){
 try{
  const r=await fetch('/api/status?t='+Date.now(),{cache:'no-store'});
  const d=await r.json();
  setText('bat',fmt(d.batteryVolts,' V',2));
  setText('bt',d.batteryTempValid?fmt(d.batteryTempC,' °C',1):'INVALID');
  setText('nt',d.nanoTempValid?fmt(d.nanoTempC,' °C',1):'INVALID');
  const ch=document.getElementById('charger'); ch.textContent=d.chargerOn?'ON':'OFF'; ch.className='value '+(d.chargerOn?'ok':'warn');
  setText('state',d.state); setText('mode','Mode: '+d.mode);

  const phase=d.tempSyncPhase||'OFF';
  const ts=document.getElementById('tsyncState'); ts.textContent=phase; ts.className='value small '+(phase==='COMPLETE'?'ok':(d.tempSyncActive?'warn':''));
  setText('tsyncDelta',d.tempSyncDeltaValid?fmt(d.tempSyncDeltaC,' °C',2):'--');
  setText('tsyncBase',d.tempSyncBaselineValid?fmt(d.tempSyncBaselineC,' °C',2):'--');
  setText('tsyncBaseSamples','Baseline samples: '+d.tempSyncBaselineSamples+' / '+targetSamples);
  setText('tsyncCharge',d.tempSyncChargeValid?fmt(d.tempSyncChargeC,' °C',2):'--');
  setText('tsyncChargeSamples','Charging samples: '+d.tempSyncChargeSamples+' / '+targetSamples);
  setText('tsyncFinal',d.tempSyncFinalValid?fmt(d.tempSyncFinalC,' °C',2):'--');
  setText('tsyncNew',d.tempSyncNewOffsetValid?fmt(d.tempSyncNewOffsetC,' °C',2):'--');

  let instruction='Put the external sensor beside the Nano inside the case, then press START TEMP SYNC.';
  if(phase==='BASELINE') instruction='Stage 1 of 2: leave the sensor beside the Nano. The Nano is holding the charger OFF and collecting 60 baseline samples.';
  if(phase==='WAIT_CHARGE') instruction='Stage 1 complete. Connect a battery that needs charging (or use one low enough to charge). The Nano is back in AUTO and is waiting for CHARGER=ON. It will not force charging on.';
  if(phase==='CHARGING') instruction='Stage 2 of 2: charging detected. Leave the sensor beside the Nano while 60 charging samples are collected. If charging stops, sampling pauses until charging resumes.';
  if(phase==='COMPLETE') instruction='TEMP SYNC complete. Copy the exact code line below into NanoLeadAcidCharger/PinsAndConfig.h, reflash the Nano, then move the external sensor back to the battery.';
  setText('tsyncInstruction',instruction);

  const code=document.getElementById('resultCode');
  if(phase==='COMPLETE' && d.tempSyncNewOffsetValid){
    code.style.display='block';
    code.textContent='constexpr float INTERNAL_TEMP_CALIBRATION_OFFSET_C = '+Number(d.tempSyncNewOffsetC).toFixed(2)+'f;';
  }else{code.style.display='none';code.textContent='';}

  document.getElementById('syncStart').disabled=d.tempSyncActive;
  document.getElementById('syncStop').disabled=!d.tempSyncActive;

  const ln=document.getElementById('link'); ln.textContent=d.nanoConnected?'CONNECTED':'OFFLINE'; ln.className='value small '+(d.nanoConnected?'ok':'bad');
  setText('age',d.statusAgeMs===null?'No status received':'Last status '+d.statusAgeMs+' ms ago');
  setText('ips','Hotspot IP: '+d.hotspotIp);
  setText('raw',d.lastNanoLine||'--');
 }catch(e){const ln=document.getElementById('link');ln.textContent='WEB UPDATE ERROR';ln.className='value small bad';setText('age',String(e))}
}
async function cmd(c){
 const o=document.getElementById('cmdResult');o.textContent='Sending '+c+'...';
 try{const r=await fetch('/api/command?cmd='+encodeURIComponent(c),{method:'POST',cache:'no-store'});const d=await r.json();o.textContent=d.ok?'Sent: '+d.command:'Error: '+d.error;setTimeout(refreshNow,350)}catch(e){o.textContent='Command failed: '+e}
}
refreshNow();setInterval(refreshNow,intervalMs);
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
      command == "TSYNC START" || command == "TSYNC STATUS" ||
      command == "TSYNC STOP" || command == "STOP" ||
      command == "AUTO" || command == "HELP";

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
  json.reserve(800);
  json += '{';

  json += "\"nanoConnected\":";
  json += _nano.connected() ? "true" : "false";

  json += ",\"statusAgeMs\":";
  if (s.receivedAtMs == 0) json += "null";
  else json += String(_nano.statusAgeMs());

  json += ",\"batteryVolts\":";
  if (s.valid) json += String(s.batteryVolts, 2);
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

  json += ",\"tempSyncActive\":";
  json += s.tempSyncActive ? "true" : "false";
  json += ",\"tempSyncPhase\":\"";
  json += jsonEscape(s.tempSyncPhase);
  json += '"';

  json += ",\"tempSyncDeltaValid\":";
  json += s.tempSyncDeltaValid ? "true" : "false";
  json += ",\"tempSyncDeltaC\":";
  if (s.tempSyncDeltaValid) json += String(s.tempSyncDeltaC, 2);
  else json += "null";

  json += ",\"tempSyncBaselineValid\":";
  json += s.tempSyncBaselineValid ? "true" : "false";
  json += ",\"tempSyncBaselineC\":";
  if (s.tempSyncBaselineValid) json += String(s.tempSyncBaselineC, 2);
  else json += "null";
  json += ",\"tempSyncBaselineSamples\":";
  json += String(s.tempSyncBaselineSamples);

  json += ",\"tempSyncChargeValid\":";
  json += s.tempSyncChargeValid ? "true" : "false";
  json += ",\"tempSyncChargeC\":";
  if (s.tempSyncChargeValid) json += String(s.tempSyncChargeC, 2);
  else json += "null";
  json += ",\"tempSyncChargeSamples\":";
  json += String(s.tempSyncChargeSamples);

  json += ",\"tempSyncFinalValid\":";
  json += s.tempSyncFinalValid ? "true" : "false";
  json += ",\"tempSyncFinalC\":";
  if (s.tempSyncFinalValid) json += String(s.tempSyncFinalC, 2);
  else json += "null";

  json += ",\"tempSyncReady\":";
  json += s.tempSyncReady ? "true" : "false";

  json += ",\"tempSyncNewOffsetValid\":";
  json += s.tempSyncNewOffsetValid ? "true" : "false";
  json += ",\"tempSyncNewOffsetC\":";
  if (s.tempSyncNewOffsetValid) json += String(s.tempSyncNewOffsetC, 2);
  else json += "null";

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
