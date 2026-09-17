#pragma once
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "ChargerMonitorConfig.h"

struct ChargerStatus {
  bool valid = false;
  float batteryVolts = 0.0f;
  bool batteryTempValid = false;
  float batteryTempC = 0.0f;
  bool nanoTempValid = false;
  float nanoTempC = 0.0f;
  bool chargerOn = false;
  String state = "UNKNOWN";
  String mode = "UNKNOWN";

  // Three-point internal-temperature calibration telemetry.
  bool tempSyncActive = false;
  String tempSyncPhase = "OFF";
  bool tempSyncDeltaValid = false;
  float tempSyncDeltaC = 0.0f;

  bool tempPointValid[3] = {false, false, false};
  float tempPointExternalC[3] = {0.0f, 0.0f, 0.0f};
  bool tempPointRawValid[3] = {false, false, false};
  float tempPointRaw[3] = {0.0f, 0.0f, 0.0f};
  uint16_t tempPointSamples[3] = {0, 0, 0};

  bool tempSyncReady = false;
  bool tempCalRawValid = false;
  float tempCalRaw = 0.0f;
  bool tempCalCValid = false;
  float tempCalC = 0.0f;
  bool tempCountsPerCValid = false;
  float tempCountsPerC = 0.0f;

  // Three-point voltage-divider calibration telemetry.
  bool voltageCalActive = false;
  String voltageCalPhase = "OFF";
  uint8_t voltageCalSamples = 0;
  bool voltageCalTargetValid = false;
  float voltageCalTargetV = 0.0f;
  bool voltageCalReady = false;
  bool voltageCalScaleValid = false;
  float voltageCalScale = 0.0f;
  bool voltageCalOffsetValid = false;
  float voltageCalOffsetV = 0.0f;

  unsigned long receivedAtMs = 0;
};

class NanoLink {
public:
  NanoLink();
  void begin();
  void update();
  void sendCommand(const String& command);
  bool connected() const;
  unsigned long statusAgeMs() const;
  const ChargerStatus& status() const;
  const String& lastLine() const;

private:
  SoftwareSerial _serial;
  char _line[NANO_LINE_BUFFER_SIZE];
  size_t _length;
  unsigned long _lastPollMs;
  ChargerStatus _status;
  String _lastLine;

  void readSerial();
  void handleChar(char c);
  void handleLine();
  void parseStatus(const String& line);
  String valueForKey(const String& line, const String& key) const;
};
