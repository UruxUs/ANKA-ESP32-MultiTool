/**
 * ANKA - ESP32 Multi-Tool Firmware
 * IR Module Header
 */

#ifndef IR_MODULE_H
#define IR_MODULE_H

#include "../config.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <SPIFFS.h>

#define MAX_IR_SIGNALS 10
#define IR_SIGNAL_NAME_LEN 16
#define IR_STORAGE_DIR "/ir"
#define IR_INDEX_FILE "/ir/index.json"

struct IRSignal {
  char name[IR_SIGNAL_NAME_LEN];
  decode_type_t protocol;
  uint64_t value;
  uint16_t bits;
  uint16_t *rawData;
  uint16_t rawLen;
  bool isRaw;
};

class IRModule {
public:
  IRModule();
  ~IRModule();

  void init();
  void deinit();

  // Receiving
  bool startReceive();
  void stopReceive();
  bool isReceiving();
  bool hasSignal();
  bool hasLastSignal();
  bool readSignal(IRSignal *signal);
  String getLastProtocol();
  String getLastValue();

  // Transmitting
  void sendSignal(IRSignal *signal);
  void sendNEC(uint64_t data, uint16_t bits = 32);
  void sendSamsung(uint64_t data, uint16_t bits = 32);
  void sendSony(uint64_t data, uint16_t bits = 12);
  void sendRC5(uint64_t data, uint16_t bits = 12);
  void sendRC6(uint64_t data, uint16_t bits = 20);
  void sendRaw(uint16_t *data, uint16_t len, uint16_t freq = 38);

  // TV-B-Gone
  void startTvBGone();
  void stopTvBGone();
  bool isTvBGoneRunning();
  int getTvBGoneProgress();

  // Signal storage
  bool saveSignal(const char *name, IRSignal *signal);
  bool saveSignalAtIndex(int index, const char *name, IRSignal *signal);
  bool loadSignal(int index, IRSignal *signal);
  bool deleteSignal(int index);
  int getSignalCount();
  void clearSignals();

private:
  IRrecv *irRecv;
  IRsend *irSend;
  decode_results results;

  bool receiving;
  bool signalReady;
  bool tvbGoneRunning;
  int tvbGoneProgress;

  IRSignal storedSignals[MAX_IR_SIGNALS];
  IRSignal lastSignal; // Persistent buffer for last received signal
  int signalCount;

  TaskHandle_t tvbGoneTaskHandle;

  static void tvbGoneTask(void *param);

  // SPIFFS helpers
  void loadSignalIndex();
  void saveSignalIndex();

  // TV-B-Gone codes
  static const uint32_t TV_CODES[][2];
  static const int TV_CODES_COUNT;
};

// Global IR module instance
extern IRModule irModule;

#endif // IR_MODULE_H
