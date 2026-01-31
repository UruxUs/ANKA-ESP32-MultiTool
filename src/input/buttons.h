/**
 * ANKA - ESP32 Multi-Tool Firmware
 * Button Input Handler Header
 */

#ifndef BUTTONS_H
#define BUTTONS_H

#include "../config.h"
#include <Arduino.h>

enum ButtonEvent {
  BTN_NONE = 0,
  BTN_UP_PRESS,
  BTN_DOWN_PRESS,
  BTN_SELECT_PRESS,
  BTN_BACK_PRESS,
  BTN_UP_LONG,
  BTN_DOWN_LONG,
  BTN_SELECT_LONG,
  BTN_BACK_LONG
};

class Buttons {
public:
  Buttons();

  void init();
  void update();

  ButtonEvent getEvent();
  bool isPressed(int pin);
  bool isHeld(int pin);

  // Direct state access
  bool upPressed();
  bool downPressed();
  bool selectPressed();
  bool backPressed();

  bool anyPressed();
  void waitForRelease();

private:
  struct ButtonState {
    int pin;
    bool lastState;
    bool currentState;
    unsigned long pressTime;
    bool longPressFired;
  };

  ButtonState buttons[4];
  ButtonEvent pendingEvent;

  void checkButton(int idx);
};

// Global buttons instance
extern Buttons buttons;

#endif // BUTTONS_H
