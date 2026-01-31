/**
 * ANKA - ESP32 Multi-Tool Firmware
 * Main Application Entry Point
 *
 * A multi-feature security research tool for ESP32
 * Features: WiFi Deauther, BLE Spam, IR Remote
 */

#include "ble/ble_core.h"
#include "config.h"
#include "display/display.h"
#include "input/buttons.h"
#include "ir/ir_module.h"
#include "menu/menu.h"
#include "wifi/wifi_module.h"
#include <Arduino.h>
#include <esp_wifi.h>

// Globals
unsigned long lastActivityTime = 0;
bool screenSaverActive = false;

// Callbacks

// WiFi callbacks
void wifiScan() {
  display.drawMessage("WiFi", "Scanning...");

  int count = wifiModule.scanNetworks();

  if (count == 0) {
    display.drawMessage("WiFi", "No networks found");
    delay(2000);
    menuManager.showWifiMenu();
    return;
  }

  // Build arrays for scrolling network list
  const char *ssids[MAX_SCAN_RESULTS];
  int rssis[MAX_SCAN_RESULTS];

  for (int i = 0; i < count; i++) {
    AccessPoint *ap = wifiModule.getNetwork(i);
    ssids[i] = ap->ssid.c_str();
    rssis[i] = ap->rssi;
  }

  // Show scrolling network list
  int selected = display.showNetworkList(ssids, rssis, count, "WiFi Networks");

  if (selected >= 0) {
    AccessPoint *ap = wifiModule.getNetwork(selected);
    char info[64];
    snprintf(info, sizeof(info), "CH:%d %s", ap->channel,
             wifiModule.getEncryptionType(ap->encryption).c_str());
    display.drawMessage(ap->ssid.c_str(), info);
    delay(3000);
  }

  menuManager.showWifiMenu();
}

void wifiDeauth() {
  // First scan for targets
  display.drawMessage("Deauth", "Scanning targets...");

  int count = wifiModule.scanNetworks();

  if (count == 0) {
    display.drawMessage("Deauth", "No targets found");
    delay(2000);
    menuManager.showWifiMenu();
    return;
  }

  // Build arrays for network selection UI
  const char **ssids = new const char *[count];
  int *rssis = new int[count];

  for (int i = 0; i < count; i++) {
    AccessPoint *ap = wifiModule.getNetwork(i);
    ssids[i] = ap->ssid.c_str();
    rssis[i] = ap->rssi;
  }

  // Show network selection UI
  int selectedIndex =
      display.showNetworkList(ssids, rssis, count, "Select Target");

  // Cleanup
  delete[] ssids;
  delete[] rssis;

  if (selectedIndex == -1) {
    // User cancelled
    menuManager.showWifiMenu();
    return;
  }

  // Get selected network
  AccessPoint *target = wifiModule.getNetwork(selectedIndex);

  char msg[40];
  snprintf(msg, sizeof(msg), "Target: %s", target->ssid.c_str());
  display.drawMessage("Deauth", msg);
  delay(1000);

  wifiModule.startDeauth(target->bssid, target->channel);

  Serial.println("[DEBUG] Deauth started, entering button loop...");
  display.drawMessage("Deauth", "Attack running...");

  // Wait for button press to stop
  unsigned long lastUpdate = 0;
  while (true) {
    buttons.update();
    ButtonEvent evt = buttons.getEvent();

    if (evt == BTN_BACK_PRESS || evt == BTN_SELECT_PRESS) {
      Serial.printf("[DEBUG] Button pressed: %d, exiting...\n", evt);
      break;
    }

    // Update display (1Hz)
    if (millis() - lastUpdate > 1000) {
      char status[32];
      snprintf(status, sizeof(status), "Packets: %d",
               wifiModule.getDeauthPacketsSent());
      display.drawMessage("Deauth", status);
      lastUpdate = millis();
    }

    delay(10);
  }

  wifiModule.stopDeauth();
  display.drawMessage("Deauth", "Stopped");
  delay(1000);

  menuManager.showWifiMenu();
}

void wifiBeaconSpam() {
  display.drawMessage("Beacon Spam", "Starting...");
  delay(500);

  wifiModule.startRandomBeaconSpam(20);

  display.drawMessage("Beacon Spam", "Running...");

  while (true) {
    buttons.update();
    ButtonEvent evt = buttons.getEvent();

    if (evt == BTN_BACK_PRESS || evt == BTN_SELECT_PRESS) {
      break;
    }

    delay(10);
  }

  wifiModule.stopBeaconSpam();
  display.drawMessage("Beacon Spam", "Stopped");
  delay(1000);

  menuManager.showWifiMenu();
}

// BLE callbacks
#include "ble/ble_core.h"
// ... (keep other includes)

// ...

// BLE callbacks
// BLE
void bleScan() {
  display.drawMessage("BLE", "Scanning...");

  // Scan for 5 seconds
  bleCore.startScan(5);
  int count = bleCore.getDeviceCount();

  if (count == 0) {
    display.drawMessage("BLE", "No devices found");
    delay(2000);
    menuManager.showBleMenu();
    return;
  }

  // Build arrays for scrolling list
  // We need to manage memory carefully here
  const char **names = new const char *[count];
  int *rssis = new int[count];

  // Temporary buffers for names if needed, but BLEDeviceInfo has String
  // We need to keep the Strings alive while displaying.
  // bleCore keeps them alive in scannedDevices vector.

  for (int i = 0; i < count; i++) {
    BLEDeviceInfo *dev = bleCore.getDevice(i);
    if (dev->name.length() > 0) {
      names[i] = dev->name.c_str();
    } else {
      names[i] = dev->address.c_str(); // Fallback to MAC
    }
    rssis[i] = dev->rssi;
  }

  // Show scrolling list
  int selected = display.showNetworkList(names, rssis, count, "BLE Devices");

  // Cleanup arrays (but not content strings as they are in bleCore)
  delete[] names;
  delete[] rssis;

  if (selected >= 0) {
    BLEDeviceInfo *dev = bleCore.getDevice(selected);
    char addr[20];
    snprintf(addr, sizeof(addr), "%s", dev->address.c_str());

    // Show details
    display.drawMessage(dev->name.length() > 0 ? dev->name.c_str() : "Unknown",
                        addr);

    // Wait for user to dismiss
    while (true) {
      buttons.update();
      if (buttons.getEvent() != BTN_NONE)
        break;
      delay(10);
    }
  }

  menuManager.showBleMenu();
}

void bleAppleSpam() {
  display.drawMessage("Apple Spam", "Starting...");
  delay(500);

  bleCore.startSpam(BLE_SPAM_APPLE);

  while (true) {
    buttons.update();
    ButtonEvent evt = buttons.getEvent();

    if (evt == BTN_BACK_PRESS || evt == BTN_SELECT_PRESS) {
      break;
    }

    char msg[32];
    snprintf(msg, sizeof(msg), "Sent: %d", bleCore.getPacketCount());
    display.drawMessage("Apple Spam", msg);
    delay(100);
  }

  bleCore.stopSpam();
  display.drawMessage("Apple Spam", "Stopped");
  delay(1000);
  menuManager.showBleMenu();
}

void bleSourAppleSpam() {
  bleAppleSpam(); // Redirect for now
}

void bleSamsungSpam() {
  display.drawMessage("Samsung Spam", "Starting...");
  delay(500);

  bleCore.startSpam(BLE_SPAM_SAMSUNG);

  while (true) {
    buttons.update();
    ButtonEvent evt = buttons.getEvent();

    if (evt == BTN_BACK_PRESS || evt == BTN_SELECT_PRESS) {
      break;
    }
    char msg[32];
    snprintf(msg, sizeof(msg), "Sent: %d", bleCore.getPacketCount());
    display.drawMessage("Samsung Spam", msg);
    delay(100);
  }

  bleCore.stopSpam();
  menuManager.showBleMenu();
}

void bleMicrosoftSpam() {
  display.drawMessage("Windows Spam", "Starting...");
  delay(500);

  bleCore.startSpam(BLE_SPAM_MICROSOFT);

  while (true) {
    buttons.update();
    ButtonEvent evt = buttons.getEvent();

    if (evt == BTN_BACK_PRESS || evt == BTN_SELECT_PRESS) {
      break;
    }
    char msg[32];
    snprintf(msg, sizeof(msg), "Sent: %d", bleCore.getPacketCount());
    display.drawMessage("Windows Spam", msg);
    delay(100);
  }

  bleCore.stopSpam();
  menuManager.showBleMenu();
}

void bleMicrosoftLegacySpam() {
  display.drawMessage("Legacy Spam", "Starting...");
  delay(500);
  bleCore.startSpam(BLE_SPAM_LEGACY);
  while (true) {
    buttons.update();
    if (buttons.getEvent() == BTN_BACK_PRESS)
      break;
    char msg[32];
    snprintf(msg, sizeof(msg), "Sent: %d", bleCore.getPacketCount());
    display.drawMessage("Legacy Spam", msg);
    delay(100);
  }
  bleCore.stopSpam();
  menuManager.showBleMenu();
}

void bleAndroidSpam() {
  display.drawMessage("Android Spam", "Starting...");
  delay(500);
  bleCore.startSpam(BLE_SPAM_ANDROID);
  while (true) {
    buttons.update();
    if (buttons.getEvent() == BTN_BACK_PRESS)
      break;
    char msg[32];
    snprintf(msg, sizeof(msg), "Sent: %d", bleCore.getPacketCount());
    display.drawMessage("Android Spam", msg);
    delay(100);
  }
  bleCore.stopSpam();
  menuManager.showBleMenu();
}

void bleAllSpam() {
  display.drawMessage("All Spam", "Starting...");
  delay(500);
  bleCore.startSpam(BLE_SPAM_MICROSOFT);

  // Mixed spam loop
  while (true) {
    buttons.update();
    if (buttons.getEvent() == BTN_BACK_PRESS)
      break;
    char msg[32];
    snprintf(msg, sizeof(msg), "Sent: %d", bleCore.getPacketCount());
    display.drawMessage("All Spam (Mix)", msg);
    delay(100);
  }
  bleCore.stopSpam();
  menuManager.showBleMenu();
}

void bleFlood() {
  // Legacy Flooding
  display.drawMessage("Flood", "Starting...");
  delay(500);
  bleCore.startSpam(BLE_SPAM_LEGACY);
  while (true) {
    buttons.update();
    if (buttons.getEvent() == BTN_BACK_PRESS)
      break;
    char msg[32];
    snprintf(msg, sizeof(msg), "Sent: %d", bleCore.getPacketCount());
    display.drawMessage("Flood", msg);
    delay(100);
  }
  bleCore.stopSpam();
  menuManager.showBleMenu();
}

// IR
void irReceive() {
  display.drawMessage("IR Receive", "Point remote...");

  irModule.startReceive();

  unsigned long startTime = millis();
  bool signalReceived = false;

  while (millis() - startTime < IR_RECEIVE_TIMEOUT * 1000) {
    buttons.update();
    ButtonEvent evt = buttons.getEvent();

    if (evt == BTN_BACK_PRESS) {
      break;
    }

    if (irModule.hasSignal()) {
      signalReceived = true;
      break;
    }

    delay(10);
  }

  irModule.stopReceive();

  if (signalReceived) {
    String protocol = irModule.getLastProtocol();
    String value = irModule.getLastValue();

    char msg[40];
    snprintf(msg, sizeof(msg), "%s: %s", protocol.c_str(), value.c_str());
    display.drawMessage("IR Received", msg);
    delay(1500);

    // Ask user if they want to save the signal
    display.drawConfirm("Save Signal?", "SELECT=Yes BACK=No");

    while (true) {
      buttons.update();
      ButtonEvent evt = buttons.getEvent();

      if (evt == BTN_SELECT_PRESS) {
        // Auto-generate numbered name
        int nextNum = irModule.getSignalCount() + 1;
        char autoName[16];
        snprintf(autoName, sizeof(autoName), "%d", nextNum);

        // Read and save the signal
        IRSignal signal;
        if (irModule.readSignal(&signal)) {
          if (irModule.saveSignal(autoName, &signal)) {
            char saveMsg[32];
            snprintf(saveMsg, sizeof(saveMsg), "Saved as '%s'", autoName);
            display.drawMessage("IR Save", saveMsg);
          } else {
            display.drawMessage("IR Save", "Save failed!");
          }
        } else {
          display.drawMessage("IR Save", "Read error!");
        }
        delay(1500);
        break;
      } else if (evt == BTN_BACK_PRESS) {
        // Skip saving
        break;
      }
      delay(10);
    }
  } else {
    display.drawMessage("IR Receive", "No signal");
    delay(2000);
  }

  menuManager.showIrMenu();
}

void irTransmit() {
  // Send last received signal
  if (!irModule.hasLastSignal()) {
    display.drawMessage("Replay Last", "No signal saved");
    delay(2000);
    menuManager.showIrMenu();
    return;
  }

  // Read and send the last received signal
  IRSignal signal;
  if (irModule.readSignal(&signal)) {
    String protocol = irModule.getLastProtocol();
    String value = irModule.getLastValue();

    char msg[40];
    snprintf(msg, sizeof(msg), "%s: %s", protocol.c_str(), value.c_str());
    display.drawMessage("Sending...", msg);

    irModule.sendSignal(&signal);

    display.drawMessage("Replay Last", "Sent!");
    delay(1000);
  } else {
    display.drawMessage("Replay Last", "Read error!");
    delay(1500);
  }

  menuManager.showIrMenu();
}

void irTvBGone() {
  display.drawMessage("TV-B-Gone", "Starting...");
  delay(500);

  irModule.startTvBGone();

  while (irModule.isTvBGoneRunning()) {
    buttons.update();
    ButtonEvent evt = buttons.getEvent();

    if (evt == BTN_BACK_PRESS || evt == BTN_SELECT_PRESS) {
      irModule.stopTvBGone();
      break;
    }

    display.drawProgress("TV-B-Gone", "Sending codes...",
                         irModule.getTvBGoneProgress());

    delay(100);
  }

  display.drawMessage("TV-B-Gone", "Complete!");
  delay(1000);

  menuManager.showIrMenu();
}

void irSavedSignals() {
  int count = irModule.getSignalCount();

  if (count == 0) {
    display.drawMessage("Saved Signals", "No signals saved");
    delay(2000);
    menuManager.showIrMenu();
    return;
  }

  // Pre-load signal names for menu
  char menuStrings[MAX_IR_SIGNALS + 1][32];
  const char *menuPtrs[MAX_IR_SIGNALS + 1];

  for (int i = 0; i < count && i < MAX_IR_SIGNALS; i++) {
    IRSignal signal;
    if (irModule.loadSignal(i, &signal)) {
      String proto = typeToString(signal.protocol);
      // Format: "1: NEC A90C..."
      snprintf(menuStrings[i], 32, "%d: %s %llX", i + 1, proto.c_str(),
               signal.value);
    } else {
      snprintf(menuStrings[i], 32, "%d: Error", i + 1);
    }
    menuPtrs[i] = menuStrings[i];
  }
  menuPtrs[count] = "< Back";

  int selected = 0;
  int scrollOffset = 0;
  unsigned long lastScrollTime = 0;
  bool running = true;

  while (running) {
    // Scrolling logic
    if (millis() - lastScrollTime > 50) {
      // Calculate text width of selected item
      int textWidth = display.getStringWidth(menuPtrs[selected]);
      int boxWidth = OLED_WIDTH - 8;

      if (textWidth > boxWidth) {
        scrollOffset += 2;
        if (scrollOffset > (textWidth - boxWidth + 20)) {
          scrollOffset = 0; // Reset
        }
      } else {
        scrollOffset = 0;
      }
      lastScrollTime = millis();
    }

    display.drawScrollingMenu("Saved Signals", menuPtrs, count + 1, selected,
                              scrollOffset);

    // Handle input
    buttons.update();
    ButtonEvent evt = buttons.getEvent();

    if (evt != BTN_NONE) {
      // Reset scroll on interaction
      scrollOffset = 0;
    }

    if (evt == BTN_UP_PRESS && selected > 0) {
      selected--;
    } else if (evt == BTN_DOWN_PRESS && selected < count) {
      selected++;
    } else if (evt == BTN_SELECT_PRESS) {
      if (selected == count) {
        running = false;
      } else {
        // Send selected signal
        IRSignal signal;
        if (irModule.loadSignal(selected, &signal)) {
          display.drawMessage("Sending...", menuStrings[selected]);
          irModule.sendSignal(&signal);
          delay(500);
        }
      }
    } else if (evt == BTN_BACK_PRESS) {
      running = false;
    }

    delay(10);
  }

  menuManager.showIrMenu();
}

// Helper for overwrite
void irOverwriteSignal(int index) {
  display.drawMessage("Overwrite", "Waiting for signal...");
  delay(500);

  irModule.startReceive();

  bool waiting = true;
  while (waiting) {
    buttons.update();
    if (buttons.getEvent() == BTN_BACK_PRESS) {
      waiting = false;
    }

    if (irModule.hasSignal()) {
      IRSignal signal;
      if (irModule.readSignal(&signal)) {
        // Generate name
        char name[32];
        snprintf(name, sizeof(name), "Signal %d", index + 1);

        if (irModule.saveSignalAtIndex(index, name, &signal)) {
          display.drawMessage("Success", "Signal Overwritten!");
        } else {
          display.drawMessage("Error", "Save Failed");
        }
        delay(1000);
        waiting = false;
      }
    }
    delay(10);
  }

  irModule.stopReceive();
}

void irManageSignals() {
  bool running = true;
  int selected = 0;

  while (running) {
    int count = irModule.getSignalCount();

    // Build menu
    char menuStrings[MAX_IR_SIGNALS + 2][32];
    const char *menuPtrs[MAX_IR_SIGNALS + 2];

    for (int i = 0; i < count && i < MAX_IR_SIGNALS; i++) {
      IRSignal signal;
      if (irModule.loadSignal(i, &signal)) {
        snprintf(menuStrings[i], 32, "%d: %s %llX", i + 1,
                 typeToString(signal.protocol).c_str(), signal.value);
      } else {
        snprintf(menuStrings[i], 32, "%d: Error", i + 1);
      }
      menuPtrs[i] = menuStrings[i];
    }

    menuPtrs[count] = "Clear All";
    menuPtrs[count + 1] = "< Back";

    // Draw
    display.drawMenu("Manage Signals", menuPtrs, count + 2, selected);

    // Input
    buttons.update();
    ButtonEvent evt = buttons.getEvent();

    if (evt == BTN_UP_PRESS && selected > 0)
      selected--;
    else if (evt == BTN_DOWN_PRESS && selected < count + 1)
      selected++;
    else if (evt == BTN_BACK_PRESS)
      running = false;
    else if (evt == BTN_SELECT_PRESS) {
      if (selected == count + 1) {
        running = false;
      } else if (selected == count) {
        // Clear All
        display.drawConfirm("Clear All?", "Are you sure?");
        bool confirmed = false;
        // Simple confirm loop
        while (true) {
          buttons.update();
          ButtonEvent e = buttons.getEvent();
          if (e == BTN_SELECT_PRESS) {
            confirmed = true;
            break;
          }
          if (e == BTN_BACK_PRESS)
            break;
          delay(10);
        }
        if (confirmed) {
          irModule.clearSignals();
          display.drawMessage("Done", "Signals Cleared");
          delay(1000);
          selected = 0; // Reset
        }
      } else {
        // Signal selected - Show Submenu
        const char *options[] = {"Delete", "Overwrite", "< Back"};
        int subSel = 0;
        bool subRun = true;

        while (subRun) {
          display.drawMenu("Action", options, 3, subSel);
          buttons.update();
          ButtonEvent e = buttons.getEvent();

          if (e == BTN_UP_PRESS && subSel > 0)
            subSel--;
          else if (e == BTN_DOWN_PRESS && subSel < 2)
            subSel++;
          else if (e == BTN_BACK_PRESS)
            subRun = false;
          else if (e == BTN_SELECT_PRESS) {
            if (subSel == 2)
              subRun = false;
            else if (subSel == 0) {
              // Delete
              irModule.deleteSignal(selected);
              display.drawMessage("Deleted", "Signal removed");
              delay(500);
              subRun = false;
            } else if (subSel == 1) {
              // Overwrite
              irOverwriteSignal(selected);
              subRun = false;
            }
          }
          delay(10);
        }
      }
    }
    delay(10);
  }

  menuManager.showIrMenu();
}

// Legacy function - redirects to manage
void irClearSignals() { irManageSignals(); }

// System
void showDeviceInfo() {
  display.clear();

  display.drawHeader("Device Info");

  char buf[40];

  display.drawString(4, 26, ANKA_NAME);
  snprintf(buf, sizeof(buf), "Version: %s", ANKA_VERSION);
  display.drawString(4, 38, buf);

  snprintf(buf, sizeof(buf), "Heap: %d KB", ESP.getFreeHeap() / 1024);
  display.drawString(4, 50, buf);

  snprintf(buf, sizeof(buf), "Flash: %d MB",
           ESP.getFlashChipSize() / (1024 * 1024));
  display.drawString(4, 62, buf);

  display.update();

  while (true) {
    buttons.update();
    if (buttons.getEvent() != BTN_NONE)
      break;
    delay(10);
  }

  menuManager.showInfoMenu();
}

// Settings
uint8_t currentBrightness = 255;

void settingsBrightness() {
  const char *levels[] = {"25%", "50%", "75%", "100%"};
  int values[] = {64, 128, 192, 255};
  int selected = 3; // Default 100%

  // Find current level
  for (int i = 0; i < 4; i++) {
    if (values[i] == currentBrightness) {
      selected = i;
      break;
    }
  }

  display.drawList("Brightness", levels, 4, 0, selected);

  while (true) {
    buttons.update();
    ButtonEvent evt = buttons.getEvent();

    if (evt == BTN_UP_PRESS && selected > 0) {
      selected--;
      display.drawList("Brightness", levels, 4, 0, selected);
    } else if (evt == BTN_DOWN_PRESS && selected < 3) {
      selected++;
      display.drawList("Brightness", levels, 4, 0, selected);
    } else if (evt == BTN_SELECT_PRESS) {
      currentBrightness = values[selected];
      display.setBrightness(currentBrightness);
      display.drawMessage("Brightness", "Applied!");
      delay(1000);
      menuManager.showSettingsMenu();
      return;
    } else if (evt == BTN_BACK_PRESS) {
      menuManager.showSettingsMenu();
      return;
    }

    delay(10);
  }
}

void settingsTimeout() {
  const char *times[] = {"30 sec", "1 min", "2 min", "5 min", "Never"};
  unsigned long values[] = {30000, 60000, 120000, 300000, 0};
  int selected = 1; // Default 1 min

  display.drawList("Screen Timeout", times, 5, 0, selected);

  while (true) {
    buttons.update();
    ButtonEvent evt = buttons.getEvent();

    if (evt == BTN_UP_PRESS && selected > 0) {
      selected--;
      display.drawList("Screen Timeout", times, 5, 0, selected);
    } else if (evt == BTN_DOWN_PRESS && selected < 4) {
      selected++;
      display.drawList("Screen Timeout", times, 5, 0, selected);
    } else if (evt == BTN_SELECT_PRESS) {
      // Would save to EEPROM/NVS in production
      display.drawMessage("Timeout", "Applied!");
      delay(1000);
      menuManager.showSettingsMenu();
      return;
    } else if (evt == BTN_BACK_PRESS) {
      menuManager.showSettingsMenu();
      return;
    }

    delay(10);
  }
}

void settingsRandomMac() {
  display.drawMessage("Random MAC", "Randomizing...");
  delay(500);

  // Randomize WiFi MAC
  uint8_t mac[6];
  for (int i = 0; i < 6; i++) {
    mac[i] = random(256);
  }
  mac[0] = (mac[0] & 0xFE) | 0x02; // Locally administered, unicast

  esp_wifi_set_mac(WIFI_IF_STA, mac);

  char macStr[20];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],
           mac[1], mac[2], mac[3], mac[4], mac[5]);

  display.drawMessage("New MAC:", macStr);

  while (true) {
    buttons.update();
    if (buttons.getEvent() != BTN_NONE)
      break;
    delay(10);
  }

  menuManager.showSettingsMenu();
}

void showAbout() {
  display.clear();

  display.drawHeader("About");

  display.drawString(4, 24, ANKA_NAME " v" ANKA_VERSION);
  display.drawString(4, 36, "ESP32 Security Tool");
  display.drawString(4, 48, "For research only!");
  display.drawString(4, 60, "(c) 2026");

  display.update();

  while (true) {
    buttons.update();
    if (buttons.getEvent() != BTN_NONE)
      break;
    delay(10);
  }

  menuManager.showSettingsMenu();
}

void showSettings() {
  // This is now handled by the submenu
  menuManager.showSettingsMenu();
}

// Setup
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(100);

  Serial.println();
  Serial.println("================================");
  Serial.println("  " ANKA_NAME " Multi-Tool v" ANKA_VERSION);
  Serial.println("================================");
  Serial.println();

  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // Initialize display
  Serial.println("[Boot] Initializing display...");
  display.init();
  display.drawSplashScreen();
  delay(1500);

  // Initialize buttons
  Serial.println("[Boot] Initializing buttons...");
  buttons.init();

  // Initialize WiFi module
  Serial.println("[Boot] Initializing WiFi...");
  wifiModule.init();

  // Initialize BLE module
  Serial.println("[Boot] Initializing BLE...");
  bleCore.begin();

  // Initialize IR module
  Serial.println("[Boot] Initializing IR...");
  irModule.init();

  // Initialize menu system
  Serial.println("[Boot] Initializing menu...");
  menuManager.init();

  // Ready
  Serial.println("[Boot] Ready!");
  Serial.println();

  digitalWrite(LED_PIN, LOW);
  lastActivityTime = millis();
}

// Main Loop
void loop() {
  // Yield to prevent watchdog issues
  yield();

  // Update buttons
  buttons.update();
  ButtonEvent event = buttons.getEvent();

  // Handle screen saver
  if (event != BTN_NONE) {
    lastActivityTime = millis();

    if (screenSaverActive) {
      screenSaverActive = false;
      display.wake();
      menuManager.showMainMenu();
    } else {
      // Pass event to menu
      menuManager.handleInput(event);
    }
    yield(); // After handling input
  }

  // Check screen timeout
  if (!screenSaverActive && (millis() - lastActivityTime > SCREEN_TIMEOUT)) {
    screenSaverActive = true;
    display.sleep();
  }

  // Update menu
  menuManager.update();

  // Small delay to prevent CPU hogging
  delay(10);
}
