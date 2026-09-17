# NanoLeadAcidChargerSM

ESP8266 hotspot serial monitor and calibration dashboard for `AirysDark/NanoLeadAcidCharger`.

The ESP8266 talks to the Arduino Nano over a dedicated 9600-baud UART link, polls `STATUS` once per second, and serves a live browser dashboard.

## Wi-Fi

The ESP8266 runs as its own hotspot only. It does not connect to a router.

- SSID: `NanoCharger`
- Password: `charger123`
- Address: `http://192.168.4.1/`

Connect a phone, tablet or computer directly to `NanoCharger`, then open `192.168.4.1`.

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
VBUS = optional / leave unused when the ESP has separate power
```

This is custom UART signalling, not normal USB signalling.

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

The live page shows:

- battery voltage
- external battery/reference temperature
- Nano/internal temperature
- charger ON/OFF state
- charger control state and mode
- three-point temperature-calibration progress and result
- three-point voltage-calibration progress and result
- Nano UART link state
- hotspot address
- last raw Nano response

Main controls:

- `START TEMP SYNC`
- `CANCEL TEMP SYNC`
- `START VOLTAGE CAL`
- `CANCEL VOLTAGE CAL`
- `STOP CHARGING`
- `AUTO`
- `REFRESH`

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
- `Config.h` - UART, hotspot and web timing configuration
- `NanoLink.h/.cpp` - Nano UART link and telemetry parser
- `NetworkManager.h/.cpp` - hotspot-only Wi-Fi
- `WebUi.h/.cpp` - live dashboard, calibration controls and JSON API
- `Debug.h/.cpp` - USB Serial debugging
