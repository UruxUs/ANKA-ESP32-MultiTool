/**
 * ANKA - ESP32 Multi-Tool Firmware
 * Menu System Implementation
 */

#include "menu.h"

// Forward declarations for menu callbacks
extern void wifiScan();
extern void wifiDeauth();
extern void wifiBeaconSpam();
extern void bleScan();
extern void bleAppleSpam();
extern void bleSamsungSpam();
extern void bleAndroidSpam();
extern void bleAllSpam();
extern void bleSourAppleSpam();       // Sour Apple logic
extern void bleMicrosoftSpam();       // Windows/Swift Pair
extern void bleMicrosoftLegacySpam(); // Windows/Swift Pair (Legacy)
extern void bleFlood();               // Aggressive BLE flood
extern void irReceive();
extern void irTransmit();
extern void irTvBGone();
extern void irSavedSignals();
extern void irManageSignals();
extern void irClearSignals();
extern void showDeviceInfo();
extern void showSettings();
extern void settingsBrightness();
extern void settingsTimeout();
extern void settingsRandomMac();
extern void showAbout();

MenuManager menuManager;

// Menu Implementation

Menu::Menu(const char *t) {
  title = t;
  itemCount = 0;
  selected = 0;
  parent = nullptr;
  active = false;
}

void Menu::addItem(const char *name, const uint8_t *icon,
                   std::function<void()> callback) {
  if (itemCount < MAX_MENU_ITEMS) {
    items[itemCount].name = name;
    items[itemCount].icon = icon;
    items[itemCount].callback = callback;
    items[itemCount].isSubmenu = false;
    itemCount++;
  }
}

void Menu::addSubmenu(const char *name, const uint8_t *icon, Menu *submenu) {
  if (itemCount < MAX_MENU_ITEMS) {
    items[itemCount].name = name;
    items[itemCount].icon = icon;
    // Capture submenu for callback
    items[itemCount].callback = [submenu]() {
      menuManager.setCurrentMenu(submenu);
      submenu->show();
    };
    items[itemCount].isSubmenu = true;
    itemCount++;
    submenu->setParent(this);
  }
}

void Menu::addBack() {
  addItem("< Back", ICON_BACK, [this]() {
    if (parent) {
      active = false;
      parent->show();
    }
  });
}

void Menu::setParent(Menu *p) { parent = p; }

Menu *Menu::getParent() { return parent; }

void Menu::show() {
  active = true;
  selected = 0;

  menuManager.setCurrentMenu(this);

  draw();
}

void Menu::draw() {
  const char *names[MAX_MENU_ITEMS];
  const uint8_t *icons[MAX_MENU_ITEMS];

  for (int i = 0; i < itemCount; i++) {
    names[i] = items[i].name;
    icons[i] = items[i].icon;
  }

  display.drawMenuWithIcons(title, names, icons, itemCount, selected);
}

void Menu::handleInput(ButtonEvent event) {
  Serial.printf("[Menu] handleInput called, active=%d, event=%d, selected=%d\n",
                active, event, selected);

  if (!active) {
    Serial.println("[Menu] Menu not active, ignoring input");
    return;
  }

  switch (event) {
  case BTN_UP_PRESS:
    Serial.println("[Menu] UP pressed");
    if (selected > 0) {
      selected--;
      draw();
    }
    break;

  case BTN_DOWN_PRESS:
    Serial.println("[Menu] DOWN pressed");
    if (selected < itemCount - 1) {
      selected++;
      draw();
    }
    break;

  case BTN_SELECT_PRESS:
    Serial.printf("[Menu] SELECT pressed, item=%s, hasCallback=%d\n",
                  items[selected].name, items[selected].callback != nullptr);
    if (items[selected].callback) {
      yield(); // Before callback
      items[selected].callback();
      yield(); // After callback
    }
    break;

  case BTN_BACK_PRESS:
    Serial.printf("[Menu] BACK pressed, hasParent=%d\n", parent != nullptr);
    if (parent) {
      active = false;
      parent->show();
    }
    break;

  default:
    break;
  }
}

bool Menu::isActive() { return active; }

int Menu::getSelected() { return selected; }

void Menu::setSelected(int idx) {
  if (idx >= 0 && idx < itemCount) {
    selected = idx;
  }
}

// MenuManager Implementation

MenuManager::MenuManager() {
  mainMenu = nullptr;
  currentMenu = nullptr;
}

void MenuManager::init() {
  createMenus();
  showMainMenu();
  Serial.println("[Menu] Initialized");
}

void MenuManager::createMenus() {
  // Main Menu
  mainMenu = new Menu(ANKA_NAME);

  // WiFi Menu
  wifiMenu = new Menu("WiFi");
  wifiMenu->addItem("Scan Networks", ICON_SCAN, wifiScan);
  wifiMenu->addItem("Deauther", ICON_ATTACK, wifiDeauth);
  wifiMenu->addItem("Beacon Spam", ICON_ATTACK, wifiBeaconSpam);
  wifiMenu->addBack();

  // BLE Menu
  bleMenu = new Menu("Bluetooth");
  bleMenu->addItem("Scan Devices", ICON_SCAN, bleScan);
  bleMenu->addItem("Apple Spam", ICON_ATTACK, bleAppleSpam);
  bleMenu->addItem("Windows Spam", ICON_ATTACK, bleMicrosoftSpam);
  bleMenu->addItem("Legacy Swift Pair", ICON_ATTACK, bleMicrosoftLegacySpam);
  bleMenu->addItem("Samsung Spam", ICON_ATTACK, bleSamsungSpam);
  bleMenu->addItem("Android Spam", ICON_ATTACK, bleAndroidSpam);
  bleMenu->addItem("Spam All", ICON_ATTACK, bleAllSpam);
  bleMenu->addItem("Sour Apple", ICON_ATTACK, bleSourAppleSpam);
  bleMenu->addItem("BLE Flood", ICON_ATTACK, bleFlood);

  bleMenu->addBack();

  // IR Menu
  irMenu = new Menu("IR Remote");
  irMenu->addItem("Receive Signal", ICON_SCAN, irReceive);
  irMenu->addItem("Transmit Signal", ICON_IR, irTransmit);
  irMenu->addItem("Saved Signals", ICON_INFO, irSavedSignals);
  irMenu->addItem("Manage Signals", ICON_SETTINGS, irManageSignals);
  irMenu->addItem("TV-B-Gone", ICON_ATTACK, irTvBGone);
  irMenu->addBack();

  // Settings Menu
  settingsMenu = new Menu("Settings");
  settingsMenu->addItem("Brightness", ICON_SETTINGS, settingsBrightness);
  settingsMenu->addItem("Screen Timeout", ICON_SETTINGS, settingsTimeout);
  settingsMenu->addItem("Random MAC", ICON_SETTINGS, settingsRandomMac);
  settingsMenu->addItem("About", ICON_INFO, showAbout);
  settingsMenu->addBack();

  // Info Menu
  infoMenu = new Menu("Device Info");
  infoMenu->addItem("View Info", ICON_INFO, showDeviceInfo);
  infoMenu->addBack();

  // Build main menu
  mainMenu->addSubmenu("WiFi", ICON_WIFI, wifiMenu);
  mainMenu->addSubmenu("Bluetooth", ICON_BLE, bleMenu);
  mainMenu->addSubmenu("IR Remote", ICON_IR, irMenu);
  mainMenu->addSubmenu("Settings", ICON_SETTINGS, settingsMenu);
  mainMenu->addSubmenu("Device Info", ICON_INFO, infoMenu);

  // Set parent for all submenus (required for BACK button to work!)
  wifiMenu->setParent(mainMenu);
  bleMenu->setParent(mainMenu);
  irMenu->setParent(mainMenu);
  settingsMenu->setParent(mainMenu);
  infoMenu->setParent(mainMenu);

  currentMenu = mainMenu;
}

void MenuManager::update() {
  // Called every loop iteration if needed
}

void MenuManager::handleInput(ButtonEvent event) {
  if (currentMenu) {
    currentMenu->handleInput(event);
  }
}

void MenuManager::showMainMenu() {
  currentMenu = mainMenu;
  mainMenu->show();
}

void MenuManager::showWifiMenu() {
  currentMenu = wifiMenu;
  wifiMenu->show();
}

void MenuManager::showBleMenu() {
  currentMenu = bleMenu;
  bleMenu->show();
}

void MenuManager::showIrMenu() {
  currentMenu = irMenu;
  irMenu->show();
}

void MenuManager::showSettingsMenu() {
  currentMenu = settingsMenu;
  settingsMenu->show();
}

void MenuManager::showInfoMenu() {
  currentMenu = infoMenu;
  infoMenu->show();
}

void MenuManager::goBack() {
  if (currentMenu && currentMenu->getParent()) {
    currentMenu = currentMenu->getParent();
    currentMenu->show();
  }
}

void MenuManager::goHome() { showMainMenu(); }

void MenuManager::setCurrentMenu(Menu *menu) {
  currentMenu = menu;
  Serial.printf("[Menu] currentMenu changed to %p\n", menu);
}

Menu *MenuManager::getCurrentMenu() { return currentMenu; }
