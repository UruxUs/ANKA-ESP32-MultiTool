/**
 * ANKA - ESP32 Multi-Tool Firmware
 * Display Module Implementation
 */

#include "display.h"
#include "../input/buttons.h"

// Icons 8x8 bitmaps (XBM format)
const uint8_t ICON_WIFI[] = {0x00, 0x7E, 0x81, 0x3C, 0x42, 0x18, 0x24, 0x18};

const uint8_t ICON_BLE[] = {0x18, 0x28, 0x4A, 0x2C, 0x2C, 0x4A, 0x28, 0x18};

const uint8_t ICON_IR[] = {0x00, 0x18, 0x24, 0x5A, 0x5A, 0x24, 0x18, 0x00};

const uint8_t ICON_SETTINGS[] = {0x18, 0x24, 0x5A, 0xE7,
                                 0xE7, 0x5A, 0x24, 0x18};

const uint8_t ICON_INFO[] = {0x3C, 0x42, 0x99, 0x81, 0x99, 0x99, 0x42, 0x3C};

const uint8_t ICON_BACK[] = {0x08, 0x1C, 0x3E, 0x08, 0x08, 0x08, 0x08, 0x00};

const uint8_t ICON_ATTACK[] = {0x10, 0x38, 0x38, 0x7C, 0x7C, 0xFE, 0x10, 0x10};

const uint8_t ICON_SCAN[] = {0x3C, 0x42, 0x99, 0xA5, 0xA5, 0x99, 0x42, 0x3C};

Display display;

Display::Display() {
  sleeping = false;
  brightness = 255;
  u8g2 = nullptr;
}

void Display::init() {
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

  u8g2 = new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE,
                                                 OLED_SCL_PIN, OLED_SDA_PIN);
  u8g2->begin();
  u8g2->setFont(u8g2_font_6x10_tf);
  u8g2->setContrast(brightness);

  clear();
  Serial.println("[Display] Initialized");
}

void Display::clear() {
  if (u8g2) {
    u8g2->clearBuffer();
  }
}

void Display::update() {
  if (u8g2 && !sleeping) {
    u8g2->sendBuffer();
  }
}

void Display::drawString(int x, int y, const char *str) {
  if (u8g2) {
    u8g2->drawStr(x, y, str);
  }
}

void Display::drawStringCenter(int y, const char *str) {
  if (u8g2) {
    int w = u8g2->getStrWidth(str);
    u8g2->drawStr((OLED_WIDTH - w) / 2, y, str);
  }
}

// Truncate text to fit within maxChars, adding "..." if needed
void Display::truncateString(const char *source, char *dest, int maxChars) {
  int len = strlen(source);
  if (len <= maxChars) {
    strcpy(dest, source);
  } else {
    // Leave room for "..."
    strncpy(dest, source, maxChars - 3);
    dest[maxChars - 3] = '\0';
    strcat(dest, "...");
  }
}

void Display::drawIcon(int x, int y, const uint8_t *icon) {
  if (u8g2) {
    u8g2->drawXBM(x, y, 8, 8, icon);
  }
}

void Display::drawRect(int x, int y, int w, int h) {
  if (u8g2) {
    u8g2->drawFrame(x, y, w, h);
  }
}

void Display::drawFilledRect(int x, int y, int w, int h) {
  if (u8g2) {
    u8g2->drawBox(x, y, w, h);
  }
}

void Display::drawLine(int x1, int y1, int x2, int y2) {
  if (u8g2) {
    u8g2->drawLine(x1, y1, x2, y2);
  }
}

void Display::drawProgressBar(int x, int y, int w, int h, int percent) {
  if (u8g2) {
    u8g2->drawFrame(x, y, w, h);
    int fillW = (w - 2) * percent / 100;
    if (fillW > 0) {
      u8g2->drawBox(x + 1, y + 1, fillW, h - 2);
    }
  }
}

void Display::drawDottedLine(int x1, int y1, int x2, int y2) {
  if (!u8g2)
    return;

  // Simple horizontal dotted line optimization
  if (y1 == y2) {
    for (int x = x1; x < x2; x += 3) {
      u8g2->drawPixel(x, y1);
      u8g2->drawPixel(x + 1, y1);
    }
  } else {
    // Generic line (simplified)
    u8g2->drawLine(x1, y1, x2, y2);
  }
}

void Display::drawHeader(const char *title) {
  if (!u8g2)
    return;

  u8g2->setDrawColor(1);
  u8g2->setFont(u8g2_font_6x10_tf);

  // Draw title
  drawStringCenter(10, title);

  // Draw dotted separator
  drawDottedLine(0, 12, OLED_WIDTH, 12);
}

int Display::getStringWidth(const char *str) {
  if (u8g2) {
    u8g2->setFont(u8g2_font_6x10_tf);
    return u8g2->getStrWidth(str);
  }
  return 0;
}

void Display::drawSplashScreen() {
  clear();

  // Draw logo/title
  u8g2->setFont(u8g2_font_ncenB10_tr);
  drawStringCenter(25, ANKA_NAME);

  u8g2->setFont(u8g2_font_6x10_tf);
  drawStringCenter(40, "MARAUDER ED.");

  char version[20];
  snprintf(version, sizeof(version), "v%s", ANKA_VERSION);
  drawStringCenter(52, version);

  // Draw border
  drawRect(0, 0, OLED_WIDTH, OLED_HEIGHT);

  update();
}

void Display::drawStatusBar(int battery, bool wifiOn, bool bleOn) {
  // Top status bar with dotted style
  u8g2->setDrawColor(1);

  // Time/title on left
  u8g2->setFont(u8g2_font_5x7_tf);
  u8g2->drawStr(2, 8, ANKA_NAME);

  // Icons on right
  int iconX = OLED_WIDTH - 10;

  if (wifiOn) {
    u8g2->drawXBM(iconX, 1, 8, 8, ICON_WIFI);
    iconX -= 10;
  }

  if (bleOn) {
    u8g2->drawXBM(iconX, 1, 8, 8, ICON_BLE);
    iconX -= 10;
  }

  // Battery indicator
  char batStr[5];
  snprintf(batStr, sizeof(batStr), "%d%%", battery);
  u8g2->drawStr(iconX - 20, 8, batStr);

  // Dotted separator
  drawDottedLine(0, 10, OLED_WIDTH, 10);

  u8g2->setFont(u8g2_font_6x10_tf);
}

void Display::drawMenu(const char *title, const char **items, int itemCount,
                       int selected) {
  clear();

  // Title bar
  drawHeader(title);

  // Menu items
  int visibleItems = 4;
  int startIdx = 0;

  if (selected >= visibleItems) {
    startIdx = selected - visibleItems + 1;
  }

  int y = 24;
  for (int i = startIdx; i < itemCount && i < startIdx + visibleItems; i++) {
    if (i == selected) {
      // Highlight selected
      drawFilledRect(0, y - 10, OLED_WIDTH - 8, 12);
      u8g2->setDrawColor(0);
    }

    u8g2->drawStr(4, y, items[i]);
    u8g2->setDrawColor(1);

    y += 12;
  }

  // Scroll bar
  if (itemCount > visibleItems) {
    drawScrollBar(itemCount, visibleItems, selected);
  }

  update();
}

void Display::drawScrollingMenu(const char *title, const char **items,
                                int itemCount, int selected, int scrollOffset) {
  clear();

  // Title bar
  drawHeader(title);

  // Menu items
  int visibleItems = 4;
  int startIdx = 0;

  if (selected >= visibleItems) {
    startIdx = selected - visibleItems + 1;
  }

  int y = 24;
  for (int i = startIdx; i < itemCount && i < startIdx + visibleItems; i++) {
    int x = 4;

    if (i == selected) {
      // Highlight selected
      drawFilledRect(0, y - 10, OLED_WIDTH - 8, 12);
      u8g2->setDrawColor(0);

      // Apply scroll offset only to selected item
      x -= scrollOffset;
    }

    // Clip text drawing to the menu area
    // u8g2 automatically clips to screen, but for scrolling we want to be
    // careful We rely on u8g2's screen clipping. If x is negative, text starts
    // off-screen left.

    u8g2->drawStr(x, y, items[i]);
    u8g2->setDrawColor(1);

    y += 12;
  }

  // Scroll bar
  if (itemCount > visibleItems) {
    drawScrollBar(itemCount, visibleItems, selected);
  }

  update();
}

void Display::drawMenuWithIcons(const char *title, const char **items,
                                const uint8_t **icons, int itemCount,
                                int selected) {
  clear();

  // Title bar
  drawHeader(title);

  // Menu items with icons
  int visibleItems = 4;
  int startIdx = 0;

  if (selected >= visibleItems) {
    startIdx = selected - visibleItems + 1;
  }

  int y = 24;
  char truncatedText[20]; // Buffer for truncated text

  for (int i = startIdx; i < itemCount && i < startIdx + visibleItems; i++) {
    if (i == selected) {
      drawFilledRect(0, y - 10, OLED_WIDTH - 8, 12);
      u8g2->setDrawColor(0);
    }

    // Draw icon
    if (icons && icons[i]) {
      u8g2->drawXBM(4, y - 8, 8, 8, icons[i]);
    }

    // Draw text (truncated to 16 chars to prevent overflow)
    truncateString(items[i], truncatedText, 16);
    u8g2->drawStr(16, y, truncatedText);
    u8g2->setDrawColor(1);

    y += 12;
  }

  if (itemCount > visibleItems) {
    drawScrollBar(itemCount, visibleItems, selected);
  }

  update();
}

void Display::drawMessage(const char *title, const char *message) {
  clear();

  // Title
  drawHeader(title);

  // Message (centered)
  u8g2->setFont(u8g2_font_6x10_tf);
  drawStringCenter(38, message);

  // Border
  drawRect(0, 14, OLED_WIDTH, OLED_HEIGHT - 14);

  update();
}

void Display::drawConfirm(const char *title, const char *message) {
  drawMessage(title, message);

  // Add button hints
  u8g2->setFont(u8g2_font_5x7_tf);
  u8g2->drawStr(10, 60, "[SEL] OK");
  u8g2->drawStr(70, 60, "[BACK] Cancel");

  update();
}

void Display::drawList(const char *title, const char **items, int itemCount,
                       int startIdx, int selected) {
  drawMenu(title, items, itemCount, selected);
}

void Display::drawProgress(const char *title, const char *message,
                           int percent) {
  clear();

  // Title
  drawHeader(title);

  // Message
  drawStringCenter(32, message);

  // Progress bar
  drawProgressBar(10, 42, OLED_WIDTH - 20, 12, percent);

  // Percentage text
  char pctStr[5];
  snprintf(pctStr, sizeof(pctStr), "%d%%", percent);
  drawStringCenter(60, pctStr);

  update();
}

void Display::drawScrollBar(int total, int visible, int current) {
  int barHeight = OLED_HEIGHT - 14;
  int thumbHeight = (barHeight * visible) / total;
  if (thumbHeight < 4)
    thumbHeight = 4;

  int thumbY = 14 + ((barHeight - thumbHeight) * current) / (total - 1);

  // Track
  drawRect(OLED_WIDTH - 4, 14, 4, barHeight);

  // Thumb
  drawFilledRect(OLED_WIDTH - 3, thumbY, 2, thumbHeight);
}

void Display::animateIn() {
  // Simple fade in effect
  for (int i = 0; i <= 255; i += 25) {
    setBrightness(i);
    delay(20);
  }
}

void Display::animateOut() {
  // Simple fade out effect
  for (int i = 255; i >= 0; i -= 25) {
    setBrightness(i);
    delay(20);
  }
}

void Display::setBrightness(uint8_t level) {
  brightness = level;
  if (u8g2) {
    u8g2->setContrast(level);
  }
}

void Display::sleep() {
  sleeping = true;
  if (u8g2) {
    u8g2->setPowerSave(1);
  }
}

void Display::wake() {
  sleeping = false;
  if (u8g2) {
    u8g2->setPowerSave(0);
  }
}

bool Display::isAsleep() { return sleeping; }

// Network selection UI for WiFi deauth target selection
int Display::showNetworkList(const char **ssids, const int *rssis, int count,
                             const char *title) {
  if (!u8g2 || count == 0)
    return -1;

  int selected = 0;
  int scrollOffset = 0;
  const int maxVisible = 4; // Show 4 networks at a time
  unsigned long lastScrollUpdate = 0;
  int textScrollPos = 0; // Horizontal scroll position for selected item

  while (true) {
    u8g2->clearBuffer();

    // Title bar
    drawHeader(title);

    // Network list
    for (int i = 0; i < maxVisible && (i + scrollOffset) < count; i++) {
      int idx = i + scrollOffset;
      int y = 24 + (i * 12);

      // Selection indicator
      if (idx == selected) {
        u8g2->drawStr(0, y, ">");

        // Scrolling text for selected item
        char fullText[32];
        snprintf(fullText, sizeof(fullText), "%s %ddB", ssids[idx], rssis[idx]);

        int textWidth = u8g2->getStrWidth(fullText);
        int displayWidth = 110; // Available width for text

        if (textWidth > displayWidth) {
          // Text needs scrolling
          unsigned long now = millis();
          if (now - lastScrollUpdate >
              33) { // Scroll every 33ms (30 fps) - 3x faster
            textScrollPos++;
            if (textScrollPos > textWidth - displayWidth + 20) {
              textScrollPos = 0; // Reset scroll
            }
            lastScrollUpdate = now;
          }

          // Draw scrolling text
          u8g2->setClipWindow(10, 0, 120, 64);
          u8g2->drawStr(10 - textScrollPos, y, fullText);
          u8g2->setMaxClipWindow();
        } else {
          // Text fits, no scroll needed
          u8g2->drawStr(10, y, fullText);
          textScrollPos = 0; // Reset scroll position
        }
      } else {
        // Non-selected items: truncate
        char line[24];
        snprintf(line, sizeof(line), "%-12.12s %ddB", ssids[idx], rssis[idx]);
        u8g2->drawStr(10, y, line);
      }
    }

    // Scroll indicator
    if (count > maxVisible) {
      int barHeight = (maxVisible * 48) / count;
      int barPos = (scrollOffset * (48 - barHeight)) / (count - maxVisible);
      u8g2->drawFrame(126, 16, 2, 48);
      u8g2->drawBox(126, 16 + barPos, 2, barHeight);
    }

    u8g2->sendBuffer();

    // Handle input
    buttons.update();
    ButtonEvent evt = buttons.getEvent();

    if (evt == BTN_UP_PRESS && selected > 0) {
      selected--;
      textScrollPos = 0; // Reset scroll on selection change
      if (selected < scrollOffset) {
        scrollOffset--;
      }
    } else if (evt == BTN_DOWN_PRESS && selected < count - 1) {
      selected++;
      textScrollPos = 0; // Reset scroll on selection change
      if (selected >= scrollOffset + maxVisible) {
        scrollOffset++;
      }
    } else if (evt == BTN_SELECT_PRESS) {
      return selected; // Return selected index
    } else if (evt == BTN_BACK_PRESS) {
      return -1; // Cancelled
    }

    delay(50);
  }
}
