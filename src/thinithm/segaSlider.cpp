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

#include "segaSlider.h"


// sends a single escaped byte. return value is how much to adjust checksum by
byte segaSlider::sendSliderByte(byte data) {
  if (SLIDER_SERIAL_TEXT_MODE) {
    if (data == SLIDER_FRAMING_ESCAPE) {
      serialStream->write(String(SLIDER_FRAMING_ESCAPE).c_str());
      serialStream->write(" ");
      serialStream->write(String(SLIDER_FRAMING_ESCAPE - 0x1).c_str());
      serialStream->write(" ");
    }
    else if (data == SLIDER_FRAMING_START) {
      serialStream->write(String(SLIDER_FRAMING_ESCAPE).c_str());
      serialStream->write(" ");
      serialStream->write(String(SLIDER_FRAMING_START - 0x1).c_str());
      serialStream->write(" ");
    }
    else {
      serialStream->write(String(data).c_str());
      serialStream->write(" ");
    }
  }
  else {
    if (data == SLIDER_FRAMING_ESCAPE) {
      serialStream->write(SLIDER_FRAMING_ESCAPE);
      serialStream->write(SLIDER_FRAMING_ESCAPE - 0x1);
    }
    else if (data == SLIDER_FRAMING_START) {
      serialStream->write(SLIDER_FRAMING_ESCAPE);
      serialStream->write(SLIDER_FRAMING_START - 0x1);
    }
    else {
      serialStream->write(data);
    }
  }
  return 0 - data;
}

// verify a packet's checksum is valid
bool segaSlider::checkSliderPacketSum(const sliderPacket packet, byte expectedSum) {
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
// bufpos is the position to start reading from and will be automatically updated (tail)
// can be used with a linear buffer if bufpos is set to 0
// maxpos should be set to the end of currently valid data (head)
// check for errors using IsValid on the output packet.
//   if `Command == (sliderCommand)0` it was probably caused by end of buffer and not corruption
sliderPacket segaSlider::parseRawSliderData(const byte* data, int bufsize, int &bufpos, int maxpos) {
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
  for (int i = bufpos; (i + 1) % bufsize != bufpos && i != maxpos; i++, i %= bufsize) {
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
  int outpos = 0; // define in this scope so it can be checked later
  for (int i = startOffset; i != endOffset && outpos < MAX_SLIDER_PACKET_SIZE; i++, i %= bufsize)
  {
    if (data[i] == SLIDER_FRAMING_ESCAPE) { // esscaped byte sequence
      i++; i %= bufsize; // skip SLIDER_FRAMING_ESCAPE
      if (i == endOffset) // check data hasn't ended
        break;
      packetData[outpos] = data[i] + 1;
    }
    else {
      packetData[outpos] = data[i];
    }
    outpos++;
  }

  outPkt.DataLength = packetData[2];

  if (outpos >= outPkt.DataLength) { // read enough data
    outPkt.Command = (sliderCommand)packetData[1];
    outPkt.Data = &packetData[3];

    outPkt.IsValid = checkSliderPacketSum(outPkt, packetData[outPkt.DataLength + 3]);
  }

  if (forceAdvance || outPkt.IsValid) bufpos = endOffset;
  
  return outPkt;
}

segaSlider::segaSlider(Stream* serial) {
  serialStream = serial;
}

// sends a complete slider packet (checksum is calculated automatically)
void segaSlider::sendSliderPacket(const sliderPacket packet) {
  byte checksum = 0;

  serialStream->write(SLIDER_FRAMING_START); // packet start (should be sent raw)
  checksum -= SLIDER_FRAMING_START; // maybe should always be 0xFF..  not sure
  
  checksum += sendSliderByte((byte)packet.Command);

  checksum += sendSliderByte(packet.DataLength);
  
  for (byte i = 0; i < packet.DataLength; i++) {
    checksum += sendSliderByte(packet.Data[i]);
  }

  sendSliderByte(checksum);
}

// read new any new serial data into the internal buffer
// returns whether new data was available
bool segaSlider::readSerial() {
  if (serialStream->available()) {
    // read data from serial into a ring buffer, ending at the current write position
    // it may be somewhat better to only write until the current read position, but there's a chance that could lock everything when receiving bad data
    // the current behaviour may drop data sometimes, but that shouldn't really matter much.. sending data is much more important and doesn't depend on this at all
    if (SLIDER_SERIAL_TEXT_MODE) {
      static char buf[8];
      static byte readlen = 0;

      int newWritePos = serialBufWritePos;
      
      // while serial is available, read space separated strings into buffer bytes
      // note that buf and readlen are static, so this should work even if not a full 'byte' can be read at a time
      while (serialStream->available()) {
        if (readlen >= sizeof(buf) / sizeof(buf[0]))
          readlen = 0; // reset buf pos if it overruns (this shouldn't happen)
        
        buf[readlen] = serialStream->read();
        if (buf[readlen] == ' ') { // if found a space (end of byte)
          buf[readlen] = 0; // null-terminate buffer
          if (readlen > 0) {
            readlen = 0; // reset buf pos
            serialInBuf[newWritePos] = atoi(buf);
            newWritePos++;
            newWritePos %= SLIDER_SERIAL_BUF_SIZE;
            if (newWritePos == serialBufWritePos)
              break; // don't overwrite data that was only just read
          }
        }
      }
      serialBufWritePos = newWritePos;
    }
    else {
      int newWritePos = serialBufWritePos;
      newWritePos += serialStream->readBytes(&serialInBuf[serialBufWritePos], SLIDER_SERIAL_BUF_SIZE - serialBufWritePos); // read from serialBufWritePos to SLIDER_SERIAL_BUF_SIZE
      if (newWritePos >= SLIDER_SERIAL_BUF_SIZE)
      {
        newWritePos %= SLIDER_SERIAL_BUF_SIZE;
        newWritePos += serialStream->readBytes(&serialInBuf[newWritePos], serialBufWritePos - newWritePos); // read from newWritePos to (old)serialBufWritePos
      }
      serialBufWritePos = newWritePos;
    }
    return true;
  }
  else {
    return false;
  }
}

// returns the next slider packet from the serial buffer
// check IsValid to see if all data has been read
sliderPacket segaSlider::readNextPacket() {
  // (this does avoid reading old data from past the current write position)
  return parseRawSliderData(serialInBuf, SLIDER_SERIAL_BUF_SIZE, serialBufReadPos, serialBufWritePos);
}

