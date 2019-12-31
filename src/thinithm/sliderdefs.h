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
#define SLIDER_BOARDS_INPUT_KEYS_PER_OUTPUT 2

struct sliderDef {
  // number of keys in the protocol
  const byte keyCount;
  
  // keyMap is indexed by the protocol output key number
  // it stores the raw hw key numbers that should affect the output
  //   note: hw numbers assume 48 inputs
  // allow two raw keys per output to support merging rows
  const byte keyMap[SLIDER_BOARDS_MAX_KEYS][SLIDER_BOARDS_INPUT_KEYS_PER_OUTPUT];
  
  // number of LEDs in the protocol
  const byte ledCount;
  
  // ledMap is indexed by the protocol input LED number
  // it stores the raw hw LED number that should take the value
  //   assume 32 hw leds
  // blending isn't supported
  const byte ledMap[SLIDER_BOARDS_MAX_LEDS];

  // whether or not air sensors should be used
  const bool hasAir;

  // board and chip numbers used in the sega protocol
  const char model[sizeof(boardInfo::model)];
  const char chipNumber[sizeof(boardInfo::chipNumber)];
};

const sliderDef divaSlider =
{
  32,
  {
    {0+11,0+10}, {0+11,0+10},  {0+7,0+8}, {0+7,0+8},  {0+4,0+3}, {0+4,0+3},  {0+0,0+1}, {0+0,0+1},
    {12+11,12+10}, {12+11,12+10},  {12+7,12+8}, {12+7,12+8},  {12+4,12+3}, {12+4,12+3},  {12+0,12+1}, {12+0,12+1},
    {24+11,24+10}, {24+11,24+10},  {24+7,24+8}, {24+7,24+8},  {24+4,24+3}, {24+4,24+3},  {24+0,24+1}, {24+0,24+1},
    {36+11,36+10}, {36+11,36+10},  {36+7,36+8}, {36+7,36+8},  {36+4,36+3}, {36+4,36+3},  {36+0,36+1}, {36+0,36+1}
  },
  32,
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 },
  false,
  { '1', '5', '2', '7', '5', ' ', ' ', ' ' },
  { '0', '6', '6', '8', '7' }
};

const sliderDef chuniSlider =
{
  32,
  {
    {36+0,36+0}, {36+1,36+1},  {36+4,36+4}, {36+3,36+3},  {36+7,36+7}, {36+8,36+8},  {36+11,36+11}, {36+10,36+10},
    {24+0,24+0}, {24+1,24+1},  {24+4,24+4}, {24+3,24+3},  {24+7,24+7}, {24+8,24+8},  {24+11,24+11}, {24+10,24+10},
    {12+0,12+0}, {12+1,12+1},  {12+4,12+4}, {12+3,12+3},  {12+7,12+7}, {12+8,12+8},  {12+11,12+11}, {12+10,12+10},
    {0+0,0+0}, {0+1,0+1},  {0+4,0+4}, {0+3,0+3},  {0+7,0+7}, {0+8,0+8},  {0+11,0+11}, {0+10,0+10}
  },
  32,
  { 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 31 },
  true,
  { '1', '5', '3', '3', '0', ' ', ' ', ' ' },
  { '0', '6', '7', '1', '2' }
};

enum sliderBoardType : byte {
  SLIDER_TYPE_DIVA = 0,
  SLIDER_TYPE_CHUNI = 1,
};

const sliderDef* allSliderDefs[2] = { &divaSlider, &chuniSlider };

