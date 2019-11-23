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
 *   Wire.setClock(400000); // mpr121 can run in fast mode. if you have issues, try removing this line
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
#include <Wire.h>
#include "mpr121enums.h"

#ifndef MPR121_I2C_BUFLEN
#define MPR121_I2C_BUFLEN 26 // note: arduino Wire library defines BUFFER_LENGTH as 32, so much larger values won't work
#endif

class mpr121 {
private:
  byte i2cAddr;
  TwoWire* i2cWire;

  byte i2cReadBuf[MPR121_I2C_BUFLEN];
  bool electrodeTouchBuf[13];
  bool electrodeOORBuf[15];
  short electrodeDataBuf[13];
  byte electrodeBaselineBuf[13];
  
  // write a value to an MPR121 register
  void writeRegister(mpr121Register addr, byte value);

  
  // read bytes from consecutive MPR121 registers, starting at addr
  // max of 8 bytes
  byte* readRegister(mpr121Register addr, byte count);
  
  // read a byte from an MPR121 register
  byte readRegister(mpr121Register addr) {
    return readRegister(addr, 1)[0];
  }


  // set the touch and release thresholds for a subset of electrodes (AN3892)
  void setElectrodeThresholds(byte electrode, byte count, byte touchThreshold, byte releaseThreshold);
  
  // set the touch and release thresholds for a single electrode (AN3892)
  void setElectrodeThresholds(byte electrode, byte touchThreshold, byte releaseThreshold) {
    setElectrodeThresholds(electrode, 1, touchThreshold, releaseThreshold);
  }


  // set the "Max Half Delta" values (AN3891)
  // max: 63
  void setMHD(byte rising, byte falling);
  
  // set the "Noise Half Delta" values (AN3891)
  // max: 63
  void setNHD(byte rising, byte falling, byte touched);
  
  // set the "Noise Count Limit" values (AN3891)
  void setNCL(byte rising, byte falling, byte touched);
  
  // set the "Filter Delay Limit" values (AN3891)
  void setFDL(byte rising, byte falling, byte touched);


  // set the "Max Half Delta" values for proximity detection (AN3891/AN3893)
  // max: 63
  void setMHDProx(byte rising, byte falling);
  
  // set the "Noise Half Delta" values for proximity detection (AN3891/AN3893)
  // max: 63
  void setNHDProx(byte rising, byte falling, byte touched);
  
  // set the "Noise Count Limit" values for proximity detection (AN3891/AN3893)
  void setNCLProx(byte rising, byte falling, byte touched);
  
  // set the "Filter Delay Limit" values  for proximity detection(AN3891/AN3893)
  void setFDLProx(byte rising, byte falling, byte touched);

  
  // set "Debounce" counts
  // a detection must be held this many times before the status register is updated
  // max: 7
  void setDebounce(byte touchNumber, byte releaseNumber);


  // set "Filter Configuration" (AN3890)
  // FFI: "First Filter Iterations"
  // CDC: global "Charge Discharge Current" (μA), not used if autoconfig is enabled -- max 63, default 16
  // CDT: global "Charge Discharge Time" (μs), not used if autoconfig is enabled
  // SFI: "Second Filter Iterations"
  // ESI: "Electrode Sample Interval" (ms)
  // response time is equal to SFI * ESI in ms
  //   --set to 4 samples and 1 or 2 ms sample interval for fast response
  void setFilterConfig(mpr121FilterFFI FFI, byte CDC, mpr121FilterCDT CDT, mpr121FilterSFI SFI, mpr121FilterESI ESI);

  
  // set "Electrode Configuration"
  // CL: calibration lock (baseline tracking and initial value settings)
  // ELEPROX_EN: sets electrodes to be used for proximity detection
  // ELE_EN: sets the number of electrodes to be detected, starting from ELE0 (0 to disable)
  //   --setting this will enter run mode
  void setElectrodeConfiguration(mpr121ElectrodeConfigCL CL, mpr121ElectrodeConfigProx ELEPROX_EN, byte ELE_EN);

  
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
  void setAutoConfig(byte USL, byte LSL, byte TL, mpr121AutoConfigRetry RETRY, mpr121AutoConfigBVA BVA, bool ARE, bool ACE);
  
public:
  // create an MPR121 device with sane default settings
  mpr121(byte addr, TwoWire *wire = &Wire);

  byte touchThresholds[13]; // ELE0-ELE11, ELEPROX
  byte releaseThresholds[13]; // ELE0-ELE11, ELEPROX

  byte MHDrising; // "Max Half Delta" rising value (AN3891) -- max: 63
  byte MHDfalling; // "Max Half Delta" falling value (AN3891) -- max: 63
  
  byte NHDrising; // "Noise Half Delta" rising value (AN3891) -- max: 63
  byte NHDfalling; // "Noise Half Delta" falling value (AN3891) -- max: 63
  byte NHDtouched; // "Noise Half Delta" touched value (AN3891) -- max: 63
  
  byte NCLrising; // "Noise Count Limit" rising value (AN3891)
  byte NCLfalling; // "Noise Count Limit" falling value (AN3891)
  byte NCLtouched; // "Noise Count Limit" touched value (AN3891)
  
  byte FDLrising; // "Filter Delay Limit" rising value (AN3891)
  byte FDLfalling; // "Filter Delay Limit" falling value (AN3891)
  byte FDLtouched; // "Filter Delay Limit" touched value (AN3891)

  
  byte MHDrisingProx; // "Max Half Delta" rising value for proximity detection (AN3891/AN3893) -- max: 63
  byte MHDfallingProx; // "Max Half Delta" falling value for proximity detection (AN3891/AN3893) -- max: 63
  
  byte NHDrisingProx; // "Noise Half Delta" rising value for proximity detection (AN3891/AN3893) -- max: 63
  byte NHDfallingProx; // "Noise Half Delta" falling value for proximity detection (AN3891/AN3893) -- max: 63
  byte NHDtouchedProx; // "Noise Half Delta" touched value for proximity detection (AN3891/AN3893) -- max: 63
  
  byte NCLrisingProx; // "Noise Count Limit" rising value for proximity detection (AN3891/AN3893)
  byte NCLfallingProx; // "Noise Count Limit" falling value for proximity detection (AN3891/AN3893)
  byte NCLtouchedProx; // "Noise Count Limit" touched value for proximity detection (AN3891/AN3893)
  
  byte FDLrisingProx; // "Filter Delay Limit" rising value for proximity detection (AN3891/AN3893)
  byte FDLfallingProx; // "Filter Delay Limit" falling value for proximity detection (AN3891/AN3893)
  byte FDLtouchedProx; // "Filter Delay Limit" touched value for proximity detection (AN3891/AN3893)


  byte debounceTouch; // set "Debounce" count for touch (times a detection must be sampled) -- max: 7
  byte debounceRelease; // set "Debounce" count for release (times a detection must be sampled) -- max: 7


  mpr121FilterFFI FFI; // "First Filter Iterations" (number of samples taken for the first level of filtering)
  byte globalCDC; // global "Charge Discharge Current" (μA), not used if autoconfig is enabled -- max 63
  mpr121FilterCDT globalCDT; // global "Charge Discharge Time" (μs), not used if autoconfig is enabled
  mpr121FilterSFI SFI; // "Second Filter Iterations" (number of samples taken for the second level of filtering)
  mpr121FilterESI ESI; // "Electrode Sample Interval" (ms)
  
  // TODO: INDIVIDUAL CHARGE CURRENT/TIME


  mpr121ElectrodeConfigCL calLock; // "Calibration Lock" (baseline tracking and initial value settings)
  mpr121ElectrodeConfigProx proxEnable; // ELEPROX_EN: sets electrodes to be used for proximity detection

  byte autoConfigUSL; // "Up-Side Limit" for auto calibration -- if not set when starting, this will be set to the ideal value for 1.8V supply
  byte autoConfigLSL; // "Low-Side Limit" for auto calibration -- if not set when starting, this will be set based on USL
  byte autoConfigTL; // "Target Level" for auto calibration -- if not set when starting, this will be set based on USL
  mpr121AutoConfigRetry autoConfigRetry; // number of retries for failed config before setting out of range
  mpr121AutoConfigBVA autoConfigBaselineAdjust; // "Baseline Value Adjust" changes how the baseline registers will be set after auto-configuration completes
  bool autoConfigEnableReconfig; // "Automatic Reconfiguration Enable" will reconfigure out of range (failed) channels every sampling interval
  bool autoConfigEnableCalibration; // "Automatic Configuration Enable" will enable/disable auto-configuration when entering run mode

  // auto-conrig control register 1 stuff isn't implemented


  // TODO: GPIO


  // read the 13 touch state bools
  bool* readTouchState();

  // check the over current flag
  bool readOverCurrent();

  // clear the over current flag
  void clearOverCurrent();

  // read the 15 out of range bools
  // [13]: auto-config fail flag
  // [14]: auto-reconfig fail flag
  bool* readOORState();

  // read filtered data for consecutive electrodes
  short* readElectrodeData(byte electrode, byte count);
  
  // read filtered data for a single electrodes
  short readElectrodeData(byte electrode) {
    return readElectrodeData(electrode, 1)[0];
  }
  
  // read baseline values for consecutive electrodes
  byte* readElectrodeBaseline(byte electrode, byte count);
  
  // read baseline value for a single electrodes
  byte readElectrodeBaseline(byte electrode) {
    return readElectrodeBaseline(electrode, 1)[0];
  }

  // write baseline value for consecutive electrodes
  void writeElectrodeBaseline(byte electrode, byte count, byte value);
  
  // write baseline value for a single electrodes
  void writeElectrodeBaseline(byte electrode, byte value) {
    writeElectrodeBaseline(electrode, 1, value);
  }

  // easy way to set touchThresholds and releaseThresholds
  // prox sets whether to set for proximity detection too
  void setAllThresholds(byte touched, byte released, bool prox);


  // apply settings and enter run mode with a set number of electrodes
  void startMPR(byte electrodes);

  // exit run mode
  void stopMPR();
  
  // reset the mpr121
  void softReset();
};

