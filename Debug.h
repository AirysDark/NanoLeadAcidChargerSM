#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>

namespace Debug {
  void begin();
  void print(const __FlashStringHelper* message);
  void println(const __FlashStringHelper* message);
  void println(const String& message);
  void wifiInfo(const char* label, const IPAddress& ip);
}
