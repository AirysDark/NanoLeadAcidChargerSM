# NanoLeadAcidChargerSM

ESP32-WROOM hotspot serial monitor, calibration dashboard, and browser firmware updater for `AirysDark/NanoLeadAcidCharger`.

The ESP32-WROOM talks to the Arduino Nano over a dedicated 9600-baud UART link, polls `STATUS` once per second, and serves a live browser dashboard.

## ArduinoDroid compatibility

This project intentionally uses `ChargerMonitorConfig.h` instead of a generic `Config.h` filename. ArduinoDroid can put installed library folders ahead of the sketch folder in its include search path, so a generic `Config.h` can resolve to an unrelated installed library.

## Wi-Fi

The ESP32-WROOM runs as its own hotspot only. It does not connect to a router.

- SSID: `NanoCharger`
- Password: `charger123`
- Address: `http://192.168.4.1/`

Connect a phone, tablet or computer directly to `NanoCharger`, then open `192.168.4.1`.

## Normal charger UART wiring

```text
Nano D7 TX ---- 5V-to-3.3V divider ---- ESP32 GPIO16 RX2
Nano D8 RX <---------------------------- ESP32 GPIO17 TX2
Nano GND ------------------------------- ESP32 GND
```

For the custom USB-C UART cable:

```text
D+  = Nano D7 TX -> divider -> ESP32 GPIO16 RX2
D-  = ESP32 GPIO17 TX2 -> Nano D8 RX
GND = common ground
VBUS = optional / leave unused when the ESP has separate power
```

This is custom UART signalling, not normal USB signalling.

## Browser firmware updates

The main dashboard now has a `FIRMWARE UPDATE` button. It opens `/firmware` and accepts `.bin` files for both devices.

### ESP32-WROOM update

Upload `NanoLeadAcidChargerSM.bin`. The ESP32-WROOM uses its OTA flash area, writes the new image, and restarts automatically. No extra programming wiring is required.

The GitHub Actions workflow in `.github/workflows/build-firmware.yml` builds this file automatically and publishes it as the `NanoLeadAcidChargerSM-firmware` artifact.

### Arduino Nano update

Upload `NanoLeadAcidCharger.bin`. Before programming, the ESP32-WROOM sends `STOP` over the normal charger UART. It then resets the Nano into its standard Arduino bootloader, writes the application in 128-byte pages, verifies every page, and restarts the Nano.

The normal D7/D8 charger UART remains connected. Nano firmware flashing needs three extra programming connections:

```text
ESP32 GPIO27 TX1 -> Nano D0 / RX directly
Nano D1 / TX -> 5V-to-3.3V divider -> ESP32 GPIO26 RX1
ESP32 GPIO25 -> 1k -> logic N-MOSFET gate
MOSFET source -> GND
MOSFET drain  -> Nano RESET
MOSFET gate   -> 10k -> GND
ESP GND <-> Nano GND
```

The reset MOSFET is important: the Nano RESET line is pulled up to 5 V, so it must not be connected directly to an ESP32 GPIO. The ESP32 only drives the MOSFET gate.

The updater automatically tries classic Nano bootloader speed `57600` and Optiboot/new Nano speed `115200`. Maximum accepted Nano application binary size is 30720 bytes.

`AirysDark/NanoLeadAcidCharger` now has its own GitHub Actions workflow that builds `NanoLeadAcidCharger.bin` specifically for this web updater.

The ESP32-WROOM uses LittleFS to temporarily store the Nano `.bin` before programming. Use an ESP32 partition layout that includes filesystem space.

## Internal temperature calibration

Put the external temperature sensor beside the Nano inside the charger enclosure, then press `START TEMP SYNC`.

The dashboard guides a three-point automatic calibration:

1. **Point 1** — the Nano holds charging OFF and averages 60 paired readings.
2. The Nano returns to AUTO and waits for real `CHARGER=ON`, `STATE=CHARGING`, and roughly a 2 C rise in the external reference temperature.
3. **Point 2** — it averages another 60 paired readings while charging.
4. It waits for another temperature rise while charging.
5. **Point 3** — it averages the final 60 paired readings.
6. The Nano fits its internal raw ADC count against the external reference temperature and stops automatically.

The page displays all three points as external temperature plus averaged raw ADC value and gives the exact replacement lines for `NanoLeadAcidCharger/PinsAndConfig.h`:

```cpp
constexpr float INTERNAL_TEMP_CAL_RAW = ...f;
constexpr float INTERNAL_TEMP_CAL_C = ...f;
constexpr float INTERNAL_TEMP_COUNTS_PER_C = ...f;
constexpr float INTERNAL_TEMP_CALIBRATION_OFFSET_C = 0.0f;
```

Reflash the Nano with those values, then move the external sensor back to the battery.

The test never forces charging ON or bypasses the Nano safety logic.

## Battery voltage divider calibration

Press `START VOLTAGE CAL` and use a multimeter directly across the battery terminals.

1. Enter reading 1, for example `12.07`.
2. The Nano stores its own divider voltage at the exact moment you submit it.
3. The dashboard waits until the Nano reading rises enough and then asks for reading 2.
4. Enter reading 2.
5. It waits for another rise and asks for reading 3.
6. Enter reading 3.
7. The Nano performs a three-point linear calibration and stops automatically.

The page then gives the exact two replacement lines for `NanoLeadAcidCharger/PinsAndConfig.h`:

```cpp
constexpr float BATTERY_VOLTAGE_CALIBRATION = ...f;
constexpr float BATTERY_VOLTAGE_OFFSET_VOLTS = ...f;
```

Reflash the Nano and verify its voltage against the multimeter again.

Voltage calibration never forces charging ON or bypasses charger safety logic.

## Dashboard

The live page shows battery voltage, external battery/reference temperature, Nano/internal temperature, charger ON/OFF state, charger control state and mode, three-point temperature-calibration progress/result, three-point voltage-calibration progress/result, Nano UART link state, hotspot address, and the last raw Nano response.

Main controls are `START TEMP SYNC`, `CANCEL TEMP SYNC`, `START VOLTAGE CAL`, `CANCEL VOLTAGE CAL`, `STOP CHARGING`, `AUTO`, `REFRESH`, and `FIRMWARE UPDATE`.

The page refreshes live data without reloading the whole page.

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

There is deliberately no remote force-ON command.

## Files

- `NanoLeadAcidChargerSM.ino` - main sketch
- `ChargerMonitorConfig.h` - ESP32-WROOM UART pins, updater pins, hotspot and web configuration
- `NanoLink.h/.cpp` - normal Nano UART link and telemetry parser
- `NanoFirmwareUpdater.h/.cpp` - Nano bootloader `.bin` storage/program/verify logic
- `FirmwareUpdate.h/.cpp` - `/firmware` page, ESP32-WROOM OTA upload and Nano upload handling
- `ChargerNetwork.h/.cpp` - hotspot-only Wi-Fi
- `WebUi.h/.cpp` - live dashboard, calibration controls and JSON API
- `Debug.h/.cpp` - USB Serial debugging


## ESP32-WROOM board selection

Use **ESP32 Dev Module** for a standard ESP32-WROOM-32 development board. The GitHub Actions build uses `esp32:esp32:esp32`.

The firmware uses the ESP32 hardware UARTs instead of SoftwareSerial:

```text
UART2 normal Nano link:
GPIO16 RX2 <- Nano D7 TX through 5V-to-3.3V divider
GPIO17 TX2 -> Nano D8 RX

UART1 Nano bootloader updater:
GPIO26 RX1 <- Nano D1 TX through 5V-to-3.3V divider
GPIO27 TX1 -> Nano D0 RX
GPIO25     -> 1k -> reset MOSFET gate
```
