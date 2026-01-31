#include <stdint.h>

/**
 * @brief Override ESP-IDF's ieee80211_raw_frame_sanity_check
 *
 * This function MUST return 0 to bypass ESP-IDF's frame validation.
 * Compiled as a separate C file to ensure linker picks our version with
 * -zmuldefs.
 *
 * Based on esp32-wifi-penetration-tool WSL Bypasser implementation.
 */
int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
  return 0; // Always allow frame transmission
}
