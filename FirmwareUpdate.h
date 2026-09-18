#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <Update.h>
#include "NanoFirmwareUpdater.h"

class NanoLink;

class FirmwareUpdate {
public:
  explicit FirmwareUpdate(NanoLink& nanoLink);

  void begin(WebServer& server);
  void update();

private:
  WebServer* _server;
  NanoFirmwareUpdater _nanoUpdater;

  bool _espUploadStarted;
  bool _espUploadOk;
  String _espUploadError;

  bool _nanoUploadStarted;
  bool _nanoUploadOk;
  String _nanoUploadError;

  bool _restartPending;
  unsigned long _restartAtMs;

  void handlePage();
  void handleEspUpload();
  void handleEspFinished();
  void handleNanoUpload();
  void handleNanoFinished();

  void sendResult(bool ok, const String& message);
  static String jsonEscape(const String& input);
};
