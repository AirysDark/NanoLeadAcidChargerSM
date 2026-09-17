#pragma once
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "Config.h"

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
