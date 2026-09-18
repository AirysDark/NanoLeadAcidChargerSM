#include "FirmwareUpdate.h"
#include "ChargerMonitorConfig.h"
#include "Debug.h"
#include <Update.h>

namespace {
const char FIRMWARE_PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Firmware Update</title>
<style>
:root{color-scheme:dark;background:#101317;color:#eef2f5;font-family:Arial,sans-serif}
body{margin:0;padding:18px;max-width:760px;margin-inline:auto}
h1{font-size:1.5rem;margin:0 0 6px}.sub{color:#9aa7b2;margin-bottom:18px}
.card{background:#191e24;border:1px solid #303841;border-radius:12px;padding:14px;margin:12px 0}
.label{font-size:.78rem;color:#97a6b2;text-transform:uppercase;letter-spacing:.06em}
button,.back{display:inline-block;border:0;border-radius:10px;padding:12px 18px;margin:8px 4px 4px 0;font-size:1rem;font-weight:700;cursor:pointer;text-decoration:none}
button{background:#4fc36a;color:#07120a}button:disabled{opacity:.45;cursor:default}.back{background:#39434d;color:#fff}
input[type=file]{display:block;width:100%;box-sizing:border-box;border:1px solid #46515d;border-radius:9px;background:#0f1419;color:#fff;padding:11px;margin:10px 0}
.note{font-size:.9rem;line-height:1.45;color:#b6c0c8}.warn{color:#ffd36a}.ok{color:#71dc8c}.bad{color:#ff7878}
pre{background:#0a0d10;border:1px solid #343d46;border-radius:8px;padding:10px;white-space:pre-wrap;word-break:break-word;color:#aeb9c2}
#result{margin:14px 0;font-weight:700;word-break:break-word}
</style>
</head>
<body>
<h1>Firmware Update</h1>
<div class="sub">Nano Lead-Acid Charger hotspot updater</div>
<a class="back" href="/">BACK TO CHARGER</a>

<div class="card">
  <div class="label">ESP32-WROOM monitor firmware</div>
  <p class="note">Upload the <b>NanoLeadAcidChargerSM .bin</b>. The ESP32-WROOM writes the new firmware to its OTA slot and restarts automatically. No extra programming wires are needed.</p>
  <input id="espFile" type="file" accept=".bin,application/octet-stream">
  <button id="espButton" onclick="uploadFirmware('esp')">FLASH ESP32-WROOM .BIN</button>
</div>

<div class="card">
  <div class="label">Arduino Nano charger firmware</div>
  <p class="note">Upload the <b>NanoLeadAcidCharger .bin</b>. The ESP32-WROOM first sends STOP to the charger, resets the Nano into its Arduino bootloader, writes the application, verifies every page, then restarts the Nano.</p>
  <p class="note warn">The Nano web update needs the separate bootloader programming wiring below. The normal D7/D8 charger UART stays connected.</p>
  <pre>ESP32 GPIO27 TX1 -> Nano D0 / RX
Nano D1 / TX -> 5V-to-3.3V divider -> ESP32 GPIO26 RX1
ESP32 GPIO25 -> 1k -> logic N-MOSFET gate
MOSFET source -> GND
MOSFET drain  -> Nano RESET
MOSFET gate   -> 10k -> GND
ESP GND <-> Nano GND</pre>
  <p class="note">Maximum Nano application binary: 30720 bytes. The updater automatically tries both 57600 and 115200 baud Nano bootloaders.</p>
  <input id="nanoFile" type="file" accept=".bin,application/octet-stream">
  <button id="nanoButton" onclick="uploadFirmware('nano')">FLASH NANO .BIN</button>
</div>

<div id="result"></div>
<p class="note">Use only firmware built for the exact target board. Keep power connected for the entire update.</p>

<script>
let busy=false;
function setResult(text,cls=''){const e=document.getElementById('result');e.textContent=text;e.className=cls}
function setBusy(v){busy=v;document.getElementById('espButton').disabled=v;document.getElementById('nanoButton').disabled=v}
async function uploadFirmware(kind){
 if(busy)return;
 const input=document.getElementById(kind==='esp'?'espFile':'nanoFile');
 if(!input.files||input.files.length===0){setResult('Choose a .bin file first.','bad');return}
 const file=input.files[0];
 if(!file.name.toLowerCase().endsWith('.bin')){setResult('Firmware file must end in .bin.','bad');return}
 if(kind==='nano'&&file.size>30720){setResult('Nano .bin is too large. Maximum is 30720 bytes.','bad');return}
 if(file.size===0){setResult('Firmware file is empty.','bad');return}
 const data=new FormData();data.append('firmware',file,file.name);
 setBusy(true);setResult(kind==='esp'?'Uploading ESP32-WROOM firmware...':'Uploading and programming Nano firmware...','warn');
 try{
  const response=await fetch('/firmware/'+kind,{method:'POST',body:data,cache:'no-store'});
  const result=await response.json();
  if(!result.ok){setResult('Update failed: '+result.message,'bad');setBusy(false);return}
  setResult(result.message,'ok');
  if(kind==='esp'){
    setResult(result.message+' Rebooting ESP32-WROOM; reconnecting to the hotspot page in a few seconds...','ok');
    setTimeout(()=>{location.href='/'},6000);
  }else{
    setBusy(false);
  }
 }catch(e){
  setResult('Update connection error: '+String(e),'bad');setBusy(false);
 }
}
</script>
</body>
</html>
)HTML";
}

FirmwareUpdate::FirmwareUpdate(NanoLink& nanoLink)
  : _server(nullptr),
    _nanoUpdater(nanoLink),
    _espUploadStarted(false),
    _espUploadOk(false),
    _nanoUploadStarted(false),
    _nanoUploadOk(false),
    _restartPending(false),
    _restartAtMs(0) {
}

void FirmwareUpdate::begin(WebServer& server) {
  _server = &server;
  _nanoUpdater.begin();

  _server->on("/firmware", HTTP_GET, [this]() { handlePage(); });

  _server->on("/firmware/esp", HTTP_POST,
              [this]() { handleEspFinished(); },
              [this]() { handleEspUpload(); });

  _server->on("/firmware/nano", HTTP_POST,
              [this]() { handleNanoFinished(); },
              [this]() { handleNanoUpload(); });
}

void FirmwareUpdate::update() {
  if (_restartPending && static_cast<long>(millis() - _restartAtMs) >= 0) {
    delay(25);
    ESP.restart();
  }
}

void FirmwareUpdate::handlePage() {
  if (_server == nullptr) return;
  _server->sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  _server->send(200, "text/html", FPSTR(FIRMWARE_PAGE));
}

void FirmwareUpdate::handleEspUpload() {
  if (_server == nullptr) return;
  HTTPUpload& upload = _server->upload();

  if (upload.status == UPLOAD_FILE_START) {
    _espUploadStarted = true;
    _espUploadOk = false;
    _espUploadError = "";

    String name = upload.filename;
    name.toLowerCase();
    if (!name.endsWith(".bin")) {
      _espUploadError = F("ESP32-WROOM firmware must be a .bin file");
      return;
    }

    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      _espUploadError = F("ESP32-WROOM OTA partition is not available or does not have enough space");
      return;
    }

    _espUploadOk = true;
    if (ENABLE_DEBUG) {
      Serial.print(F("[ESP UPDATE] start "));
      Serial.println(upload.filename);
    }
    return;
  }

  if (upload.status == UPLOAD_FILE_WRITE) {
    if (!_espUploadOk) return;
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      _espUploadOk = false;
      _espUploadError = F("ESP32-WROOM flash write failed");
    }
    yield();
    return;
  }

  if (upload.status == UPLOAD_FILE_END) {
    if (_espUploadOk) {
      if (!Update.end(true)) {
        _espUploadOk = false;
        _espUploadError = F("ESP32-WROOM firmware image was rejected or incomplete");
      }
    } else {
      (void)Update.end(false);
    }
    return;
  }

  if (upload.status == UPLOAD_FILE_ABORTED) {
    _espUploadOk = false;
    _espUploadError = F("ESP32-WROOM firmware upload was aborted");
    (void)Update.end(false);
  }
}

void FirmwareUpdate::handleEspFinished() {
  if (_server == nullptr) return;

  if (!_espUploadStarted) {
    sendResult(false, F("No ESP32-WROOM firmware file was uploaded"));
    return;
  }

  if (!_espUploadOk) {
    sendResult(false, _espUploadError.length() ? _espUploadError : String(F("ESP32-WROOM update failed")));
    _espUploadStarted = false;
    return;
  }

  sendResult(true, F("ESP32-WROOM firmware written successfully."));
  _espUploadStarted = false;
  _restartPending = true;
  _restartAtMs = millis() + 750UL;
}

void FirmwareUpdate::handleNanoUpload() {
  if (_server == nullptr) return;
  HTTPUpload& upload = _server->upload();

  if (upload.status == UPLOAD_FILE_START) {
    _nanoUploadStarted = true;
    _nanoUploadError = "";
    _nanoUploadOk = _nanoUpdater.beginUpload(upload.filename);
    if (!_nanoUploadOk) _nanoUploadError = _nanoUpdater.lastError();
    return;
  }

  if (upload.status == UPLOAD_FILE_WRITE) {
    if (!_nanoUploadOk) return;
    _nanoUploadOk = _nanoUpdater.writeUpload(upload.buf, upload.currentSize);
    if (!_nanoUploadOk) _nanoUploadError = _nanoUpdater.lastError();
    yield();
    return;
  }

  if (upload.status == UPLOAD_FILE_END) {
    if (_nanoUploadOk) {
      _nanoUploadOk = _nanoUpdater.endUpload();
      if (!_nanoUploadOk) _nanoUploadError = _nanoUpdater.lastError();
    }
    return;
  }

  if (upload.status == UPLOAD_FILE_ABORTED) {
    _nanoUpdater.abortUpload();
    _nanoUploadOk = false;
    _nanoUploadError = F("Nano firmware upload was aborted");
  }
}

void FirmwareUpdate::handleNanoFinished() {
  if (_server == nullptr) return;

  if (!_nanoUploadStarted) {
    sendResult(false, F("No Nano firmware file was uploaded"));
    return;
  }

  if (!_nanoUploadOk) {
    sendResult(false, _nanoUploadError.length() ? _nanoUploadError : String(F("Nano upload failed")));
    _nanoUploadStarted = false;
    return;
  }

  String result;
  _nanoUploadOk = _nanoUpdater.flashUploadedFirmware(result);
  if (!_nanoUploadOk) {
    sendResult(false, result.length() ? result : _nanoUpdater.lastError());
  } else {
    sendResult(true, result);
  }

  _nanoUploadStarted = false;
}

void FirmwareUpdate::sendResult(bool ok, const String& message) {
  if (_server == nullptr) return;
  String json;
  json.reserve(message.length() + 48);
  json += F("{\"ok\":");
  json += ok ? F("true") : F("false");
  json += F(",\"message\":\"");
  json += jsonEscape(message);
  json += F("\"}");
  _server->sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  _server->send(ok ? 200 : 400, "application/json", json);
}

String FirmwareUpdate::jsonEscape(const String& input) {
  String out;
  out.reserve(input.length() + 8);
  for (size_t i = 0; i < input.length(); ++i) {
    const char c = input.charAt(i);
    switch (c) {
      case '\\': out += F("\\\\"); break;
      case '"': out += F("\\\""); break;
      case '\n': out += F("\\n"); break;
      case '\r': out += F("\\r"); break;
      case '\t': out += F("\\t"); break;
      default:
        if (static_cast<uint8_t>(c) >= 0x20U) out += c;
        break;
    }
  }
  return out;
}
