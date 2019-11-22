/*
 * This is an implementation of sega's arcade touch slider protocol,
 * used on Project DIVA Arcade: Future Tone and Chunithm.
 * 
 * Copyright 2019 somewhatlurker, MIT license
 * 
 * Parsing and serial writing are handled in here,
 * but other code must read data into a buffer for it.
 * This may change in the future, but it works for now.
 */

#include "protocol.h"

// sends a single escaped byte. return value is how much to adjust checksum by
byte sendSliderByte(byte data) {
  if (data == SLIDER_FRAMING_ESCAPE) {
    Serial.write(SLIDER_FRAMING_ESCAPE);
    Serial.write(SLIDER_FRAMING_ESCAPE - 0x1);
  }
  else if (data == SLIDER_FRAMING_START) {
    Serial.write(SLIDER_FRAMING_ESCAPE);
    Serial.write(SLIDER_FRAMING_START - 0x1);
  }
  else {
    Serial.write(data);
  }
  return 0 - data;
}

// sends a complete slider packet (checksum is calculated automatically)
void sendSliderPacket(const sliderPacket packet) {
  byte checksum = 0;

  Serial.write(SLIDER_FRAMING_START); // packet start (should be sent raw)
  checksum -= SLIDER_FRAMING_START; // maybe should always be 0xFF..  not sure
  
  checksum += sendSliderByte((byte)packet.Command);

  checksum += sendSliderByte(packet.DataLength);
  
  for (byte i = 0; i < packet.DataLength; i++) {
    checksum += sendSliderByte(packet.Data[i]);
  }

  sendSliderByte(checksum);
}

// verify a packet's checksum is valid
bool checkSliderPacketSum(const sliderPacket packet, byte expectedSum) {
  byte checksum = 0;
  
  checksum -= SLIDER_FRAMING_START; // maybe should always be 0xFF..  not sure
  checksum -= (byte)packet.Command;
  checksum -= packet.DataLength;

  for (byte i = 0; i < packet.DataLength; i++) {
    checksum -= packet.Data[i];
  }

  return checksum == expectedSum;
}

// read and parse a packet from a ring buffer
// invalid packets will have IsValid set to false
// bufsize is the full buffer size
// bufpos is the position to start reading from and will be automatically updated
// can be used with a linear buffer if bufpos is set to 0
// maxpos should be set to the end of currently valid data
sliderPacket parseRawSliderData(const byte* data, int bufsize, int &bufpos, int maxpos) {
  int startOffset = -1;
  int endOffset = -1;
  
  static byte packetData[MAX_SLIDER_PACKET_SIZE];
  static sliderPacket outPkt;

  outPkt.Command = (sliderCommand)0;
  outPkt.DataLength = 0;
  outPkt.IsValid = false;

  bool forceAdvance = false; // used to force advance of bufpos, for when a known full but possibly malformed packet was seen (0xFF appeared twice)

  maxpos %= bufsize;

  // find the beginning and end of a packet
  for (int i = bufpos; i != (bufpos - 1) % bufsize && i != maxpos; i++, i %= bufsize) {
    if (data[i] == SLIDER_FRAMING_START) {
      if (startOffset == -1)
      {
        startOffset = i;
      }
      else
      {
        endOffset = i;
        forceAdvance = true;
        break;
      }
    }
  }
  if (startOffset == -1) return outPkt; // no data
  if (endOffset == -1) endOffset = maxpos; // if no second split point is found, use until maxpos

  // unescape and copy bytes
  int outpos = 0;
  for (int i = startOffset; i != endOffset && outpos < MAX_SLIDER_PACKET_SIZE; i++, i %= bufsize)
  {
    if (data[i] == SLIDER_FRAMING_ESCAPE) { // esscaped byte sequence
      i++; i %= bufsize; // skip SLIDER_FRAMING_ESCAPE
      packetData[outpos] = data[i] + 1;
    }
    else {
      packetData[outpos] = data[i];
    }
    outpos++;
  }

  outPkt.Command = (sliderCommand)packetData[1];
  outPkt.DataLength = packetData[2];
  outPkt.Data = &packetData[3];

  if (outPkt.DataLength > MAX_SLIDER_PACKET_SIZE + 3) // probably not necessary
    outPkt.IsValid = false;
  else
    outPkt.IsValid = checkSliderPacketSum(outPkt, packetData[outPkt.DataLength + 3]);

  if (forceAdvance || outPkt.IsValid) bufpos = endOffset;
  
  return outPkt;
}
