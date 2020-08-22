/*
 * slider board type defs for thinithm (divaslider)
 * 
 * if adding more in the future it'd probably be a good idea to use PROGMEM,
 * but for now it's not really worth bothering
 */

#pragma once
#include <Arduino.h>
#include "segaSlider.h"

#define SLIDER_BOARDS_MAX_KEYS 32
#define SLIDER_BOARDS_MAX_LEDS 32

struct sliderDef {
  // number of keys in the protocol
  const byte keyCount;
  
  // keyMap is indexed by the protocol output key number
  // it stores the raw hw key numbers that should affect the output
  //   note: hw numbers assume 48 inputs
  // allow two raw keys per output to support merging rows
  const byte keyMap[SLIDER_BOARDS_MAX_KEYS];
  
  // number of LEDs in the protocol
  const byte ledCount;
  
  // ledMap is indexed by the protocol input LED number
  // it stores the raw hw LED number that should take the value
  //   assume 32 hw leds
  // blending isn't supported
  const byte ledMap[SLIDER_BOARDS_MAX_LEDS];

  // board and chip numbers used in the sega protocol
  const char model[sizeof(boardInfo::model)];
  const char chipNumber[sizeof(boardInfo::chipNumber)];
};

const sliderDef divaSlider =
{
  32,
  {
    0+0, 0+1, 0+2, 0+3, 0+4, 0+5, 0+6, 0+7, 0+8, 0+9, 0+10, 0+11,
    1+0, 1+1, 1+2, 1+3, 1+4, 1+5, 1+6, 1+7, 1+8, 1+9, 1+10, 1+11,
    2+0, 2+1, 2+2, 2+3, 2+4, 2+5, 2+6, 2+7
  },
  32,
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 },
  { '1', '5', '2', '7', '5', ' ', ' ', ' ' },
  { '0', '6', '6', '8', '7' }
};
