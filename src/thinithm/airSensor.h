#pragma once
#include <Arduino.h>

#define AIR_VALUE_AVERAGING 2
#define AIR_BASELINE_AVERAGING 256
#define AIR_THRESHOLD_AVERAGING 128
#define AIR_CALIBRATION_SAMPLES 128
#define AIR_HOLD_US 150

struct airSensorPins {
  byte leds[3];
  byte sensors[6];
};

class airSensor {
private:
  airSensorPins pins;
  
  bool isCalibrated = false;
  
  int averagedVals[6]; // slightly smoothed sensor readings
  
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
  
  // update a baseline
  void updateBaseline(byte level, int val);
  
  // update a threshold
  void updateThreshold(byte level, int val);
  
  // update an averaged level value
  void updateAveragedVal(byte level, int val);

public:
  // create a sensor
  airSensor(airSensorPins pins);
  
  int sensorBaselines[6]; // very smoothed readings for unblocked sensor only
  int sensorThresholds[6]; // calibrated deltas from baseline for detection
  
  // read whether an air level has been blocked
  bool checkLevel(byte level);
  
  // read whether all air levels have been blocked (returns pointer to an array of bools)
  bool* checkAll();
  
  // (re-)calibrate the sensors
  void calibrate(byte samples);
};
