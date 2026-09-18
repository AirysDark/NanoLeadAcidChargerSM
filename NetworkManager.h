#pragma once
#include <Arduino.h>
#include <WiFi.h>

class NetworkManager {
public:
  NetworkManager();

  void begin();
  void update();

  IPAddress hotspotIP() const;

private:
  void startHotspot();
};
