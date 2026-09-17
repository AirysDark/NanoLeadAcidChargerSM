# NanoLeadAcidChargerSM

ESP8266 Wi-Fi serial monitor/web interface for `AirysDark/NanoLeadAcidCharger`.

The ESP8266 talks to the Arduino Nano over a dedicated 9600-baud UART link, polls `STATUS` once per second, and serves a live browser dashboard.

## Wi-Fi

The ESP8266 runs as its own hotspot only. It does not connect to a router.

- SSID: `NanoCharger`
- Password: `charger123`
- Address: `http://192.168.4.1/`

## Temperature sync calibration

Put the external temperature sensor beside the Nano inside the charger enclosure, then press `START TEMP SYNC`.

1. The Nano holds charging OFF and collects 60 paired temperature samples.
2. It returns to AUTO and waits until it actually reports `CHARGER=ON` and `STATE=CHARGING`.
3. It collects another 60 paired samples while charging.
4. The test completes automatically and shows the exact `INTERNAL_TEMP_CALIBRATION_OFFSET_C` line to copy into `NanoLeadAcidCharger/PinsAndConfig.h` before reflashing.

The test never forces the charger ON.

## Battery voltage divider calibration

Press `START VOLTAGE CAL` and use a multimeter directly across the battery terminals.

1. The page asks for reading 1. Enter the actual multimeter voltage, for example `12.07`.
2. The Nano saves its own divider reading at that exact moment.
3. It waits until its measured battery voltage rises by at least the configured step, then asks for reading 2.
4. Enter the second multimeter voltage.
5. It waits for another voltage rise and then asks for reading 3.
6. After reading 3 it performs a three-point linear calibration and automatically shows the exact two lines to replace in `NanoLeadAcidCharger/PinsAndConfig.h`:

```cpp
constexpr float BATTERY_VOLTAGE_CALIBRATION = 1.000000f;
constexpr float BATTERY_VOLTAGE_OFFSET_VOLTS = 0.0000f;
```

The numbers shown by the webpage will be the measured result, not the example values above. Reflash the Nano after replacing both values, then verify the Nano voltage against the multimeter.

The voltage calibration does not force charging ON or bypass any charger safety logic.

## UART wiring

```text
Nano D9 TX ---- 5V-to-3.3V divider ---- ESP GPIO14/D5 RX
Nano D8 RX <---------------------------- ESP GPIO12/D6 TX
Nano GND ------------------------------- ESP GND
```

For the custom USB-C UART cable:

```text
D+  = Nano TX -> divider -> ESP RX
D-  = ESP TX -> Nano RX
GND = common ground
VBUS = optional / unused if ESP has its own power source
```

This is not normal USB signalling.

## Nano commands

```text
PING
STATUS
BATTERY
TEMP
STATE
TSYNC START
TSYNC STATUS
TSYNC STOP
VCAL START
VCAL SAMPLE 12.07
VCAL STATUS
VCAL STOP
STOP
AUTO
HELP
```

## Files

- `NanoLeadAcidChargerSM.ino` - main sketch
- `Config.h` - pins, hotspot and timing
- `NanoLink.h/.cpp` - Nano UART link and telemetry parser
- `NetworkManager.h/.cpp` - hotspot-only Wi-Fi
- `WebUi.h/.cpp` - dashboard, calibration controls and JSON API
- `Debug.h/.cpp` - USB Serial debugging
