/**
 * ANKA - ESP32 Multi-Tool Firmware
 * Button Input Handler Implementation
 */

#include "buttons.h"

Buttons buttons;

Buttons::Buttons() {
  pendingEvent = BTN_NONE;

  // Initialize button states
  buttons[0] = {BTN_UP_PIN, true, true, 0, false};
  buttons[1] = {BTN_DOWN_PIN, true, true, 0, false};
  buttons[2] = {BTN_SELECT_PIN, true, true, 0, false};
  buttons[3] = {BTN_BACK_PIN, true, true, 0, false};
}

void Buttons::init() {
  pinMode(BTN_UP_PIN, INPUT_PULLUP);
  pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
  pinMode(BTN_SELECT_PIN, INPUT_PULLUP);
  pinMode(BTN_BACK_PIN, INPUT_PULLUP);

  Serial.println("[Buttons] Initialized");
}

void Buttons::update() {
  pendingEvent = BTN_NONE;

  for (int i = 0; i < 4; i++) {
    checkButton(i);
  }
}

void Buttons::checkButton(int idx) {
  bool reading = digitalRead(buttons[idx].pin) == LOW;

  if (reading != buttons[idx].lastState) {
    buttons[idx].lastState = reading;

    if (reading) {
      // Button pressed
      buttons[idx].pressTime = millis();
      buttons[idx].longPressFired = false;
      Serial.printf("[BTN] Button %d (pin %d) PRESSED\n", idx,
                    buttons[idx].pin);

      // Reset Screen Saver Timer
      extern unsigned long lastActivityTime;
      lastActivityTime = millis();
    } else {
      // Button released
      Serial.printf("[BTN] Button %d (pin %d) RELEASED\n", idx,
                    buttons[idx].pin);
      if (!buttons[idx].longPressFired) {
        // Fire short press event
        switch (idx) {
        case 0:
          pendingEvent = BTN_UP_PRESS;
          Serial.println("[BTN] Event: UP_PRESS");
          break;
        case 1:
          pendingEvent = BTN_DOWN_PRESS;
          Serial.println("[BTN] Event: DOWN_PRESS");
          break;
        case 2:
          pendingEvent = BTN_SELECT_PRESS;
          Serial.println("[BTN] Event: SELECT_PRESS");
          break;
        case 3:
          pendingEvent = BTN_BACK_PRESS;
          Serial.println("[BTN] Event: BACK_PRESS");
          break;
        }
      }
    }
  }

  // Check for long press
  if (reading && !buttons[idx].longPressFired) {
    if (millis() - buttons[idx].pressTime > LONG_PRESS_TIME) {
      buttons[idx].longPressFired = true;

      switch (idx) {
      case 0:
        pendingEvent = BTN_UP_LONG;
        break;
      case 1:
        pendingEvent = BTN_DOWN_LONG;
        break;
      case 2:
        pendingEvent = BTN_SELECT_LONG;
        break;
      case 3:
        pendingEvent = BTN_BACK_LONG;
        break;
      }
    }
  }

  buttons[idx].currentState = reading;
}

ButtonEvent Buttons::getEvent() {
  ButtonEvent evt = pendingEvent;
  pendingEvent = BTN_NONE;
  return evt;
}

bool Buttons::isPressed(int pin) { return digitalRead(pin) == LOW; }

bool Buttons::isHeld(int pin) {
  for (int i = 0; i < 4; i++) {
    if (buttons[i].pin == pin) {
      return buttons[i].currentState &&
             (millis() - buttons[i].pressTime > LONG_PRESS_TIME);
    }
  }
  return false;
}

bool Buttons::upPressed() {
  return buttons[0].currentState && !buttons[0].lastState;
}

bool Buttons::downPressed() {
  return buttons[1].currentState && !buttons[1].lastState;
}

bool Buttons::selectPressed() {
  return buttons[2].currentState && !buttons[2].lastState;
}

bool Buttons::backPressed() {
  return buttons[3].currentState && !buttons[3].lastState;
}

bool Buttons::anyPressed() {
  for (int i = 0; i < 4; i++) {
    if (buttons[i].currentState)
      return true;
  }
  return false;
}

void Buttons::waitForRelease() {
  while (anyPressed()) {
    delay(10);
  }
  delay(DEBOUNCE_DELAY);
}
