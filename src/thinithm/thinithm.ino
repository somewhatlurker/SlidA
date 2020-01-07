#include "segaSlider.h"
#include "sliderdefs.h"
#include "mpr121.h"
#include "airTower.h"
#include "pins.h"
#include <FastLED.h>
#include <Keyboard.h>


// disable input scanning and generate fake slider data instead
// (useful for testing protocol without full hardware)
#define FAKE_DATA false
#define FAKE_DATA_TYPE_CHASE 0
#define FAKE_DATA_TYPE_PULSE 1
#define FAKE_DATA_TYPE_TIMERS 2
#define FAKE_DATA_TYPE FAKE_DATA_TYPE_TIMERS


#if FAKE_DATA && FAKE_DATA_TYPE == FAKE_DATA_TYPE_TIMERS
  #include "debugTimer.h"
  debugTimer loopTimer;
  debugTimer sendTimer;
#endif


// slider LED vars
#define NUM_SLIDER_LEDS 32
CRGB sliderLeds[NUM_SLIDER_LEDS];

#define RGB_BRIGHTNESS 127

#define MODE_LED_RGB_INDEX NUM_SLIDER_LEDS-1


// status/error LED state
enum errorState : byte {
  ERRORSTATE_NONE = 0, // no error, used for resetting state
  ERRORSTATE_SERIAL_TIMEOUT = 1, // not receiving serial data (timeout)
  ERRORSTATE_PACKET_CHECKSUM = 2, // at least one invalid packet (due to checksum error) was received
  ERRORSTATE_PACKET_OK = 4, // at least one valid packet was received
  ERRORSTATE_PACKET_MAX_REACHED = 8, // number of packets exceeded MAX_PACKETS_PER_LOOP
};
errorState operator |(errorState a, errorState b)
{
    return static_cast<errorState>(static_cast<byte>(a) | static_cast<byte>(b));
}
errorState operator |=(errorState &a, errorState b)
{
    return a = (a | b);
}

errorState curError;


// status/error LED pins
#ifdef LED_BUILTIN_RX
  #define STATUS_LED_BASIC_1_PIN LED_BUILTIN_RX // pro micro has no LED_BUILTIN, so use the RX led
  #define STATUS_LED_BASIC_1_USES_RXTX 1
#else
  #define STATUS_LED_BASIC_1_PIN LED_BUILTIN
#endif

#ifdef LED_BUILTIN_TX
  #define STATUS_LED_BASIC_2_PIN LED_BUILTIN_TX // pro micro has no LED_BUILTIN, so use the TX led
  #define STATUS_LED_BASIC_2_USES_RXTX 1
#else
  #define STATUS_LED_BASIC_2_PIN LED_BUILTIN
#endif

#define STATUS_LED_BASIC_1_ERRORS (ERRORSTATE_SERIAL_TIMEOUT | ERRORSTATE_PACKET_CHECKSUM | ERRORSTATE_PACKET_MAX_REACHED)
#define STATUS_LED_BASIC_2_ERRORS (ERRORSTATE_PACKET_OK)


// slider serial protocol implementation
segaSlider sliderProtocol = segaSlider(&Serial);


// slider board type for remapping
sliderBoardType curSliderMode = SLIDER_TYPE_DIVA;
sliderDef* curSliderDef = allSliderDefs[curSliderMode];


// slider protocol packets for reuse
byte sliderBuf[32];
sliderPacket scanPacket = { SLIDER_SCAN_REPORT, sliderBuf, 32, true };

byte emptyBytes[0];
sliderPacket emptyPacket = { (sliderCommand)0, emptyBytes, 0, true }; // command will be replaced as necessary

boardInfo boardInfoData;
sliderPacket boardinfoPacket = { SLIDER_BOARDINFO, (byte*)&boardInfoData, sizeof(boardInfo), true };


// mpr121s
#define NUM_MPRS 1
mpr121 mprs[NUM_MPRS] = { mpr121(0x5a) }; //, mpr121(0x5b), mpr121(0x5c), mpr121(0x5d) };


// air tower
airTower airTower({ {PIN_AIRLED_1, PIN_AIRLED_2, PIN_AIRLED_3}, {PIN_AIRSENSOR_1, PIN_AIRSENSOR_2, PIN_AIRSENSOR_3, PIN_AIRSENSOR_4, PIN_AIRSENSOR_5, PIN_AIRSENSOR_6} });


// loop timing/processing stuff

// sleep for x microseconds after running the main loop
#define LOOP_SLEEP_US 333

// update air readings every x loops
// (air is less sensitive to timing so can be updated less often if necessary)
#define AIR_UPDATE_DUTY 1

// time to wait after losing serial connection before disabling scan and LEDs
#define SERIAL_TIMEOUT_MS 10000

// maximum number of packets to process in one loop
#define MAX_PACKETS_PER_LOOP 2


void setup() {
  // set pin modes for stuff that's handled in the main sketch file
  pinMode(STATUS_LED_BASIC_1_PIN, OUTPUT);
  pinMode(STATUS_LED_BASIC_2_PIN, OUTPUT);
  pinMode(PIN_MODESEL, INPUT_PULLUP);
  pinMode(PIN_SLIDER_IRQ, INPUT);

  // turn on led during setup
  // not gonna bother shifting this to the status lights
  digitalWrite(LED_BUILTIN, LOW);


  #if !FAKE_DATA
    delay(10); // make sure MPR121s have booted
    Wire.begin();
    Wire.setClock(400000); // mpr121 can run in fast mode. if you have issues, try removing this line
    for (mpr121 &mpr : mprs) {
      mpr.ESI = MPR_ESI_1; // get 4ms response time (4 samples * 1ms rate)
      mpr.autoConfigUSL = 256L * (3200 - 700) / 3200; // set autoconfig for 3.2V
    }
  #endif // !FAKE_DATA


  // SK6812 should be WS2812(B) compatible, but FastLED has it natively anyway
  FastLED.addLeds<SK6812, PIN_SLIDER_LEDIN, GRB>(sliderLeds, NUM_SLIDER_LEDS);


  Serial.setTimeout(0);
  Serial.begin(115200);
  while(!Serial) {} // wait for serial to be ready on USB boards
}


bool scanOn = false;

// enable or disable slider and air scanning
void setScanning(bool on_off) {
  if (on_off && !scanOn) {
    scanOn = true;
    #if !FAKE_DATA
      for (mpr121 &mpr : mprs) {
        mpr.startMPR(12);
      }
      airTower.calibrate();
      Keyboard.begin();
    #endif // !FAKE_DATA
  }
  else if (!on_off && scanOn) {
    scanOn = false;
    #if !FAKE_DATA
      for (mpr121 &mpr : mprs) {
        mpr.stopMPR();
      }
      Keyboard.releaseAll();
      Keyboard.end();
    #endif // !FAKE_DATA
  }
}

// perform a slider scan and send it to sliderProtocol
void doSliderScan() {
  // clear the output buffer
  memset(sliderBuf, 0, sizeof(sliderBuf));
  
  #if FAKE_DATA
    #if FAKE_DATA_TYPE == FAKE_DATA_TYPE_CHASE
      sliderBuf[(millis()/30) % 32] = 0xC0;
    #elif FAKE_DATA_TYPE == FAKE_DATA_TYPE_PULSE
      sliderBuf[0] = (millis() % 1000) < 150 ? 0xC0 : 0x00;
    #elif FAKE_DATA_TYPE == FAKE_DATA_TYPE_TIMERS
      sendTimer.log();
      
      sliderBuf[0] = loopTimer.getMinMillis();
      sliderBuf[1] = loopTimer.getAverageMillis();
      sliderBuf[2] = loopTimer.getMaxMillis();
      
      sliderBuf[4] = sendTimer.getMinMillis();
      sliderBuf[5] = sendTimer.getAverageMillis();
      sliderBuf[6] = sendTimer.getMaxMillis();

      static unsigned long lastResetMillis;
      if (millis() - lastResetMillis > 3000) {
        sendTimer.reset();
        loopTimer.reset();
        lastResetMillis = millis();
      }
    #endif
  #else // FAKE_DATA
    static const byte numInputTouches = 12 * NUM_MPRS;
    static bool allTouches[numInputTouches];
  
    // read all mpr touches into allTouches
    for (byte i = 0; i < NUM_MPRS; i++) {
      mpr121 &mpr = mprs[i];
      #if MPR121_USE_BITFIELDS
        short touches = mpr.readTouchState();
        for (byte j = 0; j < 12; j++) {
          allTouches[12*i + j] = bitRead(touches, j);
        }
      #else // MPR121_USE_BITFIELDS
        bool* touches = mpr.readTouchState();
        memcpy(&allTouches[12*i], touches, sizeof(bool[12]));
      #endif // MPR121_USE_BITFIELDS
    }
  
    // apply touch data to output buffer
    for (byte i = 0; i < curSliderDef->keyCount && i < sizeof(sliderBuf); i++) { // for all keys, with bounds limited
      for (byte j = 0; j < curSliderDef->inputsPerKey; j++) { // for all inputs that may contribute
        byte inputPos = curSliderDef->keyMap[i][j];
  
        if (inputPos < numInputTouches && allTouches[inputPos]) { // check the result to read is in-range
          sliderBuf[i] |=  0xC0; // note this uses bitwise or to stack nicely
        }
      }
    }
  #endif // FAKE_DATA
  
  sliderProtocol.sendPacket(scanPacket);
}

// perform an air sensor scan and send it to Keyboard
void doAirScan() {
  #if !FAKE_DATA
    static const char airKeys[6] = {'/', '.', '\'', ';', ']', '['};
    Keyboard.releaseAll();
  
    for (byte i = 0; i < 6; i++) {
      if (airTower.checkLevel(i))
         Keyboard.press(airKeys[i]);
    }
  #endif // !FAKE_DATA
}


// timing vars used in main loop
int sleepTime = LOOP_SLEEP_US;
unsigned long lastSliderSendMillis;
unsigned long lastSerialRecvMillis = -SERIAL_TIMEOUT_MS;
unsigned long loopCount = 0;

bool ledUpdate = false;

void loop() {
  // clear errors (they'll be reset if necessary)
  curError = ERRORSTATE_NONE;

  //curSliderMode = (digitalRead(PIN_MODESEL) == LOW) ? SLIDER_TYPE_DIVA : SLIDER_TYPE_CHUNI;
  curSliderDef = allSliderDefs[curSliderMode];

  // check for new slider data
  byte pktCount = 0;
  while (pktCount < MAX_PACKETS_PER_LOOP && sliderProtocol.readSerial()) {
    lastSerialRecvMillis = millis();
    
    sliderPacket pkt = sliderProtocol.getPacket();
      
    if (!pkt.IsValid) {
      // if `Command == (sliderCommand)0` it was probably caused by end of buffer and not corruption
      // (so if `!=` it was probably a checksum issue)
      if (pkt.Command == (sliderCommand)0) {
        break; // no point looping here if there's not enough data
      }
      else {
        curError |= ERRORSTATE_PACKET_CHECKSUM;
        pktCount++;
      }
      
      continue;
    }

    curError |= ERRORSTATE_PACKET_OK;
    pktCount++;

    // handle the incoming packet because it was valid
    switch(pkt.Command) {
      case SLIDER_BOARDINFO:
        memcpy(boardInfoData.model, curSliderDef->model, sizeof(boardInfo::model));
        memcpy(boardInfoData.chipNumber, curSliderDef->chipNumber, sizeof(boardInfo::chipNumber));
        sliderProtocol.sendPacket(boardinfoPacket);
        break;

      case SLIDER_SCAN_REPORT:
        // there's no way this will give accurate results,
        // but at least it's implemented on a protocol level now
        if (!scanOn) {
          setScanning(true);
          delay(10);
          doSliderScan();
          setScanning(false);
        }
        break; // doSliderScan() sends the response
        
      case SLIDER_SCAN_ON:
        sliderProtocol.sendPacket(scanPacket);
        setScanning(true);
        break; // no response needed
        
      case SLIDER_SCAN_OFF:
        setScanning(false);
        emptyPacket.Command = SLIDER_SCAN_OFF;
        sliderProtocol.sendPacket(emptyPacket);
        break;
        
      case SLIDER_LED:
        if (pkt.DataLength > 0) {
          FastLED.setBrightness((pkt.Data[0] & 0x3f) * RGB_BRIGHTNESS / 0x3f); // this seems to max out at 0x3f (63), use that for division

          byte maxPacketLeds = (pkt.DataLength - 1) / 3; // subtract 1 because of brightness byte
          
          for (byte i = 0; i < curSliderDef->ledCount; i++) {
            byte outputLed = curSliderDef->ledMap[i];

            if (outputLed < NUM_SLIDER_LEDS) { // make sure there's no out of bounds writes
              if (i < maxPacketLeds) {
                sliderLeds[outputLed].b = pkt.Data[i*3 + 1]; // start with + 1 because of brightness byte
                sliderLeds[outputLed].r = pkt.Data[i*3 + 2];
                sliderLeds[outputLed].g = pkt.Data[i*3 + 3];
              }
              else {
                sliderLeds[outputLed] = CRGB::Black;
              }
            }
          }
          ledUpdate = true;
        }
        break; // no response needed
        
      default:
        emptyPacket.Command = pkt.Command; // just blindly acknowledge unknown commands
        sliderProtocol.sendPacket(emptyPacket);
        break;
    }
  }
  if (pktCount == MAX_PACKETS_PER_LOOP)
    curError |= ERRORSTATE_PACKET_MAX_REACHED;

  // disable scan and set error if serial is dead
  if ((millis() - lastSerialRecvMillis) > SERIAL_TIMEOUT_MS) {
    if (scanOn) {
      setScanning(false);
    }
    curError |= ERRORSTATE_SERIAL_TIMEOUT;

    
    // set mode colour for 5s (this should trigger at boot)
    // also use this to black out slider
    if ((millis() - lastSerialRecvMillis) < SERIAL_TIMEOUT_MS + 5000) {
      FastLED.setBrightness(RGB_BRIGHTNESS);

      for (byte i = 0; i < NUM_SLIDER_LEDS; i++) {
        sliderLeds[i] = CRGB::Black;
      }
      
      switch(curSliderMode) {
        case SLIDER_TYPE_DIVA:
          sliderLeds[MODE_LED_RGB_INDEX] = CRGB::Teal;
          break;
        default:
          sliderLeds[MODE_LED_RGB_INDEX] = CRGB::Olive;
          break;
      }

      ledUpdate = true;
    }
    else {
      if (sliderLeds[MODE_LED_RGB_INDEX] != (CRGB)CRGB::Black) {
        sliderLeds[MODE_LED_RGB_INDEX] = CRGB::Black;
        ledUpdate = true;
      }
    }
  }

  // now update LEDs once per loop
  // done before scanning to reduce delay from receiving to showing
  if (ledUpdate) {
    FastLED.show();
    ledUpdate = false;
  }

  // if slider scanning is on, update inputs (as appropriate)
  if (scanOn) {
    
    // if slider touch state has changed (interrupt was triggered), send data
    // also send as a keep alive if slider touch state hasn't changed recently
    if ( /*(digitalRead(PIN_SLIDER_IRQ) == LOW) ||*/ ((millis() - lastSliderSendMillis) > 10) ) {
      lastSliderSendMillis = millis();
      doSliderScan();
    }

    if (curSliderDef->hasAir) {
      // if air should be updated this loop, update it
      if (loopCount % AIR_UPDATE_DUTY == 0) {
        doAirScan();
      }
    }
  }


  // set status leds
  if (STATUS_LED_BASIC_1_ERRORS & curError) {
    #ifdef STATUS_LED_BASIC_1_USES_RXTX
      pinMode(STATUS_LED_BASIC_1_PIN, OUTPUT); // set as OUTPUT to re-enable normal operation
    #endif
    digitalWrite(STATUS_LED_BASIC_1_PIN, LOW);
  }
  else {
    #ifdef STATUS_LED_BASIC_1_USES_RXTX
      pinMode(STATUS_LED_BASIC_1_PIN, INPUT); // set as INPUT to disable normal operation
    #endif
    digitalWrite(STATUS_LED_BASIC_1_PIN, HIGH);
  }

  if (STATUS_LED_BASIC_2_ERRORS & curError) {
    #ifdef STATUS_LED_BASIC_2_USES_RXTX
      pinMode(STATUS_LED_BASIC_2_PIN, OUTPUT); // set as OUTPUT to re-enable normal operation
    #endif
    digitalWrite(STATUS_LED_BASIC_2_PIN, LOW);
  }
  else {
    #ifdef STATUS_LED_BASIC_2_USES_RXTX
      pinMode(STATUS_LED_BASIC_2_PIN, INPUT); // set as INPUT to disable normal operation
    #endif
    digitalWrite(STATUS_LED_BASIC_2_PIN, HIGH);
  }

  loopCount++;

  #if FAKE_DATA && FAKE_DATA_TYPE == FAKE_DATA_TYPE_TIMERS
    loopTimer.log();
  #endif

  delayMicroseconds(sleepTime);
}
