#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <SoftwareSerial.h>

class NanoLink;

class NanoFirmwareUpdater {
public:
  explicit NanoFirmwareUpdater(NanoLink& nanoLink);

  void begin();

  bool beginUpload(const String& filename);
  bool writeUpload(const uint8_t* data, size_t length);
  bool endUpload();
  void abortUpload();

  bool flashUploadedFirmware(String& resultMessage);

  bool storageReady() const;
  size_t firmwareSize() const;
  const String& lastError() const;

private:
  NanoLink& _nano;
  SoftwareSerial _bootSerial;
  File _uploadFile;

  bool _storageReady;
  bool _uploadReady;
  size_t _firmwareSize;
  String _lastError;

  void setError(const String& message);
  void cleanupUploadFile();

  void setReset(bool asserted);
  void pulseReset();
  void clearBootSerialInput();
  int readBootByte(unsigned long timeoutMs);
  bool expectInSyncOk(unsigned long timeoutMs = 1200UL);
  bool syncBootloader(unsigned long baud);
  bool enterProgrammingMode();
  bool leaveProgrammingMode();
  bool loadAddress(uint16_t wordAddress);
  bool programPage(const uint8_t* data, uint16_t length);
  bool verifyPage(const uint8_t* data, uint16_t length);
  bool programFile(File& firmware, size_t firmwareBytes, String& resultMessage);
};
