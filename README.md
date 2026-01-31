# ANKA

ANKA is a comprehensive security tool for **ESP32 (DevKit V1)** combined with a **0.96" SSD1306 OLED (I2C)** display. It combines WiFi Deauthentication, BLE Spamming, and IR Remote testing into one firmware. 

- Inspired by the [ESP32 Marauder](https://github.com/justcallmekoko/ESP32Marauder) project.
- WiFi bypass technique adapted from [esp32-wifi-penetration-tool](https://github.com/risinek/esp32-wifi-penetration-tool).
- Developed with the assistance of AI.

## Features

*   **WiFi**: Scanner, Deauth (with bypass), Beacon Spam
*   **BLE**: Spam payloads for iOS, Samsung, Windows, Android
*   **IR Receiver**: `GPIO 15`
*   **Note**: NRF24 modules are **not** supported in this firmware.

## Configuration

You can customize the pin definitions and system settings (like timeouts or enabling/disabling modules) in:
`src/config.h`

If your wiring differs from the defaults above, edit this file before building.

## Hardware

Designed for **ESP32 DevKit V1** with a **0.96" OLED (I2C)**.

*   **OLED**: SDA (22), SCL (21)
*   **Buttons**: Up (33), Down (32), Select (23), Back (18)
*   **IR**: TX (4), RX (15)

## Installation

This project is structured for **PlatformIO**.

1.  Clone repo.
2.  Open in VS Code (with PlatformIO extension).
3.  Upload: `pio run -t upload`

> **Note:** If you get `unsupport frame type: 0xc0` errors while deauthing, run a clean build (`pio run -t clean`) to ensure the bypass file links correctly.

## Disclaimer

**For educational purposes only.** Use this only on networks/devices you own or have permission to test.
