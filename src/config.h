/**
 * ANKA - ESP32 Multi-Tool Firmware
 * Configuration Header
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Version Info
#define ANKA_VERSION "2.0.0"
#define ANKA_NAME "ANKA"

// OLED Display Pins (SSD1306 I2C 128x64)
#define OLED_SDA_PIN 22
#define OLED_SCL_PIN 21
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDRESS 0x3C

// Button Pins (Active LOW)
#define BTN_UP_PIN 33
#define BTN_DOWN_PIN 32
#define BTN_SELECT_PIN 23
#define BTN_BACK_PIN 18

// Button timing
#define DEBOUNCE_DELAY 50   // ms
#define LONG_PRESS_TIME 800 // ms

// IR Pins
#define IR_TX_PIN 4
#define IR_RX_PIN 15

// Status LED
#define LED_PIN 2

// System Settings
#define SERIAL_BAUD 115200
#define MENU_TIMEOUT 30000   // ms
#define SCREEN_TIMEOUT 60000 // ms

// WiFi Settings
#define MAX_SCAN_RESULTS 20
#define DEAUTH_BURST_COUNT 5
#define BEACON_INTERVAL_MS 100
#define MAX_SSID_LENGTH 32

// BLE Settings
#define BLE_SCAN_TIME 5       // seconds
#define BLE_SPAM_INTERVAL 250 // ms

// IR Settings
#define IR_RECEIVE_TIMEOUT 15 // seconds
#define IR_BUFFER_SIZE 1024

// Feature Flags
#define ENABLE_WIFI 1
#define ENABLE_BLE 1
#define ENABLE_IR 1
#define ENABLE_SERIAL_CLI 1

#endif // CONFIG_H
