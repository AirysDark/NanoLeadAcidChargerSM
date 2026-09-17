#include "NanoLink.h"
#include "Debug.h"
#include <climits>

NanoLink::NanoLink()
  : _serial(PIN_NANO_RX, PIN_NANO_TX),
    _length(0),
    _lastPollMs(0) {
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
  readSerial();

  const unsigned long now = millis();
  if ((now - _lastPollMs) >= NANO_STATUS_POLL_MS) {
    _lastPollMs = now;
    sendCommand("STATUS");
  }
}

void NanoLink::sendCommand(const String& command) {
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

bool NanoLink::connected() const {
  if (!_status.valid || _status.receivedAtMs == 0) return false;
  return (millis() - _status.receivedAtMs) <= NANO_LINK_TIMEOUT_MS;
}

unsigned long NanoLink::statusAgeMs() const {
  if (_status.receivedAtMs == 0) return ULONG_MAX;
  return millis() - _status.receivedAtMs;
}

const ChargerStatus& NanoLink::status() const {
  return _status;
}

const String& NanoLink::lastLine() const {
  return _lastLine;
}

void NanoLink::readSerial() {
  while (_serial.available() > 0) {
    handleChar(static_cast<char>(_serial.read()));
  }
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

  if (_lastLine.startsWith("STATUS ")) {
    parseStatus(_lastLine);
  }
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
  const String charger = valueForKey(line, "CHARGER");
  const String state = valueForKey(line, "STATE");
  const String mode = valueForKey(line, "MODE");

  if (bat.length() == 0 || charger.length() == 0 || state.length() == 0) return;

  _status.batteryVolts = bat.toFloat();

  _status.batteryTempValid = (btemp.length() > 0 && btemp != "INVALID");
  if (_status.batteryTempValid) _status.batteryTempC = btemp.toFloat();

  _status.nanoTempValid = (ntemp.length() > 0 && ntemp != "INVALID");
  if (_status.nanoTempValid) _status.nanoTempC = ntemp.toFloat();

  _status.chargerOn = (charger == "ON");
  _status.state = state;
  _status.mode = mode.length() ? mode : "UNKNOWN";

  const String tsync = valueForKey(line, "TSYNC");
  const String tphase = valueForKey(line, "TPHASE");
  const String tdelta = valueForKey(line, "TDELTA");
  const String tbase = valueForKey(line, "TBASE");
  const String tbsamp = valueForKey(line, "TBSAMP");
  const String tchg = valueForKey(line, "TCHG");
  const String tcsamp = valueForKey(line, "TCSAMP");
  const String tfinal = valueForKey(line, "TFINAL");
  const String tready = valueForKey(line, "TREADY");
  const String tnew = valueForKey(line, "TNEW");

  _status.tempSyncActive = (tsync == "ON");
  _status.tempSyncPhase = tphase.length() ? tphase : "OFF";

  _status.tempSyncDeltaValid = (tdelta.length() > 0 && tdelta != "INVALID");
  if (_status.tempSyncDeltaValid) _status.tempSyncDeltaC = tdelta.toFloat();

  _status.tempSyncBaselineValid = (tbase.length() > 0 && tbase != "INVALID");
  if (_status.tempSyncBaselineValid) _status.tempSyncBaselineC = tbase.toFloat();
  _status.tempSyncBaselineSamples =
      tbsamp.length() ? static_cast<uint16_t>(tbsamp.toInt()) : 0U;

  _status.tempSyncChargeValid = (tchg.length() > 0 && tchg != "INVALID");
  if (_status.tempSyncChargeValid) _status.tempSyncChargeC = tchg.toFloat();
  _status.tempSyncChargeSamples =
      tcsamp.length() ? static_cast<uint16_t>(tcsamp.toInt()) : 0U;

  _status.tempSyncFinalValid = (tfinal.length() > 0 && tfinal != "INVALID");
  if (_status.tempSyncFinalValid) _status.tempSyncFinalC = tfinal.toFloat();

  _status.tempSyncReady = (tready == "YES");

  _status.tempSyncNewOffsetValid = (tnew.length() > 0 && tnew != "INVALID");
  if (_status.tempSyncNewOffsetValid) _status.tempSyncNewOffsetC = tnew.toFloat();

  _status.receivedAtMs = millis();
  _status.valid = true;
}
