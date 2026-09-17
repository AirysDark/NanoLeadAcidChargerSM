#pragma once
#include <Arduino.h>

// ============================================================
// ONLY EDIT THIS FILE FOR NORMAL CONFIGURATION CHANGES
// ============================================================

// -------------------------- Debug -----------------------------
constexpr bool ENABLE_DEBUG = true;
constexpr unsigned long DEBUG_BAUD = 115200UL;

// ----------------------- Nano UART link -----------------------
// ESP8266 perspective:
// PIN_NANO_RX receives Nano TX (Nano D9) THROUGH a 5V -> 3.3V divider.
// PIN_NANO_TX sends to Nano RX (Nano D8) directly.
// GPIO numbers are used so this works even if D-pin aliases differ by board.
constexpr uint8_t PIN_NANO_RX = 14;  // GPIO14, commonly D5
constexpr uint8_t PIN_NANO_TX = 12;  // GPIO12, commonly D6
constexpr unsigned long NANO_BAUD = 9600UL;

// Poll the Nano STATUS command this often.
constexpr unsigned long NANO_STATUS_POLL_MS = 1000UL;

// If no STATUS response is received for this long, report Nano link offline.
constexpr unsigned long NANO_LINK_TIMEOUT_MS = 5000UL;

// Maximum line received from the Nano.
constexpr size_t NANO_LINE_BUFFER_SIZE = 192;

// --------------------------- Wi-Fi ----------------------------
// Hotspot only. The ESP8266 does NOT connect to any router.
constexpr char HOTSPOT_SSID[] = "NanoCharger";

// Must be at least 8 characters for a protected ESP8266 soft-AP.
// Change this before permanent use if you want a different password.
constexpr char HOTSPOT_PASSWORD[] = "charger123";

// ------------------------- Web server -------------------------
constexpr uint16_t WEB_SERVER_PORT = 80;

// Browser refresh interval. The page itself is not reloaded; only live data
// is fetched, so values update without getting stuck on an old page.
constexpr unsigned long WEB_REFRESH_MS = 1000UL;
