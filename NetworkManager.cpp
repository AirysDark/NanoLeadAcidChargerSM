#include "NetworkManager.h"
#include "Config.h"
#include "Debug.h"
#include <ESP8266mDNS.h>
#include <cstring>

NetworkManager::NetworkManager()
  : _lastRouterAttemptMs(0),
    _mdnsStarted(false) {
}

void NetworkManager::begin() {
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_AP_STA);
  WiFi.hostname(WIFI_HOSTNAME);

  startHotspot();

  if (ENABLE_ROUTER_CONNECTION && strlen(ROUTER_SSID) > 0) {
    startRouterConnection();
  } else {
    Debug::println(F("Router connection disabled/not configured; hotspot remains active"));
  }
}

void NetworkManager::startHotspot() {
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

void NetworkManager::startRouterConnection() {
  _lastRouterAttemptMs = millis();

  if (ENABLE_DEBUG) {
    Serial.print(F("Connecting to router: "));
    Serial.println(ROUTER_SSID);
  }

  WiFi.begin(ROUTER_SSID, ROUTER_PASSWORD);
}

void NetworkManager::update() {
  if (ENABLE_ROUTER_CONNECTION && strlen(ROUTER_SSID) > 0) {
    if (WiFi.status() != WL_CONNECTED) {
      if ((millis() - _lastRouterAttemptMs) >= ROUTER_RETRY_MS) {
        startRouterConnection();
      }
    }
  }

  updateMdns();
}

void NetworkManager::updateMdns() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!_mdnsStarted) {
      _mdnsStarted = MDNS.begin(WIFI_HOSTNAME);
      if (_mdnsStarted) {
        MDNS.addService("http", "tcp", WEB_SERVER_PORT);
        if (ENABLE_DEBUG) {
          Serial.print(F("Router connected. IP: "));
          Serial.println(WiFi.localIP());
          Serial.print(F("mDNS: http://"));
          Serial.print(WIFI_HOSTNAME);
          Serial.println(F(".local/"));
        }
      }
    }
  }

  if (_mdnsStarted) {
    MDNS.update();
  }
}

bool NetworkManager::stationConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

IPAddress NetworkManager::stationIP() const {
  return WiFi.localIP();
}

IPAddress NetworkManager::hotspotIP() const {
  return WiFi.softAPIP();
}

String NetworkManager::stationSSID() const {
  return stationConnected() ? WiFi.SSID() : String();
}
