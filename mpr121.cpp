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
 *   // for better autoconfig: mpr.autoConfigUSL = 256 * (supplyMillivolts - 700) / supplyMillivolts;
 *   
 *   Wire.begin();
 *   mpr.startMPR();
 *   
 *   bool* touches = mpr.readTouchState();
 *   bool touch0 = touches[0];
 *   
 *   reading data isn't thread-safe, but that shouldn't be an issue
 *   
 *   changes to properties won't take effect until you stop then restart the MPR121
 * 
 * Copyright 2019 somewhatlurker, MIT license
 */

#pragma once
#include <Arduino.h>
#include "mpr121.h"

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


// set the touch and release thresholds for a subset of electrodes (AN3892)
void mpr121::setElectrodeThresholds(byte electrode, byte count, byte touchThreshold, byte releaseThreshold) {
  if (electrode > 12)
    return;

  if (electrode + count > 13)
    count = 13 - electrode;

  for (int i = 0; i < count; i++) {
    writeRegister((mpr121Register)(0x41 + (i+electrode)*2), touchThreshold);
    writeRegister((mpr121Register)(0x42 + (i+electrode)*2), touchThreshold);
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


// create an MPR121 device with sane default settings
mpr121::mpr121(byte addr, TwoWire *wire)
{
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

  // adjusted values
  MHDrising = 0x01;
  MHDfalling = 0x01;
  NHDrising = 0x01;
  NHDfalling = 0x03;
  NHDtouched = 0x00;
  NCLrising = 0x04;
  NCLfalling = 0x80;
  NCLtouched = 0x00;
  FDLrising = 0x00;
  FDLfalling = 0x02;
  FDLtouched = 0x00;

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

  
// read the 13 touch state bools and the over current flag
bool* mpr121::readTouchState() {
  byte* rawdata = readRegister(MPRREG_ELE0_TO_ELE7_TOUCH_STATUS, 2);
  
  electrodeTouchBuf[0] = (rawdata[0] & 0b00000001) != 0;
  electrodeTouchBuf[1] = (rawdata[0] & 0b00000010) != 0;
  electrodeTouchBuf[2] = (rawdata[0] & 0b00000100) != 0;
  electrodeTouchBuf[3] = (rawdata[0] & 0b00001000) != 0;
  electrodeTouchBuf[4] = (rawdata[0] & 0b00010000) != 0;
  electrodeTouchBuf[5] = (rawdata[0] & 0b00100000) != 0;
  electrodeTouchBuf[6] = (rawdata[0] & 0b01000000) != 0;
  electrodeTouchBuf[7] = (rawdata[0] & 0b10000000) != 0;
  
  electrodeTouchBuf[8] = (rawdata[1] & 0b00000001) != 0;
  electrodeTouchBuf[9] = (rawdata[1] & 0b00000010) != 0;
  electrodeTouchBuf[10] = (rawdata[1] & 0b00000100) != 0;
  electrodeTouchBuf[11] = (rawdata[1] & 0b00001000) != 0;
  electrodeTouchBuf[12] = (rawdata[1] & 0b00010000) != 0;
  
  // electrodeTouchBuf[13] = (rawdata[1] & 0b10000000) != 0;

  return electrodeTouchBuf;
}


// check the over current flag
bool mpr121::readOverCurrent() {
  return (readRegister(MPRREG_ELE8_TO_ELEPROX_TOUCH_STATUS) & 0b10000000) != 0;
}

// clear the over current flag
void mpr121::clearOverCurrent() {
  writeRegister(MPRREG_ELE8_TO_ELEPROX_TOUCH_STATUS, 0b10000000);
}


// read the 15 out of range bools
// [13]: auto-config fail flag
// [14]: auto-reconfig fail flag
bool* mpr121::readOORState() {
  byte* rawdata = readRegister(MPRREG_ELE0_TO_ELE7_OOR_STATUS, 2);
  
  electrodeOORBuf[0] = (rawdata[0] & 0b00000001) != 0;
  electrodeOORBuf[1] = (rawdata[0] & 0b00000010) != 0;
  electrodeOORBuf[2] = (rawdata[0] & 0b00000100) != 0;
  electrodeOORBuf[3] = (rawdata[0] & 0b00001000) != 0;
  electrodeOORBuf[4] = (rawdata[0] & 0b00010000) != 0;
  electrodeOORBuf[5] = (rawdata[0] & 0b00100000) != 0;
  electrodeOORBuf[6] = (rawdata[0] & 0b01000000) != 0;
  electrodeOORBuf[7] = (rawdata[0] & 0b10000000) != 0;
  
  electrodeOORBuf[8] = (rawdata[1] & 0b00000001) != 0;
  electrodeOORBuf[9] = (rawdata[1] & 0b00000010) != 0;
  electrodeOORBuf[10] = (rawdata[1] & 0b00000100) != 0;
  electrodeOORBuf[11] = (rawdata[1] & 0b00001000) != 0;
  electrodeOORBuf[12] = (rawdata[1] & 0b00010000) != 0;
  
  electrodeTouchBuf[13] = (rawdata[1] & 0b10000000) != 0;
  electrodeTouchBuf[14] = (rawdata[1] & 0b01000000) != 0;

  return electrodeTouchBuf;
}
  

// read filtered data for consecutive electrodes
short* mpr121::readElectrodeData(byte electrode, byte count) {
  if (electrode > 12)
    return electrodeDataBuf;

  if (electrode + count > 13)
    count = 13 - electrode;

  for (int i = 0; i < count; i++) {
    byte* rawdata = readRegister((mpr121Register)(0x04 + (i+electrode)*2), 2);
    electrodeDataBuf[i] = rawdata[0] | ((rawdata[1] & 0b00000011) << 8);
  }

  return electrodeDataBuf;
}
  
// read baseline values for consecutive electrodes
byte* mpr121::readElectrodeBaseline(byte electrode, byte count) {
  if (electrode > 12)
    return electrodeBaselineBuf;

  if (electrode + count > 13)
    count = 13 - electrode;

  for (int i = 0; i < count; i++) {
    electrodeBaselineBuf[i] = readRegister((mpr121Register)(0x1e + (i+electrode)));
  }

  return electrodeBaselineBuf;
}

// write baseline value for consecutive electrodes
void mpr121::writeElectrodeBaseline(byte electrode, byte count, byte value) {
  if (electrode > 12)
    return;

  if (electrode + count > 13)
    count = 13 - electrode;

  for (int i = 0; i < count; i++) {
    writeRegister((mpr121Register)(0x1e + (i+electrode)), value);
  }
}


// apply settings and enter run mode with a set number of electrodes
void mpr121::startMPR(byte electrodes) {
  // restrict value of numeric properties with < 8 bits to actual sent values
  MHDrising &= 0b00111111;
  MHDfalling &= 0b00111111;
  NHDrising &= 0b00111111;
  NHDfalling &= 0b00111111;
  NHDtouched &= 0b00111111;
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

