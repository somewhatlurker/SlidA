/*
 * This is an implementation of sega's arcade touch slider protocol,
 * used on Project DIVA Arcade: Future Tone and Chunithm.
 * 
 * Requires a dedicated Serial_ or Stream (such as `Serial`) to operate.
 * (Stream with a define changed)
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

// pro micro ~~should only have a 64 byte internal buffer~~
//   scratch that, USB CDC behaves differently to I thought and it doesn't really buffer like that...
//   64 bytes is the endpoint size though
// don't make this larger than 255
#define SLIDER_SERIAL_BUF_SIZE 200

// use a better(?) check for serial being available
// pro micro only, and a little hacky
// trying it because of https://github.com/arduino/ArduinoCore-avr/issues/112 mentioning an issue with UEBCLX reading 0 during transfers
// (I've observed similar, but it's unclear if it's due to the same cause)
// this seems to do very little so I'll leave it disabled
#define SLIDER_SERIAL_RECEIVE_CHECK_RWAL false

// wait until X ms after last received byte before returning from readSerial
#define SLIDER_SERIAL_RECEIVE_TIMEOUT 1

// wait maximum of X ms from starting to receive bytes before returning from readSerial (even if new bytes can still be read)
#define SLIDER_SERIAL_RECEIVE_MAX_MS 6

// wait maximum of X ms for there to be enough output capacity to send a packet
#define SLIDER_SERIAL_SEND_WAIT_MS 5

// use a Stream instead of the platform's serial class
// more portable (eg. can use on HardwareSerial with USB boards), but can't handle all host failures well
#define SLIDER_USE_STREAM false

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
  #if SLIDER_USE_STREAM
    typedef Stream streamtype;
  #else
    typedef decltype(Serial) streamtype;
  /*
  #elif defined(USBCON)
    typedef Serial_ streamtype;
  #elif defined(CORE_TEENSY)
    typedef usb_serial_class streamtype;
  #elif defined(HAVE_HWSERIAL0)
    typedef HardwareSerial streamtype;
  #else
    #error "Platform serial class unknown"
  */
  #endif
  
  streamtype* serialStream;

  byte serialInBuf[SLIDER_SERIAL_BUF_SIZE];
  byte serialInBufPos = 0;
  bool sendError;

  // some variables are used to convert incoming text to bytes if necessary
  #if SLIDER_SERIAL_TEXT_MODE
    char serialTextBuf[8];
    byte serialTextReadlen = 0; // the number of chars already read into serialTextBuf
  #endif // SLIDER_SERIAL_TEXT_MODE
  
  // sends a single escaped byte. return value is how much to adjust checksum by
  byte sendEscapedByte(byte data);

  // verify a packet's checksum is valid
  bool checkPacketSum(const sliderPacket packet, byte expectedSum);

  #if SLIDER_SERIAL_TEXT_MODE 
    // tries to read a text "byte" from serial
    // (returns -1 if needs more data)
    int tryReadSerialTextByte();
  #endif // SLIDER_SERIAL_TEXT_MODE 

  // check for serialStream->available() to be true
  // (or an equivalent function)
  bool checkReadAvailable();

  // read new serial data into the internal buffer
  // returns whether new data was available
  bool readSerial();
  
public:
  segaSlider(streamtype* serial = &Serial);
  
  // sends a slider packet (checksum is calculated automatically)
  // returns whether the packet was successfully sent
  bool sendPacket(const sliderPacket packet);

  // returns the slider packet from the serial buffer
  // invalid packets will have IsValid set to false
  //   - if `Command == (sliderCommand)0` it was probably caused by end of buffer and not corruption
  sliderPacket getPacket();
};
