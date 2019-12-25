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
// alternatively, SERIAL_RX_BUFFER_SIZE can be defined by the sketch
#define SLIDER_SERIAL_BUF_SIZE 256

// all known valid slider protocol commands (for use in sliderPacket)
enum sliderCommand {
  SLIDER_SCAN_REPORT = 0x01, // scan results are sent with this command -- 1 byte for each sensor
  SLIDER_LED = 0x02, // set led colours -- 1 byte (0-63?) brightness, followed by BRG data
  SLIDER_SCAN_ON = 0x03, // enable auto scan
  SLIDER_SCAN_OFF = 0x04, // disable auto scan
  SLIDER_UNKNOWN_09 = 0x09, // segatools says this is something about diva LEDs, but those are 0x02 like chuni
  SLIDER_UNKNOWN_0A = 0x0A,
  SLIDER_CALIBRATION = 0x0B, // just a guess, doesn't really matter
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
  int serialBufWritePos; // head
  int serialBufReadPos; // tail

  // some variables are used to convert incoming text to bytes if necessary
  #if SLIDER_SERIAL_TEXT_MODE
    char serialTextBuf[8];
    byte serialTextReadlen = 0; // the number of chars already read into serialTextBuf
  #endif // SLIDER_SERIAL_TEXT_MODE
  
  // sends a single escaped byte. return value is how much to adjust checksum by
  byte sendSliderByte(byte data);

  // verify a packet's checksum is valid
  bool checkSliderPacketSum(const sliderPacket packet, byte expectedSum);
  
  // read and parse a packet from a ring buffer
  // invalid packets will have IsValid set to false
  // datalen is the full buffer size
  // bufpos is the position to start reading from and will be automatically updated (tail)
  // can be used with a linear buffer if bufpos is set to 0
  // maxpos should be set to the end of currently valid data (head)
  // check for errors using IsValid on the output packet.
  //   if `Command == (sliderCommand)0` it was probably caused by end of buffer and not corruption
  sliderPacket parseRawSliderData(const byte* data, int datalen, int &bufpos, int maxpos);
  
public:
  segaSlider(Stream* serial = &Serial);
  
  // sends a complete slider packet (checksum is calculated automatically)
  void sendSliderPacket(const sliderPacket packet);

  // read new any new serial data into the internal buffer
  // returns whether new data was available
  bool readSerial();

  // returns the next slider packet from the serial buffer
  // check IsValid to see if all data has been read
  sliderPacket readNextPacket();
};
