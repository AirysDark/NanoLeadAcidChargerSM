#include "NanoFirmwareUpdater.h"
#include "ChargerMonitorConfig.h"
#include "NanoLink.h"
#include "Debug.h"

namespace {
constexpr uint8_t STK_OK = 0x10;
constexpr uint8_t STK_INSYNC = 0x14;
constexpr uint8_t CRC_EOP = 0x20;
constexpr uint8_t STK_GET_SYNC = 0x30;
constexpr uint8_t STK_ENTER_PROGMODE = 0x50;
constexpr uint8_t STK_LEAVE_PROGMODE = 0x51;
constexpr uint8_t STK_LOAD_ADDRESS = 0x55;
constexpr uint8_t STK_PROG_PAGE = 0x64;
constexpr uint8_t STK_READ_PAGE = 0x74;
constexpr uint8_t MEMTYPE_FLASH = 'F';
}

NanoFirmwareUpdater::NanoFirmwareUpdater(NanoLink& nanoLink)
  : _nano(nanoLink),
    _bootSerial(1),
    _storageReady(false),
    _uploadReady(false),
    _firmwareSize(0) {
}

void NanoFirmwareUpdater::begin() {
  pinMode(PIN_NANO_RESET_GATE, OUTPUT);
  digitalWrite(PIN_NANO_RESET_GATE, LOW);  // reset released

  _storageReady = LittleFS.begin(true);
  if (!_storageReady) {
    setError(F("LittleFS mount failed; use an ESP32 partition layout with filesystem space"));
  } else {
    LittleFS.remove(NANO_UPDATE_FILE);
    _lastError = "";
  }
}

bool NanoFirmwareUpdater::beginUpload(const String& filename) {
  _uploadReady = false;
  _firmwareSize = 0;
  _lastError = "";

  if (!_storageReady) {
    setError(F("ESP32 filesystem is not available"));
    return false;
  }

  String lowerName = filename;
  lowerName.toLowerCase();
  if (!lowerName.endsWith(".bin")) {
    setError(F("Nano firmware must be a .bin file"));
    return false;
  }

  if (_uploadFile) _uploadFile.close();
  LittleFS.remove(NANO_UPDATE_FILE);
  _uploadFile = LittleFS.open(NANO_UPDATE_FILE, "w");
  if (!_uploadFile) {
    setError(F("Could not create temporary Nano firmware file"));
    return false;
  }

  return true;
}

bool NanoFirmwareUpdater::writeUpload(const uint8_t* data, size_t length) {
  if (!_uploadFile) return false;

  if ((_firmwareSize + length) > NANO_MAX_FIRMWARE_BYTES) {
    setError(F("Nano .bin is larger than the safe 30720-byte application area"));
    abortUpload();
    return false;
  }

  const size_t written = _uploadFile.write(data, length);
  if (written != length) {
    setError(F("Failed while storing Nano firmware on ESP32"));
    abortUpload();
    return false;
  }

  _firmwareSize += written;
  return true;
}

bool NanoFirmwareUpdater::endUpload() {
  if (_uploadFile) _uploadFile.close();

  if (_lastError.length() != 0) {
    _uploadReady = false;
    return false;
  }

  if (_firmwareSize == 0 || _firmwareSize > NANO_MAX_FIRMWARE_BYTES) {
    setError(F("Nano firmware file is empty or too large"));
    cleanupUploadFile();
    return false;
  }

  _uploadReady = true;
  return true;
}

void NanoFirmwareUpdater::abortUpload() {
  if (_uploadFile) _uploadFile.close();
  _uploadReady = false;
  _firmwareSize = 0;
  LittleFS.remove(NANO_UPDATE_FILE);
}

bool NanoFirmwareUpdater::flashUploadedFirmware(String& resultMessage) {
  resultMessage = "";

  if (!_uploadReady || !_storageReady) {
    if (_lastError.length() == 0) setError(F("No complete Nano firmware upload is ready"));
    resultMessage = _lastError;
    return false;
  }

  File firmware = LittleFS.open(NANO_UPDATE_FILE, "r");
  if (!firmware) {
    setError(F("Could not reopen stored Nano firmware"));
    resultMessage = _lastError;
    return false;
  }

  // Put the charger into its safe remote-OFF state before resetting the Nano.
  // If the application is already offline this command simply has no effect.
  _nano.sendCommand("STOP");
  delay(150);
  _nano.pause();

  const bool ok = programFile(firmware, _firmwareSize, resultMessage);
  firmware.close();

  _bootSerial.end();
  setReset(false);

  // Restart the Nano application whether programming succeeded or failed.
  // On a failed partial flash the browser can simply upload the .bin again;
  // the Arduino bootloader itself is not overwritten by this updater.
  pulseReset();
  delay(350);
  _nano.resume();

  if (ok) {
    LittleFS.remove(NANO_UPDATE_FILE);
    _uploadReady = false;
    _firmwareSize = 0;
    _lastError = "";
  } else if (resultMessage.length() == 0) {
    resultMessage = _lastError.length() ? _lastError : String(F("Nano programming failed"));
  }

  return ok;
}

bool NanoFirmwareUpdater::storageReady() const { return _storageReady; }
size_t NanoFirmwareUpdater::firmwareSize() const { return _firmwareSize; }
const String& NanoFirmwareUpdater::lastError() const { return _lastError; }

void NanoFirmwareUpdater::setError(const String& message) {
  _lastError = message;
  if (ENABLE_DEBUG) {
    Serial.print(F("[NANO UPDATE] "));
    Serial.println(_lastError);
  }
}

void NanoFirmwareUpdater::cleanupUploadFile() {
  if (_uploadFile) _uploadFile.close();
  _uploadReady = false;
  _firmwareSize = 0;
  if (_storageReady) LittleFS.remove(NANO_UPDATE_FILE);
}

void NanoFirmwareUpdater::setReset(bool asserted) {
  // GPIO drives only the gate of the reset pull-down MOSFET.
  digitalWrite(PIN_NANO_RESET_GATE, asserted ? HIGH : LOW);
}

void NanoFirmwareUpdater::pulseReset() {
  setReset(true);
  delay(70);
  setReset(false);
  delay(35);
}

void NanoFirmwareUpdater::clearBootSerialInput() {
  while (_bootSerial.available() > 0) {
    (void)_bootSerial.read();
  }
}

int NanoFirmwareUpdater::readBootByte(unsigned long timeoutMs) {
  const unsigned long started = millis();
  while ((millis() - started) < timeoutMs) {
    if (_bootSerial.available() > 0) return _bootSerial.read();
    yield();
  }
  return -1;
}

bool NanoFirmwareUpdater::expectInSyncOk(unsigned long timeoutMs) {
  const unsigned long started = millis();
  while ((millis() - started) < timeoutMs) {
    const int value = readBootByte(40);
    if (value < 0) continue;
    if (value != STK_INSYNC) continue;
    return readBootByte(250) == STK_OK;
  }
  return false;
}

bool NanoFirmwareUpdater::syncBootloader(unsigned long baud) {
  _bootSerial.end();
  delay(5);
  _bootSerial.begin(baud, SERIAL_8N1, PIN_NANO_PROG_RX, PIN_NANO_PROG_TX);

  pulseReset();
  clearBootSerialInput();

  for (uint8_t attempt = 0; attempt < 12; ++attempt) {
    _bootSerial.write(STK_GET_SYNC);
    _bootSerial.write(CRC_EOP);
    _bootSerial.flush();

    if (expectInSyncOk(180)) {
      if (ENABLE_DEBUG) {
        Serial.print(F("[NANO UPDATE] bootloader synced at "));
        Serial.println(baud);
      }
      return true;
    }

    clearBootSerialInput();
    delay(35);
  }

  return false;
}

bool NanoFirmwareUpdater::enterProgrammingMode() {
  _bootSerial.write(STK_ENTER_PROGMODE);
  _bootSerial.write(CRC_EOP);
  _bootSerial.flush();
  return expectInSyncOk();
}

bool NanoFirmwareUpdater::leaveProgrammingMode() {
  _bootSerial.write(STK_LEAVE_PROGMODE);
  _bootSerial.write(CRC_EOP);
  _bootSerial.flush();
  return expectInSyncOk();
}

bool NanoFirmwareUpdater::loadAddress(uint16_t wordAddress) {
  _bootSerial.write(STK_LOAD_ADDRESS);
  _bootSerial.write(static_cast<uint8_t>(wordAddress & 0xFFU));
  _bootSerial.write(static_cast<uint8_t>((wordAddress >> 8) & 0xFFU));
  _bootSerial.write(CRC_EOP);
  _bootSerial.flush();
  return expectInSyncOk();
}

bool NanoFirmwareUpdater::programPage(const uint8_t* data, uint16_t length) {
  _bootSerial.write(STK_PROG_PAGE);
  _bootSerial.write(static_cast<uint8_t>((length >> 8) & 0xFFU));
  _bootSerial.write(static_cast<uint8_t>(length & 0xFFU));
  _bootSerial.write(MEMTYPE_FLASH);
  _bootSerial.write(data, length);
  _bootSerial.write(CRC_EOP);
  _bootSerial.flush();
  return expectInSyncOk(1800UL);
}

bool NanoFirmwareUpdater::verifyPage(const uint8_t* data, uint16_t length) {
  _bootSerial.write(STK_READ_PAGE);
  _bootSerial.write(static_cast<uint8_t>((length >> 8) & 0xFFU));
  _bootSerial.write(static_cast<uint8_t>(length & 0xFFU));
  _bootSerial.write(MEMTYPE_FLASH);
  _bootSerial.write(CRC_EOP);
  _bootSerial.flush();

  if (readBootByte(1200UL) != STK_INSYNC) return false;

  for (uint16_t i = 0; i < length; ++i) {
    if (readBootByte(500UL) != data[i]) return false;
  }

  return readBootByte(500UL) == STK_OK;
}

bool NanoFirmwareUpdater::programFile(File& firmware,
                                      size_t firmwareBytes,
                                      String& resultMessage) {
  unsigned long activeBaud = 0;

  if (syncBootloader(NANO_BOOT_BAUD_PRIMARY)) {
    activeBaud = NANO_BOOT_BAUD_PRIMARY;
  } else if (NANO_BOOT_BAUD_SECONDARY != NANO_BOOT_BAUD_PRIMARY &&
             syncBootloader(NANO_BOOT_BAUD_SECONDARY)) {
    activeBaud = NANO_BOOT_BAUD_SECONDARY;
  }

  if (activeBaud == 0) {
    setError(F("Could not sync with Nano bootloader. Check D0/D1 programming UART and RESET MOSFET wiring."));
    return false;
  }

  if (!enterProgrammingMode()) {
    setError(F("Nano bootloader refused programming mode"));
    return false;
  }

  uint8_t page[NANO_FLASH_PAGE_SIZE];
  size_t offset = 0;

  while (offset < firmwareBytes) {
    memset(page, 0xFF, sizeof(page));

    const size_t remaining = firmwareBytes - offset;
    const size_t wanted = remaining < sizeof(page) ? remaining : sizeof(page);
    const int bytesRead = firmware.read(page, wanted);
    if (bytesRead <= 0 || static_cast<size_t>(bytesRead) != wanted) {
      setError(F("Could not read stored Nano firmware during programming"));
      return false;
    }

    uint16_t writeLength = static_cast<uint16_t>(bytesRead);
    if ((writeLength & 1U) != 0U) {
      page[writeLength] = 0xFF;
      ++writeLength;
    }

    const uint16_t wordAddress = static_cast<uint16_t>(offset / 2U);

    if (!loadAddress(wordAddress) || !programPage(page, writeLength)) {
      setError(String(F("Nano flash write failed at byte ")) + String(offset));
      return false;
    }

    if (!loadAddress(wordAddress) || !verifyPage(page, writeLength)) {
      setError(String(F("Nano flash verify failed at byte ")) + String(offset));
      return false;
    }

    offset += static_cast<size_t>(bytesRead);
    yield();
  }

  // A missing reply here is not treated as a failed flash because some Nano
  // bootloaders reset immediately when LEAVE_PROGMODE is received.
  (void)leaveProgrammingMode();

  resultMessage = String(F("Nano firmware programmed and verified: ")) +
                  String(firmwareBytes) + F(" bytes, bootloader ") +
                  String(activeBaud) + F(" baud");
  return true;
}
