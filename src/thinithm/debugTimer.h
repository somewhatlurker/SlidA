#pragma once
#include <Arduino.h>

class debugTimer {
private:
  unsigned long lastMicros;
  unsigned long minMicros;
  unsigned long maxMicros;
  unsigned long totalMicros;
  unsigned long nSamples;

public:
  debugTimer() {
    reset();
  }
  
  void reset();
  
  void log();
  
  unsigned long getMinMicros() {
    return minMicros;
  }
  unsigned long getMinMillis() {
    return minMicros / 1000;
  }

  unsigned long getMaxMicros() {
    return maxMicros;
  }
  unsigned long getMaxMillis() {
    return maxMicros / 1000;
  }

  unsigned long getAverageMicros() {
    return totalMicros / nSamples;
  }
  unsigned long getAverageMillis() {
    return totalMicros / (nSamples * 1000);
  }
};

