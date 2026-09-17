# NanoLeadAcidChargerSM

ESP8266 Wi-Fi serial monitor/web interface for `AirysDark/NanoLeadAcidCharger`.

The ESP8266 talks to the Arduino Nano over a dedicated 9600-baud UART link, polls `STATUS` once per second, and serves a live browser dashboard.

## Wi-Fi operation

The ESP8266 runs as its own Wi-Fi hotspot only. It does not connect to a router.

Default hotspot:

- SSID: `NanoCharger`
- Password: `charger123` — change this in `Config.h`
- Address: `http://192.168.4.1/`

Connect your phone, tablet, or computer directly to the `NanoCharger` Wi-Fi network, then open `192.168.4.1` in a browser.

The web page updates live with `/api/status`; the whole page does not need to reload.

## Nano UART wiring

Nano firmware defaults:

- Nano D8 = RX from ESP8266
- Nano D9 = TX to ESP8266
- 9600 baud

ESP8266 firmware defaults:

- GPIO14 / D5 = RX from Nano
- GPIO12 / D6 = TX to Nano

Wire:

```text
Nano D9 TX ---- 5V-to-3.3V divider ---- ESP GPIO14/D5 RX
Nano D8 RX <---------------------------- ESP GPIO12/D6 TX
Nano GND ------------------------------- ESP GND
```

Nano TX is 5V logic. Do not connect Nano TX directly to ESP8266 RX. The ESP8266 TX is 3.3V and can normally drive the Nano RX directly.

## Custom USB-C UART cable

If two USB-C breakout boards are being used only as a convenient cable connector:

```text
D+  = Nano TX -> divider -> ESP RX
D-  = ESP TX -> Nano RX
GND = common ground
VBUS = optional / leave disconnected if the ESP has its own power source
```

This is **not USB signalling**. Label the ports `UART LINK - NOT USB` and do not plug the custom data wiring into a normal PC/phone USB port.

## Web controls

The dashboard exposes:

- live battery voltage
- battery temperature
- Nano/charger internal temperature
- charger ON/OFF state
- charger state and AUTO/STOP mode
- Nano UART connection status
- hotspot IP address
- last raw line received from the Nano

Buttons:

- `STOP CHARGING` sends `STOP`
- `AUTO` returns control to the Nano's automatic charger logic

There is intentionally no remote force-ON command, so the ESP8266 cannot bypass the Nano's voltage or temperature safety logic.

## Nano commands used

```text
PING
STATUS
BATTERY
TEMP
STATE
STOP
AUTO
HELP
```

## Files

- `NanoLeadAcidChargerSM.ino` - main sketch
- `Config.h` - pins, hotspot and timing; normally the only file to edit
- `NanoLink.h/.cpp` - Nano UART link and STATUS parser
- `NetworkManager.h/.cpp` - hotspot-only Wi-Fi
- `WebUi.h/.cpp` - live web page and JSON API
- `Debug.h/.cpp` - USB Serial debugging
