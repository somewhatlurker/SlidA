/*
 * This is a library for using mpr121 capacitive touch sensing ICs.
 * It's designed to be ideal for use in rhythm game controllers.
 * 
 * Allows configuration of autoconfig and important sampling parameters.
 * Very much based on the quick start guide (AN3944).
 * 
 * Basic usage:
 *   mpr121 mpr = mpr121(address);
 *   // optionally set custom parameters
 *   // for 4ms response time (default is 8): mpr.ESI = MPR_ESI_1;
 *   // for better autoconfig: mpr.autoConfigUSL = 256L * (supplyMillivolts - 700) / supplyMillivolts;
 *   
 *   Wire.begin();
 *   Wire.setClock(400000); // mpr121 can run in fast mode. if you have issues, try removing this line
 *   mpr.startMPR();
 *   
 *   #if MPR121_USE_BITFIELDS
 *     short touches = mpr.readTouchState();
 *     bool touch0 = bitRead(touches, 0);
 *   #else // MPR121_USE_BITFIELDS
 *     bool* touches = mpr.readTouchState();
 *     bool touch0 = touches[0];
 *   #endif // MPR121_USE_BITFIELDS
 *   
 *   reading data isn't thread-safe, but that shouldn't be an issue
 *   also note that some internal buffers (returned by some functions) are shared between instances to save memory
 *   (electrodeTouchBuf returned by readTouchState is excepted)
 *   
 *   changes to properties won't take effect until you stop then restart the MPR121
 *   
 *   make sure to allow some time (10ms should be plenty) for the MPR121 to start
 * 
 * Copyright 2019 somewhatlurker, MIT license
 */

#pragma once
#include <Arduino.h>
#include "mpr121.h"

byte mpr121::i2cReadBuf[MPR121_I2C_BUFLEN];
short mpr121::electrodeDataBuf[13];
byte mpr121::electrodeBaselineBuf[13];
#if !MPR121_USE_BITFIELDS
  bool mpr121::electrodeOORBuf[15];
#endif

// write a value to an MPR121 register
void mpr121::writeRegister(mpr121Register addr, byte value) {
  i2cWire->beginTransmission(i2cAddr);
  i2cWire->write(addr);
  i2cWire->write(value);
  i2cWire->endTransmission();
}

// read bytes from consecutive MPR121 registers, starting at addr
// max of 8 bytes
byte* mpr121::readRegister(mpr121Register addr, byte count) {
  if (count > MPR121_I2C_BUFLEN)
    count = MPR121_I2C_BUFLEN;
  
  // write the address to read from
  i2cWire->beginTransmission(i2cAddr);
  i2cWire->write(addr);
  i2cWire->endTransmission(false); // use false to restart instead of stopping
  
  i2cWire->requestFrom(i2cAddr, count, (byte)true); // sendStop is true by defaualt where supported, but setting it guarantees support

  byte readnum = 0;
  while(i2cWire->available() && readnum < count)
  {
    i2cReadBuf[readnum] = i2cWire->read();
    readnum++;
  }
  
  return i2cReadBuf;
}


// returns true if electrode num and count can be used or false if the function should immediately return
// may modify electrode and/or count to keep them in bounds as necessay
bool mpr121::checkElectrodeNum(byte &electrode, byte &count) {
  if (electrode > 12)
    return false;

  if (electrode + count > 13)
    count = 13 - electrode;

  return true;
}

// returns true if electrode num can be used or false if the function should immediately return
// may modify electrode to keep it in bounds as necessay
bool mpr121::checkElectrodeNum(byte &electrode) {
  if (electrode > 12)
    return false;

  return true;
}

// returns true if pin num and count can be used or false if the function should immediately return
// may modify pin and/or count to keep them in bounds as necessay
bool mpr121::checkGPIOPinNum(byte &pin, byte &count) {
  if (pin > 12)
    return false;
  
  if (pin < 4)
    pin = 4;
  
  if (pin + count > 12)
    count = 12 - pin;
    
  return true;
}

// returns true if pin num can be used or false if the function should immediately return
// may modify pin to keep it in bounds as necessay
bool mpr121::checkGPIOPinNum(byte &pin) {
  if (pin > 12)
    return false;
  
  if (pin < 4)
    pin = 4;
    
  return true;
}


// set the touch and release thresholds for a subset of electrodes (AN3892)
void mpr121::setElectrodeThresholds(byte electrode, byte count, byte touchThreshold, byte releaseThreshold) {
  if (!checkElectrodeNum(electrode, count))
    return;

  for (int i = 0; i < count; i++) {
    writeRegister((mpr121Register)(MPRREG_ELE0_TOUCH_THRESHOLD + (i+electrode)*2), touchThreshold);
    writeRegister((mpr121Register)(MPRREG_ELE0_RELEASE_THRESHOLD + (i+electrode)*2), touchThreshold);
  }
}


// set the "Max Half Delta" values (AN3891)
// max: 63
void mpr121::setMHD(byte rising, byte falling) {
  writeRegister(MPRREG_MHD_RISING, rising);
  writeRegister(MPRREG_MHD_FALLING, falling);
}
  
// set the "Noise Half Delta" values (AN3891)
// max: 63
void mpr121::setNHD(byte rising, byte falling, byte touched) {
  writeRegister(MPRREG_NHD_AMOUNT_RISING, rising);
  writeRegister(MPRREG_NHD_AMOUNT_FALLING, falling);
  writeRegister(MPRREG_NHD_AMOUNT_TOUCHED, touched);
}
  
// set the "Noise Count Limit" values (AN3891)
void mpr121::setNCL(byte rising, byte falling, byte touched) {
  writeRegister(MPRREG_NCL_RISING, rising);
  writeRegister(MPRREG_NCL_FALLING, falling);
  writeRegister(MPRREG_NCL_TOUCHED, touched);
}

// set the "Filter Delay Limit" values (AN3891)
void mpr121::setFDL(byte rising, byte falling, byte touched) {
  writeRegister(MPRREG_FDL_RISING, rising);
  writeRegister(MPRREG_FDL_FALLING, falling);
  writeRegister(MPRREG_NCL_TOUCHED, touched);
}


// set the "Max Half Delta" values for proximity detection (AN3891/AN3893)
// max: 63
void mpr121::setMHDProx(byte rising, byte falling) {
  writeRegister(MPRREG_ELEPROX_MHD_RISING, rising);
  writeRegister(MPRREG_ELEPROX_MHD_FALLING, falling);
}

// set the "Noise Half Delta" values for proximity detection (AN3891/AN3893)
// max: 63
void mpr121::setNHDProx(byte rising, byte falling, byte touched) {
  writeRegister(MPRREG_ELEPROX_NHD_AMOUNT_RISING, rising);
  writeRegister(MPRREG_ELEPROX_NHD_AMOUNT_FALLING, falling);
  writeRegister(MPRREG_ELEPROX_NHD_AMOUNT_TOUCHED, touched);
}

// set the "Noise Count Limit" values for proximity detection (AN3891/AN3893)
void mpr121::setNCLProx(byte rising, byte falling, byte touched) {
  writeRegister(MPRREG_ELEPROX_NCL_RISING, rising);
  writeRegister(MPRREG_ELEPROX_NCL_FALLING, falling);
  writeRegister(MPRREG_ELEPROX_NCL_TOUCHED, touched);
}

// set the "Filter Delay Limit" values  for proximity detection(AN3891/AN3893)
void mpr121::setFDLProx(byte rising, byte falling, byte touched) {
  writeRegister(MPRREG_ELEPROX_FDL_RISING, rising);
  writeRegister(MPRREG_ELEPROX_FDL_FALLING, falling);
  writeRegister(MPRREG_ELEPROX_NCL_TOUCHED, touched);
}


// set "Debounce" counts
// a detection must be held this many times before the status register is updated
// max: 7
void mpr121::setDebounce(byte touchNumber, byte releaseNumber) {
  touchNumber &= 0b0111;
  releaseNumber &= 0b0111;

  writeRegister(MPRREG_DEBOUNCE, (touchNumber << 4) | releaseNumber);
}


// set "Filter Configuration" (AN3890)
// FFI: "First Filter Iterations"
// CDC: global "Charge Discharge Current" (μA), not used if autoconfig is enabled -- max 63, default 16
// CDT: global "Charge Discharge Time" (μs), not used if autoconfig is enabled
// SFI: "Second Filter Iterations"
// ESI: "Electrode Sample Interval" (ms)
// response time is equal to SFI * ESI in ms
//   --set to 4 samples and 1 or 2 ms sample interval for fast response
void mpr121::setFilterConfig(mpr121FilterFFI FFI, byte CDC, mpr121FilterCDT CDT, mpr121FilterSFI SFI, mpr121FilterESI ESI) {
  byte FFI_2 = (byte)FFI & 0b00000011;
  byte CDC_6 = CDC & 0b00111111;
  byte CDT_3 = (byte)CDT & 0b00000111;
  byte SFI_2 = (byte)SFI & 0b00000011;
  byte ESI_3 = (byte)ESI & 0b00000111;

  writeRegister(MPRREG_FILTER_GLOBAL_CDC_CONFIG, (FFI_2 << 6) | CDC_6);
  writeRegister(MPRREG_FILTER_GLOBAL_CDT_CONFIG, (CDT_3 << 5) | (SFI_2 << 3) | ESI_3);

  // update FFI in autoconf
  byte autoConf = readRegister(MPRREG_AUTOCONFIG_CONTROL_0) & 0b00111111;
  autoConf |= (FFI_2 << 6);
  writeRegister(MPRREG_AUTOCONFIG_CONTROL_0, autoConf);
}


// set "Electrode Configuration"
// CL: calibration lock (baseline tracking and initial value settings)
// ELEPROX_EN: sets electrodes to be used for proximity detection
// ELE_EN: sets the number of electrodes to be detected, starting from ELE0 (0 to disable)
//   --setting this will enter run mode
void mpr121::setElectrodeConfiguration(mpr121ElectrodeConfigCL CL, mpr121ElectrodeConfigProx ELEPROX_EN, byte ELE_EN) {
  byte CL_2 = CL & 0b00000011;
  byte ELEPROX_EN_2 = ELEPROX_EN & 0b00000011;
  byte ELE_EN_4 = ELE_EN & 0b00001111;

  writeRegister(MPRREG_ELECTRODE_CONFIG, (CL_2 << 6) | (ELEPROX_EN_2 << 4) | ELE_EN_4);
}


// set "Auto-Configure" settings (AN3889)
// USL: "Up-Side Limit" -- calculate this as `256 * (supplyMillivolts - 700) / supplyMillivolts`
//   --if unsure, use the value for 1.8V supply (156)
// LSL: "Low-Side Limit" -- calculate this as `USL * 0.65`
//   --if unsure, use the value for 1.8V supply (101)
// TL: "Target Level" -- calculate this as `USL * 0.9`
//   --if unsure, use the value for 1.8V supply (140)
// RETRY: set the number of retries for failed config before setting out of range
// BVA: "Baseline Value Adjust" changes how the baseline registers will be set after auto-configuration completes
// ARE: "Automatic Reconfiguration Enable" will reconfigure out of range (failed) channels every sampling interval
// ACE: "Automatic Configuration Enable" will enable/disable auto-configuration when entering run mode
void mpr121::setAutoConfig(byte USL, byte LSL, byte TL, mpr121AutoConfigRetry RETRY, mpr121AutoConfigBVA BVA, bool ARE, bool ACE) {
  byte FFI = (readRegister(MPRREG_AFE_CONFIG) >> 6) & 0b00000011;
  byte RETRY_2 = RETRY & 0b00000011;
  byte BVA_2 = BVA & 0b00000011;
  
  writeRegister(MPRREG_AUTOCONFIG_USL, USL);
  writeRegister(MPRREG_AUTOCONFIG_LSL, LSL);
  writeRegister(MPRREG_AUTOCONFIG_TL, TL);
  writeRegister(MPRREG_AUTOCONFIG_CONTROL_0, (FFI << 6) | (RETRY_2 << 4) | (BVA_2 << 2) | ((ARE ? 1 : 0) << 1) | (ARE ? 1 : 0));
}


// set the GPIO PWM value for consecutive pins (AN3894)
// max value is 15
// pin 9 apparently has a logic bug and must be set the same as pin 10 to work
//   (https://community.nxp.com/thread/305474)
void mpr121::setPWM(byte pin, byte count, byte value) {
  if (!checkGPIOPinNum(pin, count))
    return;

  pin -= 4; // easier to make it 0-indexed now

  byte value_4 = value & 0b1111;
  
  mpr121Register reg;
  byte regVal;

  for (int i = 0; i < count; i++) {
    if (i == 0 || (pin + i) % 2 == 0) { // if just starting of moving to a new register's start
      reg = MPRREG_PWM_DUTY_0 + (pin + i)/2;
      regVal = readRegister(reg);
    }
    
    if ((pin + i) % 2 == 0)
      regVal = (regVal & 0b11110000) | value_4;
    else
      regVal = (regVal & 0b00001111) | (value_4 << 4);

    if ((pin + i) % 2 == 1 || i == count - 1) // if the last of a register or the last iteration
      writeRegister(reg, regVal);
  }
}


// create an MPR121 device with sane default settings
mpr121::mpr121(byte addr, TwoWire *wire)
{
  // ensure the mpr device has had time to get ready
  // no check because pro micro sucks
  // if (millis() < 10) {
  //   delay(10);
  // }
  
  i2cAddr = addr;
  i2cWire = wire;

  // values from getting started guide
  // MHDrising = 0x01;
  // MHDfalling = 0x01;
  // NHDrising = 0x01;
  // NHDfalling = 0x01;
  // NHDtouched = 0x00; // ?
  // NCLrising = 0x00;
  // NCLfalling = 0xff;
  // NCLtouched = 0x00; // ?
  // FDLrising = 0x00;
  // FDLfalling = 0x02;
  // FDLtouched = 0x00; // ?

  // values from AN3893
  // MHDrisingProx = 0xff;
  // MHDfallingProx = 0x01;
  // NHDrisingProx = 0xff;
  // NHDfallingProx = 0x01;
  // NHDtouchedProx = 0x00;
  // NCLrisingProx = 0x00;
  // NCLfallingProx = 0xff;
  // NCLtouchedProx = 0x00;
  // FDLrisingProx = 0x00;
  // FDLfallingProx = 0xff;
  // FDLtouchedProx = 0x00;

  // adjusted values
  MHDrising = 0x01;
  MHDfalling = 0x01;
  NHDrising = 0x01;
  NHDfalling = 0x03;
  NHDtouched = 0x00;
  NCLrising = 0x04;
  NCLfalling = 0xc0;
  NCLtouched = 0x00;
  FDLrising = 0x00;
  FDLfalling = 0x02;
  FDLtouched = 0x00;

  // no clue what might be good here lol
  MHDrisingProx = 0x20;
  MHDfallingProx = 0x01;
  NHDrisingProx = 0x10;
  NHDfallingProx = 0x03;
  NHDtouchedProx = 0x00;
  NCLrisingProx = 0x04;
  NCLfallingProx = 0xc0;
  NCLtouchedProx = 0x00;
  FDLrisingProx = 0x00;
  FDLfallingProx = 0x80;
  FDLtouchedProx = 0x00;

  for (int i = 0; i < 13; i++)
  {
    touchThresholds[i] = 0x0f;
    releaseThresholds[i] = 0x0a;
  }

  debounceTouch = 0x00;
  debounceRelease = 0x00;

  FFI = MPR_FFI_6;
  globalCDC = 16;
  globalCDT = MPR_CDT_0_5;
  SFI = MPR_SFI_4;
  ESI = MPR_ESI_2; // 4 samples, 2ms rate ==> 8ms response time

  calLock = MPR_CL_TRACKING_ENABLED;
  proxEnable = MPR_ELEPROX_DISABLED;

  autoConfigUSL = 0;
  autoConfigLSL = 0;
  autoConfigTL = 0;
  autoConfigRetry = MPR_AUTOCONFIG_RETRY_DISABLED;
  autoConfigBaselineAdjust = MPR_AUTOCONFIG_BVA_SET_CLEAR3;
  autoConfigEnableReconfig = true;
  autoConfigEnableCalibration = true;
}


// read one touch state bool
// also use this for reading GPIO inputs
bool mpr121::readTouchState(byte electrode) {
  if (!checkElectrodeNum(electrode))
    return false;

  byte rawdata = readRegister((mpr121Register)(MPRREG_ELE0_TO_ELE7_TOUCH_STATUS + electrode/8));
  
  return bitRead(rawdata, electrode % 8);
}


#if MPR121_USE_BITFIELDS
  // read the 13 touch state bits
  // also use this for reading GPIO inputs
  short mpr121::readTouchState() {
    byte* rawdata = readRegister(MPRREG_ELE0_TO_ELE7_TOUCH_STATUS, 2);
    return rawdata[0] | (rawdata[1] << 8);
  }

  // read the 15 out of range bits
  // [13]: auto-config fail flag
  // [14]: auto-reconfig fail flag
  short mpr121::readOORState() {
    byte* rawdata = readRegister(MPRREG_ELE0_TO_ELE7_OOR_STATUS, 2);
    byte autoConfBits = ((rawdata[1] & 0b10000000) >> 2) | (rawdata[1] & 0b01000000);
    return rawdata[0] | ((rawdata[1] & 0b00011111) << 8) | (autoConfBits << 8);
  }
#else // MPR121_USE_BITFIELDS
  // read the 13 touch state bools
  // also use this for reading GPIO inputs
  bool* mpr121::readTouchState() {
    byte* rawdata = readRegister(MPRREG_ELE0_TO_ELE7_TOUCH_STATUS, 2);
    
    electrodeTouchBuf[0] = bitRead(rawdata[0], 0);
    electrodeTouchBuf[1] = bitRead(rawdata[0], 1);
    electrodeTouchBuf[2] = bitRead(rawdata[0], 2);
    electrodeTouchBuf[3] = bitRead(rawdata[0], 3);
    electrodeTouchBuf[4] = bitRead(rawdata[0], 4);
    electrodeTouchBuf[5] = bitRead(rawdata[0], 5);
    electrodeTouchBuf[6] = bitRead(rawdata[0], 6);
    electrodeTouchBuf[7] = bitRead(rawdata[0], 7);
    
    electrodeTouchBuf[8] = bitRead(rawdata[1], 0);
    electrodeTouchBuf[9] = bitRead(rawdata[1], 1);
    electrodeTouchBuf[10] = bitRead(rawdata[1], 2);
    electrodeTouchBuf[11] = bitRead(rawdata[1], 3);
    electrodeTouchBuf[12] = bitRead(rawdata[1], 4);
    
    // electrodeTouchBuf[13] = bitRead(rawdata[1], 7);
  
    return electrodeTouchBuf;
  }

  // read the 15 out of range bools
  // [13]: auto-config fail flag
  // [14]: auto-reconfig fail flag
  bool* mpr121::readOORState() {
    byte* rawdata = readRegister(MPRREG_ELE0_TO_ELE7_OOR_STATUS, 2);
    
    electrodeOORBuf[0] = bitRead(rawdata[0], 0);
    electrodeOORBuf[1] = bitRead(rawdata[0], 1);
    electrodeOORBuf[2] = bitRead(rawdata[0], 2);
    electrodeOORBuf[3] = bitRead(rawdata[0], 3);
    electrodeOORBuf[4] = bitRead(rawdata[0], 4);
    electrodeOORBuf[5] = bitRead(rawdata[0], 5);
    electrodeOORBuf[6] = bitRead(rawdata[0], 6);
    electrodeOORBuf[7] = bitRead(rawdata[0], 7);
    
    electrodeOORBuf[8] = bitRead(rawdata[1], 0);
    electrodeOORBuf[9] = bitRead(rawdata[1], 1);
    electrodeOORBuf[10] = bitRead(rawdata[1], 2);
    electrodeOORBuf[11] = bitRead(rawdata[1], 3);
    electrodeOORBuf[12] = bitRead(rawdata[1], 4);
    
    electrodeOORBuf[13] = bitRead(rawdata[1], 7);
    electrodeOORBuf[14] = bitRead(rawdata[1], 6);
  
    return electrodeOORBuf;
  }
#endif // MPR121_USE_BITFIELDS


// check the over current flag
bool mpr121::readOverCurrent() {
  return bitRead(readRegister(MPRREG_ELE8_TO_ELEPROX_TOUCH_STATUS), 7);
}

// clear the over current flag
void mpr121::clearOverCurrent() {
  writeRegister(MPRREG_ELE8_TO_ELEPROX_TOUCH_STATUS, 0b10000000);
}
  

// read filtered data for consecutive electrodes
short* mpr121::readElectrodeData(byte electrode, byte count) {
  if (!checkElectrodeNum(electrode, count))
    return electrodeDataBuf;

  byte* rawdata = readRegister((mpr121Register)(MPRREG_ELE0_FILTERED_DATA_LSB + electrode*2), count*2);

  for (int i = 0; i < count; i++) {
    electrodeDataBuf[electrode + i] = rawdata[i*2] | ((rawdata[i*2 + 1] & 0b00000011) << 8);
  }

  return &electrodeDataBuf[electrode];
}
  
// read baseline values for consecutive electrodes
byte* mpr121::readElectrodeBaseline(byte electrode, byte count) {
  if (!checkElectrodeNum(electrode, count))
    return electrodeBaselineBuf;

  byte* rawdata = readRegister((mpr121Register)(MPRREG_ELE0_BASELINE + electrode), count);

  for (int i = 0; i < count; i++) {
    electrodeBaselineBuf[electrode + i] = rawdata[i];
  }

  return &electrodeBaselineBuf[electrode];
}

// write baseline value for consecutive electrodes
void mpr121::writeElectrodeBaseline(byte electrode, byte count, byte value) {
  if (!checkElectrodeNum(electrode, count))
    return;

  for (int i = 0; i < count; i++) {
    writeRegister((mpr121Register)(MPRREG_ELE0_BASELINE + (i+electrode)), value);
  }
}

// easy way to set touchThresholds and releaseThresholds
// prox sets whether to set for proximity detection too
void mpr121::setAllThresholds(byte touched, byte released, bool prox) {
  byte maxElectrode = 11;
  if (prox)
    maxElectrode = 12;

  for (int i = 0; i <= maxElectrode; i++) {
    touchThresholds[i] = touched;
    releaseThresholds[i] = released;
  }
}


// set mode for consecutive GPIO pins
// GPIO can be used on pins 4-11 when they aren't used for sensing
// use mode MPR_GPIO_MODE_OUTPUT_OPENDRAIN_HIGH for direct LED driving -- it can source up to 12mA
void mpr121::setGPIOMode(byte pin, byte count, mpr121GPIOMode mode) {
  if (!checkGPIOPinNum(pin, count))
    return;

  pin -= 4; // easier to make it 0-indexed now

  
  byte enableByte = readRegister(MPRREG_GPIO_ENABLE);

  // disable the modified outputs while changing stuff around
  for (int i = 0; i < count; i++) {
    bitClear(enableByte, pin + i);
  }
  writeRegister(MPRREG_GPIO_ENABLE, enableByte);

  if (mode == MPR_GPIO_MODE_DISABLED)
    return; // all done, no need to worry about other values


  byte tempByte;
  bool tempVal;

  // set direction
  tempByte = readRegister(MPRREG_GPIO_DIRECTION);
  tempVal = bitRead(mode, 2);
  for (int i = 0; i < count; i++) {
    bitWrite(tempByte, pin + i, tempVal);
  }
  writeRegister(MPRREG_GPIO_DIRECTION, tempByte);

  // set control 0
  tempByte = readRegister(MPRREG_GPIO_CONTROL_0);
  tempVal = bitRead(mode, 1);
  for (int i = 0; i < count; i++) {
    bitWrite(tempByte, pin + i, tempVal);
  }
  writeRegister(MPRREG_GPIO_CONTROL_0, tempByte);

  // set control 1
  tempByte = readRegister(MPRREG_GPIO_CONTROL_1);
  tempVal = bitRead(mode, 0);
  for (int i = 0; i < count; i++) {
    bitWrite(tempByte, pin + i, tempVal);
  }
  writeRegister(MPRREG_GPIO_CONTROL_1, tempByte);


  // set enable to final value
  tempVal = bitRead(mode, 3);
  for (int i = 0; i < count; i++) {
    bitWrite(enableByte, pin + i, tempVal);
  }
  writeRegister(MPRREG_GPIO_ENABLE, enableByte);
}


// write a digital value to consecutive GPIO pins
void mpr121::writeGPIODigital(byte pin, byte count, bool value) {
  if (!checkGPIOPinNum(pin, count))
    return;

  // disable PWM for affected pins
  // (doesn't use 0-indexed GPIO pin number)
  setPWM(pin, count, 0);

  pin -= 4; // easier to make it 0-indexed now

  
  mpr121Register reg = value ? MPRREG_GPIO_DATA_SET : MPRREG_GPIO_DATA_CLEAR;

  byte tempByte = 0;
  for (int i = 0; i < count; i++) {
    bitSet(tempByte, pin + i);
  }
  writeRegister(reg, tempByte);
}


// write an "analog" (PWM) value to consecutive GPIO pins
// max value is 15
// pin 9 apparently has a logic bug and must be set the same as pin 10 to work
//   (https://community.nxp.com/thread/305474)
void mpr121::writeGPIOAnalog(byte pin, byte count, byte value) {
  if (!checkGPIOPinNum(pin, count))
    return;

  // set PWM for affected pins
  // (doesn't use 0-indexed GPIO pin number)
  setPWM(pin, count, value);

  pin -= 4; // easier to make it 0-indexed now


  mpr121Register reg = value == 0 ? MPRREG_GPIO_DATA_CLEAR : MPRREG_GPIO_DATA_SET;

  byte tempByte = 0;
  for (int i = 0; i < count; i++) {
    bitSet(tempByte, pin + i);
  }
  writeRegister(reg, tempByte);
}


// apply settings and enter run mode with a set number of electrodes
void mpr121::startMPR(byte electrodes) {
  stopMPR();
  
  // restrict value of numeric properties with < 8 bits to actual sent values
  MHDrising &= 0b00111111;
  MHDfalling &= 0b00111111;
  NHDrising &= 0b00111111;
  NHDfalling &= 0b00111111;
  NHDtouched &= 0b00111111;
  MHDrisingProx &= 0b00111111;
  MHDfallingProx &= 0b00111111;
  NHDrisingProx &= 0b00111111;
  NHDfallingProx &= 0b00111111;
  NHDtouchedProx &= 0b00111111;
  debounceTouch &= 0b0111;
  debounceRelease &= 0b0111;
  globalCDC &= 0b00111111;

  // set/calculate autoconfig values
  if (autoConfigUSL == 0)
    autoConfigUSL = 156;
  if (autoConfigLSL == 0)
    autoConfigLSL = autoConfigUSL * 0.65;
  if (autoConfigTL == 0)
    autoConfigTL = autoConfigUSL * 0.9;

  setMHD(MHDrising, MHDfalling);
  setNHD(NHDrising, NHDfalling, NHDtouched);
  setNCL(NCLrising, NCLfalling, NCLtouched);
  setFDL(FDLrising, FDLfalling, FDLtouched);
  
  setMHDProx(MHDrisingProx, MHDfallingProx);
  setNHDProx(NHDrisingProx, NHDfallingProx, NHDtouchedProx);
  setNCLProx(NCLrisingProx, NCLfallingProx, NCLtouchedProx);
  setFDLProx(FDLrisingProx, FDLfallingProx, FDLtouchedProx);
  
  for (int i = 0; i < 13; i++)
  {
    mpr121::setElectrodeThresholds(i, touchThresholds[i], releaseThresholds[i]);
  }

  setDebounce(debounceTouch, debounceRelease);

  setFilterConfig(FFI, globalCDC, globalCDT, SFI, ESI);

  setAutoConfig(autoConfigUSL, autoConfigLSL, autoConfigTL, autoConfigRetry, autoConfigBaselineAdjust, autoConfigEnableReconfig, autoConfigEnableCalibration);
  
  setElectrodeConfiguration(calLock, proxEnable, electrodes);
}

// exit run mode
void mpr121::stopMPR() {
  byte oldConfig = readRegister(MPRREG_ELECTRODE_CONFIG);
  writeRegister(MPRREG_ELECTRODE_CONFIG, oldConfig & 0b11000000);
}

// reset the mpr121
void mpr121::softReset() {
  writeRegister(MPRREG_SOFT_RESET, 0x63);
}

