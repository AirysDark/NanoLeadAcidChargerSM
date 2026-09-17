#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>

class NetworkManager {
public:
  NetworkManager();

  void begin();
  void update();

  bool stationConnected() const;
  IPAddress stationIP() const;
  IPAddress hotspotIP() const;
  String stationSSID() const;

private:
  unsigned long _lastRouterAttemptMs;
  bool _mdnsStarted;

  void startHotspot();
  void startRouterConnection();
  void updateMdns();
};
