/**
 * ANKA - ESP32 Multi-Tool Firmware
 * Display Module Header
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include "../config.h"
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// Icons (8x8 bitmaps)
extern const uint8_t ICON_WIFI[];
extern const uint8_t ICON_BLE[];
extern const uint8_t ICON_IR[];
extern const uint8_t ICON_SETTINGS[];
extern const uint8_t ICON_INFO[];
extern const uint8_t ICON_BACK[];
extern const uint8_t ICON_ATTACK[];
extern const uint8_t ICON_SCAN[];

class Display {
public:
  Display();

  void init();
  void clear();
  void update();

  // Basic drawing
  void drawString(int x, int y, const char *str);
  void drawStringCenter(int y, const char *str);
  void drawIcon(int x, int y, const uint8_t *icon);
  void drawRect(int x, int y, int w, int h);
  void drawFilledRect(int x, int y, int w, int h);
  void drawLine(int x1, int y1, int x2, int y2);
  void drawProgressBar(int x, int y, int w, int h, int percent);
  void drawDottedLine(int x1, int y1, int x2, int y2);
  void drawHeader(const char *title);

  // UI Components
  void drawSplashScreen();
  void drawStatusBar(int battery = 100, bool wifiOn = false,
                     bool bleOn = false);
  void drawMenu(const char *title, const char **items, int itemCount,
                int selected);
  void drawScrollingMenu(const char *title, const char **items, int itemCount,
                         int selected, int scrollOffset);
  void drawMenuWithIcons(const char *title, const char **items,
                         const uint8_t **icons, int itemCount, int selected);
  void drawMessage(const char *title, const char *message);
  void drawConfirm(const char *title, const char *message);
  void drawList(const char *title, const char **items, int itemCount,
                int startIdx, int selected);
  void drawProgress(const char *title, const char *message, int percent);

  // Animation
  void animateIn();
  void animateOut();

  // Screen management
  void setBrightness(uint8_t level); // 1-255
  int getStringWidth(const char *str);
  void sleep();
  void wake();
  bool isAsleep();

  // Network selection for WiFi deauth
  int showNetworkList(const char **ssids, const int *rssis, int count,
                      const char *title);

private:
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C *u8g2;
  bool sleeping;
  uint8_t brightness;

  void drawScrollBar(int total, int visible, int current);
  void truncateString(const char *source, char *dest, int maxChars);
};

// Global display instance
extern Display display;

#endif // DISPLAY_H
