#include "ChargerMonitorConfig.h"
#include "Debug.h"
#include "NanoLink.h"
#include "NetworkManager.h"
#include "WebUi.h"

NanoLink nanoLink;
NetworkManager network;
WebUi webUi(nanoLink, network);

void setup() {
  Debug::begin();
  nanoLink.begin();
  network.begin();
  webUi.begin();
}

void loop() {
  nanoLink.update();
  network.update();
  webUi.update();
  yield();
}
