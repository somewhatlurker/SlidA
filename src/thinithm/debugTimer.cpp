#include "debugTimer.h"

void debugTimer::reset() {
  lastMicros = 0;
  minMicros = 0;
  maxMicros = 0;
  totalMicros = 0;
  nSamples = 0;
}

void debugTimer::log() {
  if (lastMicros == 0) {
    lastMicros = micros();
  }
  else {
    unsigned long t = micros() - lastMicros;
    lastMicros += t;
    
    if (minMicros > t)
      minMicros = t;

    if (maxMicros < t)
      maxMicros = t;

    totalMicros += t;
    nSamples++;
  }
}

