#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include "FirmwareUpdate.h"

class NanoLink;
class ChargerNetwork;

class WebUi {
public:
  WebUi(NanoLink& nanoLink, ChargerNetwork& network);

  void begin();
  void update();

private:
  NanoLink& _nano;
  ChargerNetwork& _network;
  WebServer _server;
  FirmwareUpdate _firmwareUpdate;

  void handleRoot();
  void handleStatus();
  void handleCommand();
  void handleNotFound();

  String buildStatusJson() const;
  static String jsonEscape(const String& input);
};
