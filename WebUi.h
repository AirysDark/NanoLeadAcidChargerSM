#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include "FirmwareUpdate.h"

class NanoLink;
class NetworkManager;

class WebUi {
public:
  WebUi(NanoLink& nanoLink, NetworkManager& network);

  void begin();
  void update();

private:
  NanoLink& _nano;
  NetworkManager& _network;
  WebServer _server;
  FirmwareUpdate _firmwareUpdate;

  void handleRoot();
  void handleStatus();
  void handleCommand();
  void handleNotFound();

  String buildStatusJson() const;
  static String jsonEscape(const String& input);
};
