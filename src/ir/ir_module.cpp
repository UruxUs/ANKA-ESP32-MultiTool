/**
 * ANKA - ESP32 Multi-Tool Firmware
 * IR Module Implementation
 */

#include "ir_module.h"
#include "../display/display.h"
#include "../input/buttons.h"

IRModule irModule;

// TV-B-Gone power codes - Common TV power codes (NEC protocol)
// Format: {code, bits}
const uint32_t IRModule::TV_CODES[][2] = {
    // Samsung
    {0xE0E040BF, 32},
    {0xE0E019E6, 32},
    // LG
    {0x20DF10EF, 32},
    {0x20DF08F7, 32},
    // Sony (12-bit)
    {0xA90, 12},
    {0x290, 12},
    {0x490, 12},
    // Panasonic
    {0x400401FC, 32},
    // Philips RC-5
    {0x0C, 12},
    // Toshiba
    {0x02FD48B7, 32},
    // Sharp
    {0x45A, 15},
    // Vizio
    {0x20DF10EF, 32},
    // TCL/Roku
    {0x57E3E817, 32},
    // Hisense
    {0x20DF10EF, 32},
    // Sanyo
    {0x1CE3, 15},
    // JVC
    {0xC5E8, 16},
    // Hitachi
    {0x0DF308F7, 32},
    // Emerson
    {0xF708FB04, 32},
    // RCA
    {0x0FF00FF0, 32},
    // Magnavox
    {0x20DF10EF, 32},
    // ===== TURKISH TV BRANDS =====
    // Vestel (Turkish brand - comprehensive codes)
    {0x00FF00FF, 32}, // Vestel power 1
    {0x807F00FF, 32}, // Vestel power 2
    {0x40BF00FF, 32}, // Vestel power 3
    {0x00FFA857, 32}, // Vestel RC3920
    {0x20DF10EF, 32}, // Vestel common (LG-like)
    {0xC03F00FF, 32}, // Vestel variant 1
    {0x00FF48B7, 32}, // Vestel variant 2
    {0x00FF7887, 32}, // Vestel variant 3
    {0x20DF23DC, 32}, // Vestel alternate
    {0x00FFC03F, 32}, // Vestel RC1910
    // Axen (Turkish brand - comprehensive codes)
    {0x00FF02FD, 32}, // Axen power 1
    {0x807F02FD, 32}, // Axen power 2
    {0x00FFA25D, 32}, // Axen common
    {0x00FF629D, 32}, // Axen alt
    {0x20DF02FD, 32}, // Axen variant
    {0x40BF02FD, 32}, // Axen variant 2
    {0x00FF827D, 32}, // Axen variant 3
    {0x00FFE21D, 32}, // Axen RC2000
    {0xC03F02FD, 32}, // Axen alternate
};
const int IRModule::TV_CODES_COUNT = sizeof(TV_CODES) / sizeof(TV_CODES[0]);

IRModule::IRModule() {
  irRecv = nullptr;
  irSend = nullptr;
  receiving = false;
  signalReady = false;
  tvbGoneRunning = false;
  tvbGoneProgress = 0;
  signalCount = 0;
  tvbGoneTaskHandle = nullptr;
}

IRModule::~IRModule() { deinit(); }

void IRModule::init() {
  irRecv = new IRrecv(IR_RX_PIN, IR_BUFFER_SIZE, 50, true);
  irSend = new IRsend(IR_TX_PIN);

  irSend->begin();

  // Initialize SPIFFS for persistent storage
  if (!SPIFFS.begin(true)) {
    Serial.println("[IR] SPIFFS mount failed!");
  } else {
    Serial.println("[IR] SPIFFS mounted");

    // Create IR directory if not exists
    if (!SPIFFS.exists(IR_STORAGE_DIR)) {
      // SPIFFS doesn't need mkdir, files create paths
    }

    // Load signal count from index
    loadSignalIndex();
  }

  Serial.println("[IR] Initialized");
}

void IRModule::deinit() {
  stopReceive();
  stopTvBGone();

  if (irRecv) {
    delete irRecv;
    irRecv = nullptr;
  }
  if (irSend) {
    delete irSend;
    irSend = nullptr;
  }
}

bool IRModule::startReceive() {
  if (!irRecv)
    return false;

  irRecv->enableIRIn();
  receiving = true;
  signalReady = false;

  Serial.println("[IR] Receiving started");
  return true;
}

void IRModule::stopReceive() {
  if (!irRecv)
    return;

  irRecv->disableIRIn();
  receiving = false;
}

bool IRModule::isReceiving() { return receiving; }

bool IRModule::hasLastSignal() {
  return (lastSignal.protocol != UNKNOWN || lastSignal.isRaw);
}

bool IRModule::hasSignal() {
  if (!irRecv || !receiving)
    return false;

  if (irRecv->decode(&results)) {
    // Save to lastReceivedSignal BEFORE resuming (which clears the buffer)
    lastSignal.protocol = results.decode_type;
    lastSignal.value = results.value;
    lastSignal.bits = results.bits;
    lastSignal.isRaw = (results.decode_type == UNKNOWN);

    // Deep copy raw data if needed
    if (lastSignal.rawData) {
      delete[] lastSignal.rawData;
      lastSignal.rawData = nullptr;
    }

    if (lastSignal.isRaw && results.rawlen > 0) {
      lastSignal.rawLen = results.rawlen - 1;
      lastSignal.rawData = new uint16_t[lastSignal.rawLen];
      for (int i = 0; i < lastSignal.rawLen; i++) {
        lastSignal.rawData[i] = results.rawbuf[i + 1] * kRawTick;
      }
    } else {
      lastSignal.rawLen = 0;
    }

    // Copy name
    snprintf(lastSignal.name, sizeof(lastSignal.name), "Signal %d",
             signalCount + 1);

    signalReady = true;
    irRecv->resume();
    return true;
  }
  return false;
}

bool IRModule::readSignal(IRSignal *signal) {
  if (!signalReady)
    return false;

  // Copy from persistent lastSignal
  signal->protocol = lastSignal.protocol;
  signal->value = lastSignal.value;
  signal->bits = lastSignal.bits;
  signal->isRaw = lastSignal.isRaw;
  signal->rawLen = lastSignal.rawLen;

  strncpy(signal->name, lastSignal.name, sizeof(signal->name));

  if (signal->isRaw && lastSignal.rawData) {
    signal->rawData = new uint16_t[signal->rawLen];
    memcpy(signal->rawData, lastSignal.rawData,
           signal->rawLen * sizeof(uint16_t));
  } else {
    signal->rawData = nullptr;
  }

  return true;
}

String IRModule::getLastProtocol() { return typeToString(lastSignal.protocol); }

String IRModule::getLastValue() {
  char buf[20];
  snprintf(buf, sizeof(buf), "0x%llX", lastSignal.value);
  return String(buf);
}

void IRModule::sendSignal(IRSignal *signal) {
  if (!irSend)
    return;

  if (signal->isRaw && signal->rawData) {
    sendRaw(signal->rawData, signal->rawLen);
  } else {
    switch (signal->protocol) {
    case NEC:
      sendNEC(signal->value, signal->bits);
      break;
    case SAMSUNG:
      sendSamsung(signal->value, signal->bits);
      break;
    case SONY:
      sendSony(signal->value, signal->bits);
      break;
    case RC5:
      sendRC5(signal->value, signal->bits);
      break;
    case RC6:
      sendRC6(signal->value, signal->bits);
      break;
    default:
      // Try NEC as fallback
      sendNEC(signal->value, signal->bits);
      break;
    }
  }

  Serial.printf("[IR] Sent signal: %s 0x%llX\n",
                typeToString(signal->protocol).c_str(), signal->value);
}

void IRModule::sendNEC(uint64_t data, uint16_t bits) {
  if (irSend) {
    irSend->sendNEC(data, bits);
  }
}

void IRModule::sendSamsung(uint64_t data, uint16_t bits) {
  if (irSend) {
    irSend->sendSAMSUNG(data, bits);
  }
}

void IRModule::sendSony(uint64_t data, uint16_t bits) {
  if (irSend) {
    irSend->sendSony(data, bits);
  }
}

void IRModule::sendRC5(uint64_t data, uint16_t bits) {
  if (irSend) {
    irSend->sendRC5(data, bits);
  }
}

void IRModule::sendRC6(uint64_t data, uint16_t bits) {
  if (irSend) {
    irSend->sendRC6(data, bits);
  }
}

void IRModule::sendRaw(uint16_t *data, uint16_t len, uint16_t freq) {
  if (irSend) {
    irSend->sendRaw(data, len, freq);
  }
}

void IRModule::tvbGoneTask(void *param) {
  IRModule *self = (IRModule *)param;

  for (int i = 0; i < TV_CODES_COUNT && self->tvbGoneRunning; i++) {
    uint32_t code = TV_CODES[i][0];
    uint16_t bits = TV_CODES[i][1];

    // Send as NEC (most common)
    self->irSend->sendNEC(code, bits);
    delay(50);

    // Also try Samsung protocol
    self->irSend->sendSAMSUNG(code, bits);
    delay(50);

    self->tvbGoneProgress = ((i + 1) * 100) / TV_CODES_COUNT;

    vTaskDelay(pdMS_TO_TICKS(100));
  }

  self->tvbGoneRunning = false;
  self->tvbGoneProgress = 100;

  vTaskDelete(NULL);
}

void IRModule::startTvBGone() {
  if (tvbGoneRunning)
    return;

  tvbGoneRunning = true;
  tvbGoneProgress = 0;

  xTaskCreatePinnedToCore(tvbGoneTask, "tvbgone", 4096, this, 1,
                          &tvbGoneTaskHandle, 1);

  Serial.println("[IR] TV-B-Gone started");
}

void IRModule::stopTvBGone() {
  if (!tvbGoneRunning)
    return;

  tvbGoneRunning = false;

  if (tvbGoneTaskHandle) {
    vTaskDelay(pdMS_TO_TICKS(200));
    tvbGoneTaskHandle = nullptr;
  }

  Serial.println("[IR] TV-B-Gone stopped");
}

bool IRModule::isTvBGoneRunning() { return tvbGoneRunning; }

int IRModule::getTvBGoneProgress() { return tvbGoneProgress; }

bool IRModule::saveSignal(const char *name, IRSignal *signal) {
  if (signalCount >= MAX_IR_SIGNALS)
    return false;

  // Store in RAM
  strncpy(storedSignals[signalCount].name, name, IR_SIGNAL_NAME_LEN - 1);
  storedSignals[signalCount].name[IR_SIGNAL_NAME_LEN - 1] = '\0';
  storedSignals[signalCount].protocol = signal->protocol;
  storedSignals[signalCount].value = signal->value;
  storedSignals[signalCount].bits = signal->bits;
  storedSignals[signalCount].isRaw = signal->isRaw;

  if (signal->isRaw && signal->rawData && signal->rawLen > 0) {
    storedSignals[signalCount].rawLen = signal->rawLen;
    storedSignals[signalCount].rawData = new uint16_t[signal->rawLen];
    memcpy(storedSignals[signalCount].rawData, signal->rawData,
           signal->rawLen * sizeof(uint16_t));
  } else {
    storedSignals[signalCount].rawData = nullptr;
    storedSignals[signalCount].rawLen = 0;
  }

  // Save to SPIFFS
  char filename[32];
  snprintf(filename, sizeof(filename), "%s/%d.json", IR_STORAGE_DIR,
           signalCount);

  File file = SPIFFS.open(filename, "w");
  if (file) {
    StaticJsonDocument<512> doc;
    doc["name"] = name;
    doc["protocol"] = (int)signal->protocol;
    doc["value"] = signal->value;
    doc["bits"] = signal->bits;
    doc["isRaw"] = signal->isRaw;
    doc["rawLen"] = signal->rawLen;

    if (signal->isRaw && signal->rawData) {
      JsonArray rawArr = doc.createNestedArray("raw");
      for (int i = 0; i < signal->rawLen && i < 200; i++) {
        rawArr.add(signal->rawData[i]);
      }
    }

    serializeJson(doc, file);
    file.close();
    Serial.printf("[IR] Saved signal %d to flash\n", signalCount);
  }

  signalCount++;
  saveSignalIndex();

  return true;
}

bool IRModule::saveSignalAtIndex(int index, const char *name,
                                 IRSignal *signal) {
  if (index < 0 || index >= MAX_IR_SIGNALS)
    return false;

  // Update RAM
  strncpy(storedSignals[index].name, name, IR_SIGNAL_NAME_LEN - 1);
  storedSignals[index].name[IR_SIGNAL_NAME_LEN - 1] = '\0';
  storedSignals[index].protocol = signal->protocol;
  storedSignals[index].value = signal->value;
  storedSignals[index].bits = signal->bits;
  storedSignals[index].isRaw = signal->isRaw;

  // Free old raw data
  if (storedSignals[index].rawData) {
    delete[] storedSignals[index].rawData;
    storedSignals[index].rawData = nullptr;
  }

  // Copy new raw data
  if (signal->isRaw && signal->rawData && signal->rawLen > 0) {
    storedSignals[index].rawLen = signal->rawLen;
    storedSignals[index].rawData = new uint16_t[signal->rawLen];
    memcpy(storedSignals[index].rawData, signal->rawData,
           signal->rawLen * sizeof(uint16_t));
  } else {
    storedSignals[index].rawLen = 0;
  }

  // Save to SPIFFS
  char filename[32];
  snprintf(filename, sizeof(filename), "%s/%d.json", IR_STORAGE_DIR, index);

  File file = SPIFFS.open(filename, "w");
  if (file) {
    StaticJsonDocument<512> doc;
    doc["name"] = name;
    doc["protocol"] = (int)signal->protocol;
    doc["value"] = signal->value;
    doc["bits"] = signal->bits;
    doc["isRaw"] = signal->isRaw;
    doc["rawLen"] = signal->rawLen;

    if (signal->isRaw && signal->rawData) {
      JsonArray rawArr = doc.createNestedArray("raw");
      for (int i = 0; i < signal->rawLen && i < 200; i++) {
        rawArr.add(signal->rawData[i]);
      }
    }

    serializeJson(doc, file);
    file.close();
    Serial.printf("[IR] Overwrote signal %d\n", index);
  }

  return true;
}

bool IRModule::loadSignal(int index, IRSignal *signal) {
  if (index < 0 || index >= signalCount)
    return false;

  // Try loading from SPIFFS first
  char filename[32];
  snprintf(filename, sizeof(filename), "%s/%d.json", IR_STORAGE_DIR, index);

  File file = SPIFFS.open(filename, "r");
  if (file) {
    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, file);
    file.close();

    if (!err) {
      strncpy(signal->name, doc["name"] | "Signal", IR_SIGNAL_NAME_LEN - 1);
      signal->protocol = (decode_type_t)(int)doc["protocol"];
      signal->value = doc["value"];
      signal->bits = doc["bits"];
      signal->isRaw = doc["isRaw"];
      signal->rawLen = doc["rawLen"];

      if (signal->isRaw && signal->rawLen > 0) {
        signal->rawData = new uint16_t[signal->rawLen];
        JsonArray rawArr = doc["raw"];
        int i = 0;
        for (JsonVariant v : rawArr) {
          if (i < signal->rawLen) {
            signal->rawData[i++] = v.as<uint16_t>();
          }
        }
      } else {
        signal->rawData = nullptr;
      }

      return true;
    }
  }

  // Fall back to RAM
  *signal = storedSignals[index];
  return true;
}

int IRModule::getSignalCount() { return signalCount; }

bool IRModule::deleteSignal(int index) {
  if (index < 0 || index >= signalCount) {
    return false;
  }

  // Free raw data if exists
  if (storedSignals[index].rawData) {
    delete[] storedSignals[index].rawData;
  }

  // Delete file from SPIFFS
  char filename[32];
  snprintf(filename, sizeof(filename), "%s/%d.json", IR_STORAGE_DIR, index);
  SPIFFS.remove(filename);

  // Shift remaining signals down
  for (int i = index; i < signalCount - 1; i++) {
    storedSignals[i] = storedSignals[i + 1];

    // Rename files in SPIFFS
    char oldName[32], newName[32];
    snprintf(oldName, sizeof(oldName), "%s/%d.json", IR_STORAGE_DIR, i + 1);
    snprintf(newName, sizeof(newName), "%s/%d.json", IR_STORAGE_DIR, i);

    // Read and rewrite with new index
    File oldFile = SPIFFS.open(oldName, "r");
    if (oldFile) {
      size_t size = oldFile.size();
      char *buf = new char[size];
      oldFile.readBytes(buf, size);
      oldFile.close();

      File newFile = SPIFFS.open(newName, "w");
      if (newFile) {
        newFile.write((uint8_t *)buf, size);
        newFile.close();
      }
      delete[] buf;

      SPIFFS.remove(oldName);
    }
  }

  signalCount--;
  saveSignalIndex();

  return true;
}

void IRModule::clearSignals() {
  // Delete files from SPIFFS
  for (int i = 0; i < signalCount; i++) {
    char filename[32];
    snprintf(filename, sizeof(filename), "%s/%d.json", IR_STORAGE_DIR, i);
    SPIFFS.remove(filename);

    if (storedSignals[i].rawData) {
      delete[] storedSignals[i].rawData;
      storedSignals[i].rawData = nullptr;
    }
  }
  signalCount = 0;
  saveSignalIndex();

  Serial.println("[IR] All signals cleared");
}

void IRModule::loadSignalIndex() {
  File file = SPIFFS.open(IR_INDEX_FILE, "r");
  if (file) {
    StaticJsonDocument<64> doc;
    DeserializationError err = deserializeJson(doc, file);
    file.close();

    if (!err) {
      signalCount = doc["count"] | 0;
      Serial.printf("[IR] Loaded %d signals from flash\n", signalCount);
    }
  }
}

void IRModule::saveSignalIndex() {
  File file = SPIFFS.open(IR_INDEX_FILE, "w");
  if (file) {
    StaticJsonDocument<64> doc;
    doc["count"] = signalCount;
    serializeJson(doc, file);
    file.close();
  }
}
