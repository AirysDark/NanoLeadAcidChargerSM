#include "NanoLink.h"
#include "Debug.h"
#include <climits>

NanoLink::NanoLink()
  : _serial(PIN_NANO_RX, PIN_NANO_TX),
    _length(0),
    _lastPollMs(0),
    _paused(false) {
  _line[0] = '\0';
}

void NanoLink::begin() {
  _serial.begin(NANO_BAUD);
  _serial.listen();
  delay(50);
  sendCommand("PING");
  sendCommand("STATUS");
}

void NanoLink::update() {
  if (_paused) return;

  readSerial();
  const unsigned long now = millis();
  if ((now - _lastPollMs) >= NANO_STATUS_POLL_MS) {
    _lastPollMs = now;
    sendCommand("STATUS");
  }
}

void NanoLink::sendCommand(const String& command) {
  if (_paused) return;

  String cmd = command;
  cmd.trim();
  if (cmd.length() == 0) return;
  _serial.print(cmd);
  _serial.print('\n');
  if (ENABLE_DEBUG) {
    Serial.print(F("[NANO TX] "));
    Serial.println(cmd);
  }
}

void NanoLink::pause() {
  _paused = true;
}

void NanoLink::resume() {
  _serial.listen();
  _paused = false;
  _length = 0;
  _line[0] = '\0';
  _lastPollMs = millis();
  delay(20);
  sendCommand("PING");
  sendCommand("STATUS");
}

bool NanoLink::paused() const { return _paused; }

bool NanoLink::connected() const {
  if (_paused) return false;
  if (!_status.valid || _status.receivedAtMs == 0) return false;
  return (millis() - _status.receivedAtMs) <= NANO_LINK_TIMEOUT_MS;
}

unsigned long NanoLink::statusAgeMs() const {
  if (_status.receivedAtMs == 0) return ULONG_MAX;
  return millis() - _status.receivedAtMs;
}

const ChargerStatus& NanoLink::status() const { return _status; }
const String& NanoLink::lastLine() const { return _lastLine; }

void NanoLink::readSerial() {
  while (_serial.available() > 0) handleChar(static_cast<char>(_serial.read()));
}

void NanoLink::handleChar(char c) {
  if (c == '\r') return;
  if (c == '\n') {
    if (_length == 0) return;
    _line[_length] = '\0';
    handleLine();
    _length = 0;
    _line[0] = '\0';
    return;
  }

  if (_length < (NANO_LINE_BUFFER_SIZE - 1U)) {
    _line[_length++] = c;
    _line[_length] = '\0';
  } else {
    _length = 0;
    _line[0] = '\0';
  }
}

void NanoLink::handleLine() {
  _lastLine = String(_line);
  _lastLine.trim();

  if (ENABLE_DEBUG) {
    Serial.print(F("[NANO RX] "));
    Serial.println(_lastLine);
  }

  if (_lastLine.startsWith("STATUS ")) parseStatus(_lastLine);
}

String NanoLink::valueForKey(const String& line, const String& key) const {
  const String needle = key + "=";
  const int start = line.indexOf(needle);
  if (start < 0) return String();
  const int valueStart = start + needle.length();
  int end = line.indexOf(' ', valueStart);
  if (end < 0) end = line.length();
  return line.substring(valueStart, end);
}

void NanoLink::parseStatus(const String& line) {
  const String bat = valueForKey(line, "BAT");
  const String btemp = valueForKey(line, "BTEMP");
  const String ntemp = valueForKey(line, "NTEMP");
  const String nraw = valueForKey(line, "NRAW");
  const String charger = valueForKey(line, "CHARGER");
  const String state = valueForKey(line, "STATE");
  const String mode = valueForKey(line, "MODE");

  if (bat.length() == 0 || charger.length() == 0 || state.length() == 0) return;

  _status.batteryVolts = bat.toFloat();
  _status.batteryTempValid = (btemp.length() > 0 && btemp != "INVALID");
  if (_status.batteryTempValid) _status.batteryTempC = btemp.toFloat();
  _status.nanoTempValid = (ntemp.length() > 0 && ntemp != "INVALID");
  if (_status.nanoTempValid) _status.nanoTempC = ntemp.toFloat();
  if (nraw.length() > 0) _status.nanoTempRawAdc = static_cast<uint16_t>(nraw.toInt());
  _status.chargerOn = (charger == "ON");
  _status.state = state;
  _status.mode = mode.length() ? mode : "UNKNOWN";

  const String tsync = valueForKey(line, "TSYNC");
  const String tphase = valueForKey(line, "TPHASE");
  const String tdelta = valueForKey(line, "TDELTA");
  _status.tempSyncActive = (tsync == "ON");
  _status.tempSyncPhase = tphase.length() ? tphase : "OFF";
  _status.tempSyncDeltaValid = (tdelta.length() > 0 && tdelta != "INVALID");
  if (_status.tempSyncDeltaValid) _status.tempSyncDeltaC = tdelta.toFloat();

  for (uint8_t i = 0; i < 3; ++i) {
    const String prefix = String("T") + String(i + 1);
    const String ext = valueForKey(line, prefix + "EXT");
    const String raw = valueForKey(line, prefix + "RAW");
    const String samples = valueForKey(line, prefix + "S");

    _status.tempPointValid[i] = (ext.length() > 0 && ext != "INVALID");
    if (_status.tempPointValid[i]) _status.tempPointExternalC[i] = ext.toFloat();

    _status.tempPointRawValid[i] = (raw.length() > 0 && raw != "INVALID");
    if (_status.tempPointRawValid[i]) _status.tempPointRaw[i] = raw.toFloat();

    _status.tempPointSamples[i] = samples.length() ? static_cast<uint16_t>(samples.toInt()) : 0U;
  }

  const String tready = valueForKey(line, "TREADY");
  const String tcalraw = valueForKey(line, "TCALRAW");
  const String tcalc = valueForKey(line, "TCALC");
  const String tcounts = valueForKey(line, "TCOUNTS");

  _status.tempSyncReady = (tready == "YES");
  _status.tempCalRawValid = (tcalraw.length() > 0 && tcalraw != "INVALID");
  if (_status.tempCalRawValid) _status.tempCalRaw = tcalraw.toFloat();
  _status.tempCalCValid = (tcalc.length() > 0 && tcalc != "INVALID");
  if (_status.tempCalCValid) _status.tempCalC = tcalc.toFloat();
  _status.tempCountsPerCValid = (tcounts.length() > 0 && tcounts != "INVALID");
  if (_status.tempCountsPerCValid) _status.tempCountsPerC = tcounts.toFloat();

  const String vcal = valueForKey(line, "VCAL");
  const String vphase = valueForKey(line, "VPHASE");
  const String vsamp = valueForKey(line, "VSAMP");
  const String vtarget = valueForKey(line, "VTARGET");
  const String vready = valueForKey(line, "VREADY");
  const String vscale = valueForKey(line, "VSCALE");
  const String voff = valueForKey(line, "VOFF");

  _status.voltageCalActive = (vcal == "ON");
  _status.voltageCalPhase = vphase.length() ? vphase : "OFF";
  _status.voltageCalSamples = vsamp.length() ? static_cast<uint8_t>(vsamp.toInt()) : 0U;
  _status.voltageCalTargetValid = (vtarget.length() > 0 && vtarget != "INVALID");
  if (_status.voltageCalTargetValid) _status.voltageCalTargetV = vtarget.toFloat();
  _status.voltageCalReady = (vready == "YES");
  _status.voltageCalScaleValid = (vscale.length() > 0 && vscale != "INVALID");
  if (_status.voltageCalScaleValid) _status.voltageCalScale = vscale.toFloat();
  _status.voltageCalOffsetValid = (voff.length() > 0 && voff != "INVALID");
  if (_status.voltageCalOffsetValid) _status.voltageCalOffsetV = voff.toFloat();

  _status.receivedAtMs = millis();
  _status.valid = true;
}
