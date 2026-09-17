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

## Temperature sync calibration

Put the external temperature sensor beside the Nano inside the charger enclosure, then press `START TEMP SYNC` on the web page.

The Nano runs the calibration automatically:

1. It temporarily inhibits charging and collects 60 paired external/Nano temperature samples.
2. When the baseline is complete it returns the charger to AUTO and the web page asks you to connect/use a battery that needs charging.
3. It waits until the Nano actually reports `CHARGER=ON` and `STATE=CHARGING`.
4. It collects another 60 paired samples while the charger is really charging. If charging stops, sampling pauses until charging resumes.
5. The test stops automatically after both stages are complete.
6. The web page displays the exact line to put into `NanoLeadAcidCharger/PinsAndConfig.h`, for example:

```cpp
constexpr float INTERNAL_TEMP_CALIBRATION_OFFSET_C = -4.25f;
```

Reflash the Nano after changing that value, then move the external temperature sensor back to the battery.

The test never forces the charger ON. Existing Nano voltage and temperature safety logic remains in control.

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

This is not USB signalling. Label the ports `UART LINK - NOT USB` and do not plug the custom data wiring into a normal PC/phone USB port.

## Web controls

The dashboard exposes live battery voltage, external temperature, Nano temperature, charger state, temperature-sync progress, Nano UART state, hotspot address, and the last raw Nano line.

Main controls:

- `START TEMP SYNC` starts the automatic 60 OFF + 60 CHARGING calibration.
- `CANCEL TEMP SYNC` cancels a running calibration.
- `STOP CHARGING` sends `STOP`.
- `AUTO` returns control to the Nano's automatic charger logic.

There is intentionally no remote force-ON command.

## Nano commands used

```text
PING
STATUS
BATTERY
TEMP
STATE
TSYNC START
TSYNC STATUS
TSYNC STOP
STOP
AUTO
HELP
```

## Files

- `NanoLeadAcidChargerSM.ino` - main sketch
- `Config.h` - pins, hotspot and timing
- `NanoLink.h/.cpp` - Nano UART link and STATUS parser
- `NetworkManager.h/.cpp` - hotspot-only Wi-Fi
- `WebUi.h/.cpp` - live web page and JSON API
- `Debug.h/.cpp` - USB Serial debugging
