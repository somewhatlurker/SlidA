#pragma once
#include <Arduino.h>

#define AIR_USE_ANALOG true

#define AIR_VALUE_AVERAGING 2
#define AIR_BASELINE_AVERAGING 256
#define AIR_THRESHOLD_AVERAGING 128
#define AIR_CALIBRATION_SAMPLES 128
#define AIR_HOLD_US 150
#define AIR_THRESHOLD_DIVISOR 3

struct airTowerPins {
  byte leds[3];
  byte sensors[6];
};

class airTower {
private:
  airTowerPins pins;

  #if AIR_USE_ANALOG
    bool isCalibrated = false;
    int averagedVals[6]; // slightly smoothed sensor readings
  #endif
  
  // set which LED should be enabled (-1 for off)
  void changeLed(byte led);
  
  // turn off all LEDs
  void killLeds() {
    changeLed(-1);
  }
  
  // read a raw sensor pin
  int readSensor(byte sensor);
  
  // read a raw sensor level value (with LED switched)
  int readLevelVal(byte level);

  #if AIR_USE_ANALOG
    // update a baseline
    void updateBaseline(byte level, int val);
    
    // update a threshold
    void updateThreshold(byte level, int val);
    
    // update an averaged level value
    void updateAveragedVal(byte level, int val);
  #endif

public:
  // create an air tower
  airTower(airTowerPins pins);

  #if AIR_USE_ANALOG
    int sensorBaselines[6]; // very smoothed readings for unblocked sensor only
    int sensorThresholds[6]; // calibrated deltas from baseline for detection
  #endif
  
  // read whether an air level has been blocked
  bool checkLevel(byte level);
  
  // read whether all air levels have been blocked (returns pointer to an array of bools)
  bool* checkAll();

  #if AIR_USE_ANALOG
    // (re-)calibrate the sensors
    // use offset to continue calibrating after already calibrating a number of samples
    void airTower::calibrate(byte samples = AIR_CALIBRATION_SAMPLES, byte offset = 0);
  #endif
};
