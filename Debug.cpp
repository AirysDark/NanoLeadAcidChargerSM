#include "Debug.h"
#include "ChargerMonitorConfig.h"

namespace Debug {

void begin() {
  if (!ENABLE_DEBUG) return;
  Serial.begin(DEBUG_BAUD);
  Serial.println();
  Serial.println(F("NanoLeadAcidChargerSM ESP8266 starting"));
}

void print(const __FlashStringHelper* message) {
  if (ENABLE_DEBUG) Serial.print(message);
}

void println(const __FlashStringHelper* message) {
  if (ENABLE_DEBUG) Serial.println(message);
}

void println(const String& message) {
  if (ENABLE_DEBUG) Serial.println(message);
}

void wifiInfo(const char* label, const IPAddress& ip) {
  if (!ENABLE_DEBUG) return;
  Serial.print(label);
  Serial.println(ip);
}

}  // namespace Debug
