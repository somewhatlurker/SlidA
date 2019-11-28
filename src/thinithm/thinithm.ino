#include "segaSlider.h"
#include "sliderdefs.h"
#include "mpr121.h"

segaSlider sliderProtocol = segaSlider(&Serial);

sliderBoardType curSliderMode;

byte sliderBuf[32];
sliderPacket scanPacket;

byte emptyBytes[0];
sliderPacket emptyPacket;

// 8 byte model number (string with spaces for padding), 1 byte device class, 5 byte chip part number (string), 1 byte unknown, 1 byte fw version, 2 bytes unknown
// board "15275   ", class 0x0a, chip "06687", 0xff, ver 144, 0x00, 0x64
byte boardInfoDataDiva[18] = {0x31, 0x35, 0x32, 0x37, 0x35, 0x20, 0x20, 0x20, 0xa0, 0x30, 0x36, 0x36, 0x38, 0x37, 0xFF, 0x90, 0x00, 0x64};
// board "15330   ", class 0x0a, chip "06712", 0xff, ver 144, 0x00, 0x64
byte boardInfoDataChuni[18] = {0x31, 0x35, 0x33, 0x33, 0x30, 0x20, 0x20, 0x20, 0xa0, 0x30, 0x36, 0x37, 0x31, 0x32, 0xFF, 0x90, 0x00, 0x64};
sliderPacket boardinfoPacket;

//mpr121 mprs[4] = { mpr121(0x5a), mpr121(0x5b), mpr121(0x5c), mpr121(0x5d) };

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(3, INPUT);
  
  scanPacket.Command = SLIDER_SCAN_REPORT;
  scanPacket.Data = sliderBuf;
  scanPacket.DataLength = 32;
  scanPacket.IsValid = true;

  emptyPacket.Command = (sliderCommand)0; // this will be replaced as necessary
  emptyPacket.Data = emptyBytes;
  emptyPacket.DataLength = 0;
  emptyPacket.IsValid = true;

  boardinfoPacket.Command = SLIDER_BOARDINFO;
  //boardinfoPacket.Data = boardInfoData;
  boardinfoPacket.DataLength = 18;
  boardinfoPacket.IsValid = true;

  //Wire.begin();
  //Wire.setClock(400000); // mpr121 can run in fast mode. if you have issues, try removing this line
  //for (mpr121 mpr : mprs) {
  //  mpr.ESI = MPR_ESI_1; // get 4ms response time (4 samples * 1ms rate)
  //  mpr.autoConfigUSL = 256 * (3200 - 700) / 3200; // set autoconfig for 3.2V
  //}

  Serial.setTimeout(0.01);
  Serial.begin(115200);
}

int curSliderPatternByte;
bool scanOn = false;
int sleepTime = 1;

void loop() {
  if (sliderProtocol.readSerial()) {
    // while packets can be read, process them
    sliderPacket pkt;
    while (pkt = sliderProtocol.readNextPacket(), pkt.IsValid) {
      digitalWrite(LED_BUILTIN, LOW);
      
      switch(pkt.Command) {
        case SLIDER_BOARDINFO:
          switch(curSliderMode) {
            case SLIDER_TYPE_DIVA:
              boardinfoPacket.Data = boardInfoDataDiva;
              break;
            default:
              boardinfoPacket.Data = boardInfoDataChuni;
              break;
          }
          sliderProtocol.sendSliderPacket(boardinfoPacket);
          break;
        case SLIDER_SCAN_ON:
          scanOn = true;
          //for (mpr121 mpr : mprs) {
          //  mpr.startMPR(12);
          //}
          break; // no response needed
        case SLIDER_SCAN_OFF:
          scanOn = false;
          //for (mpr121 mpr : mprs) {
          //  mpr.stopMPR();
          //}
          emptyPacket.Command = SLIDER_SCAN_OFF;
          sliderProtocol.sendSliderPacket(emptyPacket);
          break;
        case SLIDER_LED:
          break; // no response needed
        default:
          emptyPacket.Command = pkt.Command; // just blindly acknowledge unknown commands
          sliderProtocol.sendSliderPacket(emptyPacket);
      }
    }
  }

  //sliderBuf[0] = (millis() % 1000) < 150 ? 0xC0 : 0x00;
  curSliderPatternByte = (millis()/30) % 32;
  sliderBuf[curSliderPatternByte] = 0xC0;
  if (scanOn) sliderProtocol.sendSliderPacket(scanPacket);
  sliderBuf[curSliderPatternByte] = 0x00;
  
  digitalWrite(LED_BUILTIN, HIGH);
  delay(sleepTime);
}
