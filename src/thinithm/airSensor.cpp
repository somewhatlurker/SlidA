#include "airSensor.h"

// set which LED should be enabled (-1 for off)
void airSensor::changeLed(byte led) {
  switch (led) {
    case 0:
      pinMode(pins.leds[0], OUTPUT);
      pinMode(pins.leds[1], OUTPUT);
      pinMode(pins.leds[2], INPUT);
      
      digitalWrite(pins.leds[0], HIGH);
      digitalWrite(pins.leds[1], LOW);
      digitalWrite(pins.leds[2], LOW);
      
      break;
      
    case 1:
      pinMode(pins.leds[0], OUTPUT);
      pinMode(pins.leds[1], OUTPUT);
      pinMode(pins.leds[2], INPUT);
      
      digitalWrite(pins.leds[0], LOW);
      digitalWrite(pins.leds[1], HIGH);
      digitalWrite(pins.leds[2], LOW);
      
      break;
      
    case 2:
      pinMode(pins.leds[0], INPUT);
      pinMode(pins.leds[1], OUTPUT);
      pinMode(pins.leds[2], OUTPUT);
      
      digitalWrite(pins.leds[0], LOW);
      digitalWrite(pins.leds[1], HIGH);
      digitalWrite(pins.leds[2], LOW);
      
      break;
      
    case 3:
      pinMode(pins.leds[0], INPUT);
      pinMode(pins.leds[1], OUTPUT);
      pinMode(pins.leds[2], OUTPUT);
      
      digitalWrite(pins.leds[0], LOW);
      digitalWrite(pins.leds[1], LOW);
      digitalWrite(pins.leds[2], HIGH);
      
      break;
      
    case 4:
      pinMode(pins.leds[0], OUTPUT);
      pinMode(pins.leds[1], INPUT);
      pinMode(pins.leds[2], OUTPUT);
      
      digitalWrite(pins.leds[0], HIGH);
      digitalWrite(pins.leds[1], LOW);
      digitalWrite(pins.leds[2], LOW);
      
      break;
      
    case 5:
      pinMode(pins.leds[0], OUTPUT);
      pinMode(pins.leds[1], INPUT);
      pinMode(pins.leds[2], OUTPUT);
      
      digitalWrite(pins.leds[0], LOW);
      digitalWrite(pins.leds[1], LOW);
      digitalWrite(pins.leds[2], HIGH);
      
      break;
      
    default:
      pinMode(pins.leds[0], OUTPUT);
      pinMode(pins.leds[1], OUTPUT);
      pinMode(pins.leds[2], OUTPUT);
      
      digitalWrite(pins.leds[0], LOW);
      digitalWrite(pins.leds[1], LOW);
      digitalWrite(pins.leds[2], LOW);
      
      break;
  }
}

// read a raw sensor pin
int airSensor::readSensor(byte sensor) {
  if (sensor < 0 || sensor > 5)
    return 0;

  #if AIR_USE_ANALOG
    return analogRead(pins.sensors[sensor]);
  #else
    return digitalRead(pins.sensors[sensor]);
  #endif
}

// read a raw sensor level value (with LED switched)
int airSensor::readLevelVal(byte level) {
  changeLed(level);
  delayMicroseconds(AIR_HOLD_US); // allow some time for switching to occur
  int val = readSensor(level);
  killLeds();
  return val;
}

#if AIR_USE_ANALOG
// update a baseline
void airSensor::updateBaseline(byte level, int val) {
  // use some static floats to save on floating point operations
  static float multiplier = 1 / (float)AIR_BASELINE_AVERAGING;
  static float one_minus_multiplier = 1 - multiplier;
  
  sensorBaselines[level] = val * multiplier + sensorBaselines[level] * one_minus_multiplier;
}

// update a threshold
void airSensor::updateThreshold(byte level, int val) {
  // use some static floats to save on floating point operations
  static float multiplier = 1 / (float)AIR_THRESHOLD_AVERAGING;
  static float one_minus_multiplier = 1 - multiplier;
  
  sensorThresholds[level] = val * multiplier + sensorThresholds[level] * one_minus_multiplier;
}

// update an averaged level value
void airSensor::updateAveragedVal(byte level, int val) {
  // use some static floats to save on floating point operations
  static float multiplier = 1 / (float)AIR_VALUE_AVERAGING;
  static float one_minus_multiplier = 1 - multiplier;
  
  averagedVals[level] = val * multiplier + averagedVals[level] * one_minus_multiplier;
}
#endif // AIR_USE_ANALOG

// create a sensor
airSensor::airSensor(airSensorPins pinstruct) {
  pins = pinstruct;
  
  pinMode(pins.sensors[0], INPUT);
  pinMode(pins.sensors[1], INPUT);
  pinMode(pins.sensors[2], INPUT);
  pinMode(pins.sensors[3], INPUT);
  pinMode(pins.sensors[4], INPUT);
  pinMode(pins.sensors[5], INPUT);
}

// read whether an air level has been blocked
bool airSensor::checkLevel(byte level) {
  if (level < 0 || level > 5)
    return false;

  #if AIR_USE_ANALOG
    if (!isCalibrated)
      calibrate();
  
    updateAveragedVal(level, readLevelVal(level));
    int delta = sensorBaselines[level] - averagedVals[level];
    
    if (delta > sensorThresholds[level]) {
      updateThreshold(level, delta / AIR_THRESHOLD_DIVISOR);
      return true;
    }
    else {
      updateBaseline(level, averagedVals[level]);
      return false;
    }
  #else // AIR_USE_ANALOG
    return readLevelVal(level) == LOW ? true : false;
  #endif // AIR_USE_ANALOG
}

// read whether all air levels have been blocked (returns pointer to an array of bools)
bool* airSensor::checkAll() {
  static bool buf[6];
  for (int i = 0; i < 6; i++) {
    buf[i] = checkLevel(i);
  }
  return buf;
}

#if AIR_USE_ANALOG
// (re-)calibrate the sensors
// use offset to continue calibrating after already calibrating a number of samples
void airSensor::calibrate(byte samples, byte offset) {
  if (offset == 0) {
    // reset averages, baselines, and thresholds
    // actually completely unnecessary
    for (int i = 0; i < 6; i++) {
      averagedVals[i] = 800;
      sensorBaselines[i] = 800;
      sensorThresholds[i] = 100;
    }
  }
  
  // find baseline values for sensor on and save averaged vals
  for (float spl = 1; spl <= samples; spl++) {
    for (int i = 0; i < 6; i++) {
      int val = readLevelVal(i); // readLevelVal uses LEDs

      float multiplier = 1 / (spl + offset);
      float one_minus_multiplier = 1 - multiplier;
      
      // use custom averages update that takes into account number of samples taken
      averagedVals[i] = val * multiplier + averagedVals[i] * one_minus_multiplier;
      
      // use custom baseline update that takes into account number of samples taken
      sensorBaselines[i] = val * multiplier + sensorBaselines[i] * one_minus_multiplier;
    }
  }
  
  // find threshold values for sensor off
  for (float spl = 1; spl <= samples; spl++) {
    for (int i = 0; i < 6; i++) {
      int val = readSensor(i); // readSensor doesn't use LEDs
      int delta = sensorBaselines[i] - val;

      float multiplier = 1 / (spl + offset);
      float one_minus_multiplier = 1 - multiplier;
      
      // use custom threshold update that takes into account number of samples taken
      sensorThresholds[i] = (delta / AIR_THRESHOLD_DIVISOR) * multiplier + sensorThresholds[i] * one_minus_multiplier;
    }
    delayMicroseconds(AIR_HOLD_US); // should ensure there's some delay...
  }

  isCalibrated = true;
}
#endif // AIR_USE_ANALOG