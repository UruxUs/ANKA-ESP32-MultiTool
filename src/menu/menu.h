/**
 * ANKA - ESP32 Multi-Tool Firmware
 * Menu System Header
 */

#ifndef MENU_H
#define MENU_H

#include "../display/display.h"
#include "../input/buttons.h"
#include <Arduino.h>
#include <functional>

#define MAX_MENU_ITEMS 16

struct MenuItem {
  const char *name;
  const uint8_t *icon;
  std::function<void()> callback;
  bool isSubmenu;
};

class Menu {
public:
  Menu(const char *title);

  void addItem(const char *name, const uint8_t *icon,
               std::function<void()> callback);
  void addSubmenu(const char *name, const uint8_t *icon, Menu *submenu);
  void addBack();

  void setParent(Menu *parent);
  Menu *getParent();

  void show();
  void handleInput(ButtonEvent event);
  bool isActive();

  int getSelected();
  void setSelected(int idx);

private:
  const char *title;
  MenuItem items[MAX_MENU_ITEMS];
  int itemCount;
  int selected;
  Menu *parent;
  bool active;

  void draw();
};

// Main application menus
class MenuManager {
public:
  MenuManager();

  void init();
  void update();
  void handleInput(ButtonEvent event);

  void showMainMenu();
  void showWifiMenu();
  void showBleMenu();
  void showIrMenu();
  void showSettingsMenu();
  void showInfoMenu();

  void goBack();
  void goHome();
  void setCurrentMenu(Menu *menu);

  Menu *getCurrentMenu();

private:
  Menu *mainMenu;
  Menu *wifiMenu;
  Menu *wifiScanMenu;
  Menu *wifiAttackMenu;
  Menu *bleMenu;
  Menu *bleScanMenu;
  Menu *bleSpamMenu;
  Menu *irMenu;
  Menu *settingsMenu;
  Menu *infoMenu;

  Menu *currentMenu;

  void createMenus();
};

// Global menu manager
extern MenuManager menuManager;

#endif // MENU_H
