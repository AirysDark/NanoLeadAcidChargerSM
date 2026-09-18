#include "ChargerNetwork.h"
#include "ChargerMonitorConfig.h"
#include "Debug.h"
#include <cstring>

ChargerNetwork::ChargerNetwork() {
}

void ChargerNetwork::begin() {
  WiFi.mode(WIFI_AP);
  startHotspot();
}

void ChargerNetwork::startHotspot() {
  bool ok = false;

  if (strlen(HOTSPOT_PASSWORD) >= 8) {
    ok = WiFi.softAP(HOTSPOT_SSID, HOTSPOT_PASSWORD);
  } else {
    ok = WiFi.softAP(HOTSPOT_SSID);
  }

  if (ENABLE_DEBUG) {
    Serial.print(F("Hotspot: "));
    Serial.println(ok ? F("STARTED") : F("FAILED"));
    Serial.print(F("Hotspot SSID: "));
    Serial.println(HOTSPOT_SSID);
    Serial.print(F("Hotspot IP: "));
    Serial.println(WiFi.softAPIP());
  }
}

void ChargerNetwork::update() {
  // Hotspot-only mode requires no reconnect logic.
}

IPAddress ChargerNetwork::hotspotIP() const {
  return WiFi.softAPIP();
}
