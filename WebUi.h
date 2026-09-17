#pragma once
#include <Arduino.h>
#include <ESP8266WebServer.h>

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
  ESP8266WebServer _server;

  void handleRoot();
  void handleStatus();
  void handleCommand();
  void handleNotFound();

  String buildStatusJson() const;
  static String jsonEscape(const String& input);
};
