/*
 * This is an implementation of sega's arcade touch slider protocol,
 * used on Project DIVA Arcade: Future Tone and Chunithm.
 * 
 * Requires a dedicated stream (such as `Serial`) to operate.
 * 
 * Usage: see thinithm
 * 
 * Copyright 2019 somewhatlurker, MIT license
 */

 // protocol info: https://gist.github.com/dogtopus/b61992cfc383434deac5fab11a458597 (best)
 //                https://pastebin.com/wKME0Kkb (maybe a little easier to glance through)
 //                http://ryun.halfmoon.jp/touchslider/slider_protocol.html (original source of most info)

#pragma once
#include <Arduino.h>

#define SLIDER_FRAMING_START 0xFF
#define SLIDER_FRAMING_ESCAPE 0xFD

#define MAX_SLIDER_PACKET_SIZE 128 // max expected in practice is 100 (4 + 32*3) for LED data

#define SLIDER_SERIAL_TEXT_MODE false

// pro micro should only have a 64 byte internal buffer, but we can still combine multiple reads into one
// don't make this larger than 255
#define SLIDER_SERIAL_BUF_SIZE 200

// all known valid slider protocol commands (for use in sliderPacket)
enum sliderCommand {
  SLIDER_SCAN_REPORT = 0x01, // scan results are sent with this command -- 1 byte for each sensor
  SLIDER_LED = 0x02, // set led colours -- 1 byte (0-63?) brightness, followed by BRG data
  SLIDER_SCAN_ON = 0x03, // enable auto scan
  SLIDER_SCAN_OFF = 0x04, // disable auto scan
  SLIDER_REPORT_06 = 0x06, // some kind of report, looks a lot like 0B (with double length maybe??), so probably related to that (maybe diva only, seems unused)
  SLIDER_SCAN_07 = 0x07, // seems to be some kind of scan command related to the 06 report (maybe diva only, seems unused)
  SLIDER_UNKNOWN_09 = 0x09, // set some kind of parameter?? segatools says this is something about diva LEDs, but those are 0x02 like chuni (maybe diva only)
  SLIDER_UNKNOWN_0A = 0x0A, // set some kind of parameter?? (maybe diva only)
  SLIDER_REPORT_0B = 0x0B, // some kind of report, maybe related to calibration, doesn't really matter (maybe diva only, seems unused)
  SLIDER_SCAN_0C = 0x0C, // seems to be some kind of scan command related to the 0b report (maybe diva only, seems unused)
  SLIDER_DETECT = 0x10, // segatools calls this reset, but doesn't seem to actually reset anything in segatools
  SLIDER_EXCEPTION = 0xEE, // unimportant, but no reason not to include this
  SLIDER_BOARDINFO = 0xF0 // get slider board model, version, etc -- use boardInfo struct
};

// holds all data for a given data packet
struct sliderPacket { sliderCommand Command; const byte* Data; byte DataLength; bool IsValid; };

// define a struct for board info because it isn't just a simple array like other data
struct __attribute__((packed)) boardInfo { // packed should be safe because all members are one byte long data types
  char model[8]; // board model number (837-XXXXX), with spaces after for padding
  byte deviceClass = 0xa0; // aparrently this probably identifies the device class, but it's not known -- should be 0xa0 for slider
  char chipNumber[5]; // chip part number
  byte unk_0e = 0xff; // unknown purpose -- seems to just be 0xff
  byte fwVer = 0x90; // device firmware version...  both chuni and diva use 144 so maybe it's actually related to protocol version
  byte unk_10 = 0x00; // unknown purpose -- seems to just be 0x00
  byte unk_11 = 0x64; // unknown purpose -- seems to just be 0x64
};

class segaSlider {
private:
  Stream* serialStream;

  byte serialInBuf[SLIDER_SERIAL_BUF_SIZE]; // ring buf

  // some variables are used to convert incoming text to bytes if necessary
  #if SLIDER_SERIAL_TEXT_MODE
    char serialTextBuf[8];
    byte serialTextReadlen = 0; // the number of chars already read into serialTextBuf
  #endif // SLIDER_SERIAL_TEXT_MODE
  
  // sends a single escaped byte. return value is how much to adjust checksum by
  byte sendSliderByte(byte data);

  // verify a packet's checksum is valid
  bool checkSliderPacketSum(const sliderPacket packet, byte expectedSum);
  
public:
  segaSlider(Stream* serial = &Serial);
  
  // sends a complete slider packet (checksum is calculated automatically)
  void sendSliderPacket(const sliderPacket packet);

  // read new serial data into the internal buffer
  // returns whether new data was available
  bool readSerial();

  // returns the slider packet from the serial buffer
  // invalid packets will have IsValid set to false
  sliderPacket getPacket();
};
