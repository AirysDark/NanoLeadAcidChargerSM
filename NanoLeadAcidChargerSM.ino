#include "ChargerMonitorConfig.h"
#include "Debug.h"
#include "NanoLink.h"
#include "ChargerNetwork.h"
#include "WebUi.h"

NanoLink nanoLink;
ChargerNetwork network;
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
