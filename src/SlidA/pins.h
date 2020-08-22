/*
 * thinithm pin definitions
 */

#pragma once

// SDA and SCL use the default hw pins
#define PIN_SLIDER_IRQ 4

#define SLIDER_LEDS true

#if SLIDER_LEDS
#define PIN_SLIDER_LED 10
#endif


// this arduino can handle keyboard button input if set to true,
// but using a separate encoder would be better
#define BUTTON_INPUT true

#if BUTTON_INPUT
struct kbInput {
  const char key;
  const int pin;
  bool lastState;
};

kbInput kbButtons[] = {
  {'w', 5, false},
  {'a', 6, false},
  {'s', 7, false},
  {'d', 8, false},
  {(char)0xB0, 9, false}, // B0: KEY_RETURN
};
#endif // BUTTON_INPUT
