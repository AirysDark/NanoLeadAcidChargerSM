#pragma once
#include <Arduino.h>

// ============================================================
// ONLY EDIT THIS FILE FOR NORMAL CONFIGURATION CHANGES
// ============================================================
// Unique filename on purpose: ArduinoDroid can put unrelated libraries ahead
// of the sketch folder in its include search path. A generic name such as
// Config.h can therefore resolve to another library's Config.h (for example
// NV3047), so this project uses ChargerMonitorConfig.h instead.

// -------------------------- Debug -----------------------------
constexpr bool ENABLE_DEBUG = true;
constexpr unsigned long DEBUG_BAUD = 115200UL;

// ----------------------- Nano UART link -----------------------
// ESP32-WROOM perspective (UART2):
// PIN_NANO_RX receives Nano TX (Nano D7) THROUGH a 5V -> 3.3V divider.
// PIN_NANO_TX sends to Nano RX (Nano D8) directly.
constexpr uint8_t PIN_NANO_RX = 16;  // GPIO16, UART2 RX
constexpr uint8_t PIN_NANO_TX = 17;  // GPIO17, UART2 TX
constexpr unsigned long NANO_BAUD = 9600UL;

constexpr unsigned long NANO_STATUS_POLL_MS = 1000UL;
constexpr unsigned long NANO_LINK_TIMEOUT_MS = 5000UL;

// Long enough for temperature-sync + voltage-calibration STATUS telemetry.
constexpr size_t NANO_LINE_BUFFER_SIZE = 512;

// ---------------- Nano web firmware programming --------------
// These are a SECOND hardware UART (UART1) used only while the ESP32-WROOM
// is programming the Nano's normal Arduino bootloader from an uploaded .bin file.
//
// ESP GPIO27 TX1 -> Nano D0 / RX directly
// Nano D1 / TX -> divider -> ESP GPIO26 RX1
// ESP GPIO25 -> 1k -> logic N-MOSFET gate
// MOSFET source -> GND, drain -> Nano RESET, gate -> 10k -> GND
//
// The MOSFET makes reset open-drain so the Nano's 5V RESET pull-up never gets
// connected directly to an ESP32 GPIO.
constexpr uint8_t PIN_NANO_PROG_RX = 26;       // GPIO26, UART1 RX; from Nano D1 via divider
constexpr uint8_t PIN_NANO_PROG_TX = 27;       // GPIO27, UART1 TX; to Nano D0 directly
constexpr uint8_t PIN_NANO_RESET_GATE = 25;   // GPIO25; drives reset MOSFET gate

// Classic Nano old bootloader normally uses 57600. New/Optiboot Nano normally
// uses 115200. The updater automatically tries both.
constexpr unsigned long NANO_BOOT_BAUD_PRIMARY = 57600UL;
constexpr unsigned long NANO_BOOT_BAUD_SECONDARY = 115200UL;
constexpr uint16_t NANO_FLASH_PAGE_SIZE = 128;
constexpr size_t NANO_MAX_FIRMWARE_BYTES = 30720;
constexpr char NANO_UPDATE_FILE[] = "/nano-update.bin";

// --------------------------- Wi-Fi ----------------------------
// Hotspot only. The ESP32-WROOM does NOT connect to any router.
constexpr char HOTSPOT_SSID[] = "NanoCharger";
constexpr char HOTSPOT_PASSWORD[] = "charger123";

// ------------------------- Web server -------------------------
constexpr uint16_t WEB_SERVER_PORT = 80;
constexpr unsigned long WEB_REFRESH_MS = 1000UL;
