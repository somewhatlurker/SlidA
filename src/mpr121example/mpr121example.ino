#define NUM_MPRS 3 // must be using sequential addresses
#define ANALOG_OUTPUT false
#define PROXIMITY_ENABLE false

#include "mpr121.h"

mpr121 mprs[NUM_MPRS] = NULL;

void setup() {
  Wire.begin();
  Wire.setClock(400000); // mpr121 can run in fast mode. if you have issues, try removing this line
  for (int i = 0; i < NUM_MPRS; i++) {
    mpr121 &mpr = mprs[i];
    
    // just sets up the Wire lib.
    // mpr121 can run in 400kHz mode. if you have issues with it or want to use 100kHz, use `mpr121.begin(100000)`.
    // (or use `Wire.begin` and/or `Wire.setClock` directly)
    mpr.begin(); 
    
    mpr = mpr121(0x5a + i);
    mpr.ESI = MPR_ESI_1; // get 4ms response time (4 samples * 1ms rate)
    mpr.autoConfigUSL = 256L * (3200 - 700) / 3200; // set autoconfig for 3.2V
    mpr.proxEnable = PROXIMITY_ENABLE ? MPR_ELEPROX_0_TO_11 : MPR_ELEPROX_DISABLED;
    mpr.start(12);
    //mpr.setGPIOMode(11, MPR_GPIO_MODE_OUTPUT_OPENDRAIN_HIGH);
    //mpr.writeGPIODigital(11, true);
    //mpr.writeGPIOAnalog(11, 8);
  }

  Serial.begin(115200);
  while(!Serial) {} // wait for serial to be ready on USB boards
}

void loop() {
  int numElectrodes = PROXIMITY_ENABLE ? 13 : 12;
  
  for (int i = 0; i < NUM_MPRS; i++) {
    mpr121 &mpr = mprs[i];
    if (ANALOG_OUTPUT) {
      short* values = mpr.readElectrodeData(0, numElectrodes); // read all electrodes, starting from electrode 0
      for (int j = 0; j < numElectrodes; j++)
      {
        Serial.print(values[j]);
        Serial.print(" ");
      }
    }
    else {
      #if MPR121_USE_BITFIELDS
        short touches = mpr.readTouchState();
        for (int j = 0; j < numElectrodes; j++)
          Serial.print(bitRead(touches, j));
      #else // MPR121_USE_BITFIELDS
        // you don't actually have to implement code for without bitfields, this is mainly for reference
        bool* touches = mpr.readTouchState();
        for (int j = 0; j < numElectrodes; j++)
          Serial.print(touches[j]);
      #endif // MPR121_USE_BITFIELDS
    }
    Serial.print(" ");
  }
  Serial.println();
  delay(10);
}
