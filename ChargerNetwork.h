#pragma once
#include <Arduino.h>
#include <WiFi.h>

class ChargerNetwork {
public:
  ChargerNetwork();

  void begin();
  void update();

  IPAddress hotspotIP() const;

private:
  void startHotspot();
};
