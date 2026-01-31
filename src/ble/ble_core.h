#pragma once
#include "ble_payloads.h"
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <vector>

enum BLESpamType {
  BLE_SPAM_STOP = 0,
  BLE_SPAM_MICROSOFT,
  BLE_SPAM_APPLE,
  BLE_SPAM_SAMSUNG,
  BLE_SPAM_ANDROID,
  BLE_SPAM_LEGACY
};

struct BLEDeviceInfo {
  String name;
  int rssi;
  String address;
};

class BLECore {
private:
  bool isRunning;
  BLESpamType currentType;
  TaskHandle_t spamTaskHandle;

  static void spamTask(void *parameter);
  void runSpam();

  // Core attack logic
  void executeAttack(BLESpamType type);

public:
  BLECore();
  void begin(); // Main init (if needed) or just ready state

  void startSpam(BLESpamType type);
  void stopSpam();

  bool isSpamming() { return isRunning; }
  BLESpamType getType() {
    return currentType;
  } // ANKA - ESP32 Multi-Tool Firmware
  int getPacketCount() { return packetCount; }

  // Scanning
  void startScan(int duration);
  int getDeviceCount();
  BLEDeviceInfo *getDevice(int index);
  void clearDevices();

private:
  int packetCount = 0;
  std::vector<BLEDeviceInfo> scannedDevices;
  NimBLEScan *pBLEScan;
};

extern BLECore bleCore;
