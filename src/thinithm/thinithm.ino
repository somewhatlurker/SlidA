#include "segaSlider.h"
#include "sliderdefs.h"
#include "mpr121.h"
#include "airTower.h"
#include "pins.h"
#include <FastLED.h>
#include <Keyboard.h>


// disable input scanning and generate fake slider data instead
// (useful for testing protocol without full hardware)
#define FAKE_DATA true


// sleep for x microseconds after running the main loop
#define LOOP_SLEEP_US 1000

// update air readings every x loops
// (air is less sensitive to timing so can be updated less often if necessary)
#define AIR_UPDATE_DUTY 1


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
};
errorState operator |(errorState a, errorState b)
{
    return static_cast<errorState>(static_cast<int>(a) | static_cast<int>(b));
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

#define STATUS_LED_BASIC_1_ERRORS (ERRORSTATE_SERIAL_TIMEOUT | ERRORSTATE_PACKET_CHECKSUM)
#define STATUS_LED_BASIC_2_ERRORS (ERRORSTATE_PACKET_OK)


// slider serial protocol implementation
segaSlider sliderProtocol = segaSlider(&Serial);


// slider board type for remapping
sliderBoardType curSliderMode;


// slider protocol packets for reuse
byte sliderBuf[32];
sliderPacket scanPacket = { SLIDER_SCAN_REPORT, sliderBuf, 32, true };

byte emptyBytes[0];
sliderPacket emptyPacket = { (sliderCommand)0, emptyBytes, 0, true }; // command will be replaced as necessary


const char BOARD_MODEL_DIVA[sizeof(boardInfo::model)] = { '1', '5', '2', '7', '5', ' ', ' ', ' ' };
const char BOARD_CHIP_DIVA[sizeof(boardInfo::chipNumber)] = { '0', '6', '6', '8', '7' };
const char BOARD_MODEL_CHUNI[sizeof(boardInfo::model)] = { '1', '5', '3', '3', '0', ' ', ' ', ' ' };
const char BOARD_CHIP_CHUNI[sizeof(boardInfo::chipNumber)] = { '0', '6', '7', '1', '2' };
boardInfo boardInfoData;
sliderPacket boardinfoPacket = { SLIDER_BOARDINFO, (byte*)&boardInfoData, sizeof(boardInfo), true };


// mpr121s
#define NUM_MPRS 4
mpr121 mprs[NUM_MPRS] = { mpr121(0x5a), mpr121(0x5b), mpr121(0x5c), mpr121(0x5d) };


// air tower
airTower airTower({ {PIN_AIRLED_1, PIN_AIRLED_2, PIN_AIRLED_3}, {PIN_AIRSENSOR_1, PIN_AIRSENSOR_2, PIN_AIRSENSOR_3, PIN_AIRSENSOR_4, PIN_AIRSENSOR_5, PIN_AIRSENSOR_6} });


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


  Serial.setTimeout(0.01);
  Serial.begin(115200);
  while(!Serial) {} // wait for serial to be ready on USB boards
}


bool scanOn = false;

// enable or disable slider and air scanning
void setScanning(bool on_off) {
  if (on_off) {
    scanOn = true;
    #if !FAKE_DATA
      for (mpr121 &mpr : mprs) {
        mpr.startMPR(12);
      }
      airTower.calibrate();
      Keyboard.begin();
    #endif // !FAKE_DATA
  }
  else {
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

#if FAKE_DATA
  int curSliderPatternByte; // used for fake test data
#endif

// perform a slider scan and send it to sliderProtocol
void doSliderScan() {
  // clear the output buffer
  memset(sliderBuf, 0, sizeof(sliderBuf));
  
  #if FAKE_DATA
    //sliderBuf[0] = (millis() % 1000) < 150 ? 0xC0 : 0x00;
    curSliderPatternByte = (millis()/30) % 32;
    sliderBuf[curSliderPatternByte] = 0xC0;
  #else // FAKE_DATA
    static const int numInputTouches = 12 * NUM_MPRS;
    static bool allTouches[numInputTouches];
  
    // read all mpr touches into allTouches
    for (int i = 0; i < NUM_MPRS; i++) {
      mpr121 &mpr = mprs[i];
      bool* touches = mpr.readTouchState();
      memcpy(&allTouches[12*i], touches, sizeof(bool[12]));
    }
  
    // apply touch data to output buffer
    for (int i = 0; i < allSliderDefs[curSliderMode]->keyCount && i < sizeof(sliderBuf); i++) { // for all keys, with bounds limited
      for (int j = 0; j < SLIDER_BOARDS_INPUT_KEYS_PER_OUTPUT; j++) { // for all inputs that may contribute
        int inputPos = allSliderDefs[curSliderMode]->keyMap[i][j];
  
        if (inputPos < numInputTouches && allTouches[inputPos]) { // check the result to read is in-range
          sliderBuf[i] |=  0xC0; // note this uses bitwise or to stack nicely
        }
      }
    }
  #endif // FAKE_DATA
  
  sliderProtocol.sendSliderPacket(scanPacket);

  #if FAKE_DATA
    sliderBuf[curSliderPatternByte] = 0x00;
  #endif
}

// perform an air sensor scan and send it to Keyboard
void doAirScan() {
  #if !FAKE_DATA
    static const char airKeys[6] = {'/', '.', '\'', ';', ']', '['};
    Keyboard.releaseAll();
  
    for (int i = 0; i < 6; i++) {
      if (airTower.checkLevel(i))
         Keyboard.press(airKeys[i]);
    }
  #endif // !FAKE_DATA
}


// vars used in main loop
int sleepTime = LOOP_SLEEP_US;
unsigned long lastSliderSendMillis;
unsigned long lastSerialRecvMillis = -5000;
unsigned long loopCount = 0;

void loop() {
  // clear errors (they'll be reset if necessary)
  curError = ERRORSTATE_NONE;

  //curSliderMode = (digitalRead(PIN_MODESEL) == LOW) ? SLIDER_TYPE_DIVA : SLIDER_TYPE_CHUNI;


  // check for new slider data
  // (in an if to save processing if nothing was available)
  if (sliderProtocol.readSerial()) {
    lastSerialRecvMillis = millis();
    
    // while packets can be read, process them
    sliderPacket pkt;
    while (true) {
      pkt = sliderProtocol.readNextPacket();
      
      if (!pkt.IsValid) {
        // if `Command == (sliderCommand)0` it was probably caused by end of buffer and not corruption
        if (pkt.Command != (sliderCommand)0)
          curError |= ERRORSTATE_PACKET_CHECKSUM;
        
        // break on all invalid packets as a precaution against the above check being wrong
        // (should rarely trigger and it shouldn't matter if some data is lost)
        break;
      }

      curError |= ERRORSTATE_PACKET_OK;

      // handle the incoming packet because it was valid
      switch(pkt.Command) {
        case SLIDER_BOARDINFO:
          switch(curSliderMode) {
            case SLIDER_TYPE_DIVA:
              memcpy(boardInfoData.model, BOARD_MODEL_DIVA, sizeof(boardInfo::model));
              memcpy(boardInfoData.chipNumber, BOARD_CHIP_DIVA, sizeof(boardInfo::chipNumber));
              break;
            default:
              memcpy(boardInfoData.model, BOARD_MODEL_CHUNI, sizeof(boardInfo::model));
              memcpy(boardInfoData.chipNumber, BOARD_CHIP_CHUNI, sizeof(boardInfo::chipNumber));
              break;
          }
          sliderProtocol.sendSliderPacket(boardinfoPacket);
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
          setScanning(true);
          break; // no response needed
          
        case SLIDER_SCAN_OFF:
          setScanning(false);
          emptyPacket.Command = SLIDER_SCAN_OFF;
          sliderProtocol.sendSliderPacket(emptyPacket);
          break;
          
        case SLIDER_LED:
          if (pkt.DataLength > 0) {
            FastLED.setBrightness(pkt.Data[0] * RGB_BRIGHTNESS / 63); // this seems to max out at 0x3f (63), use that for division

            int maxPacketLeds = (pkt.DataLength - 1) / 3; // subtract 1 because of brightness byte
            
            for (int i = 0; i < allSliderDefs[curSliderMode]->ledCount; i++) {
              int outputLed = allSliderDefs[curSliderMode]->ledMap[i];

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
            FastLED.show();
          }
          break; // no response needed
          
        default:
          emptyPacket.Command = pkt.Command; // just blindly acknowledge unknown commands
          sliderProtocol.sendSliderPacket(emptyPacket);
      }
    }
  }

  // disable scan and set error if serial is dead
  if ((millis() - lastSerialRecvMillis) > 5000) {
    if (scanOn) {
      setScanning(false);
    }
    curError |= ERRORSTATE_SERIAL_TIMEOUT;

    
    // set mode colour for 5s (this should trigger at boot)
    // also use this to black out slider
    if ((millis() - lastSerialRecvMillis) < 10000) {
      FastLED.setBrightness(RGB_BRIGHTNESS);

      for (int i = 0; i < NUM_SLIDER_LEDS; i++) {
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

      FastLED.show();
    }
    else {
      if (sliderLeds[MODE_LED_RGB_INDEX] != (CRGB)CRGB::Black) {
        sliderLeds[MODE_LED_RGB_INDEX] = CRGB::Black;
        FastLED.show();
      }
    }
  }

  
  // if slider scanning is on, update inputs (as appropriate)
  if (scanOn) {
    
    // if slider touch state has changed (interrupt was triggered), send data
    // also send as a keep alive if slider touch state hasn't changed recently
    if ( (digitalRead(PIN_SLIDER_IRQ) == LOW) || ((millis() - lastSliderSendMillis) > 250) ) {
      doSliderScan();
      lastSliderSendMillis = millis();
    }


    // if air should be updated this loop, update it
    if (loopCount % AIR_UPDATE_DUTY == 0) {
      doAirScan();
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
  delayMicroseconds(sleepTime);
}
