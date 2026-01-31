/**
 * ANKA - ESP32 Multi-Tool Firmware
 * WiFi Module Header
 */

#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#include "../config.h"
#include "esp_wifi.h"
#include <Arduino.h>
#include <WiFi.h>

struct AccessPoint {
  String ssid;
  uint8_t bssid[6];
  int32_t rssi;
  uint8_t channel;
  uint8_t encryption;
  bool selected;
};

class WiFiModule {
public:
  WiFiModule();

  void init();
  void deinit();

  // Scanning
  int scanNetworks();
  AccessPoint *getNetwork(int index);
  int getNetworkCount();
  void clearNetworks();

  // Deauther
  void startDeauth(uint8_t *targetBssid, uint8_t channel);
  void startDeauthBroadcast(uint8_t channel);
  void stopDeauth();
  bool isDeauthRunning();
  int getDeauthPacketsSent();

  // Beacon Spam
  void startBeaconSpam(const char **ssids, int count);
  void startRandomBeaconSpam(int count);
  void stopBeaconSpam();
  bool isBeaconSpamRunning();

  // Utilities
  String macToString(uint8_t *mac);
  void stringToMac(const char *str, uint8_t *mac);
  String getEncryptionType(uint8_t type);

private:
  AccessPoint networks[MAX_SCAN_RESULTS];
  int networkCount;

  bool deauthRunning;
  bool beaconSpamRunning;
  int deauthPacketsSent;

  uint8_t deauthTarget[6];
  uint8_t deauthChannel;

  // Raw frame templates
  static const uint8_t deauthFrame[];
  static const uint8_t beaconFrame[];

  void sendDeauthPacket(uint8_t *target, uint8_t *source, uint8_t channel);
  void sendBeaconPacket(const char *ssid, uint8_t *mac, uint8_t channel);
  void generateRandomMac(uint8_t *mac);
  void generateRandomSSID(char *ssid, int maxLen);

  static void deauthTask(void *param);
  static void beaconTask(void *param);
  TaskHandle_t deauthTaskHandle;
  TaskHandle_t beaconTaskHandle;
};

// Global WiFi module instance
extern WiFiModule wifiModule;

#endif // WIFI_MODULE_H
