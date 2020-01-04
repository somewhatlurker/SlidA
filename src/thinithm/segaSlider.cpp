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
  // the special SLIDER_FRAMING_ESCAPE and SLIDER_FRAMING_START values must be escaped
  // escaped bytes are represented as SLIDER_FRAMING_ESCAPE followed by the original byte minus 1
  #if SLIDER_SERIAL_TEXT_MODE
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
    
  #else // SLIDER_SERIAL_TEXT_MODE
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
    
  #endif // SLIDER_SERIAL_TEXT_MODE
  
  return 0 - data; // ckecksum is based on unescaped data
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
sliderPacket segaSlider::parseRawSliderData(byte* data, int bufsize, int &bufpos, int maxpos) {
  int startOffset = -1;
  int endOffset = -1;
  
  static byte packetData[MAX_SLIDER_PACKET_SIZE];
  static sliderPacket outPkt;

  outPkt.Command = (sliderCommand)0;
  outPkt.DataLength = 0;
  outPkt.IsValid = false;

  bool forceAdvance = false; // used to force advance of bufpos, for when a known full but possibly malformed packet was seen (0xFF appeared twice)

  maxpos %= bufsize;

  bool firstloop = true;
  
  // find the beginning and end of a packet
  for (int i = bufpos; (i != bufpos && i != maxpos) || firstloop; i++, i %= bufsize) {
    firstloop = false;
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
    if (data[i] == SLIDER_FRAMING_ESCAPE) { // escaped byte sequence
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

  if (outpos >= outPkt.DataLength + 4) { // read enough data
    outPkt.Command = (sliderCommand)packetData[1];
    outPkt.Data = &packetData[3];

    outPkt.IsValid = checkSliderPacketSum(outPkt, packetData[outPkt.DataLength + 3]);
    data[startOffset] = 0; // render the packet unreadable
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

  #if SLIDER_SERIAL_TEXT_MODE
    serialStream->write(String(SLIDER_FRAMING_START).c_str()); // packet start (should be sent raw)
    serialStream->write(" ");
  
  #else // SLIDER_SERIAL_TEXT_MODE
    serialStream->write(SLIDER_FRAMING_START); // packet start (should be sent raw)
    
  #endif // SLIDER_SERIAL_TEXT_MODE
  
  checksum -= SLIDER_FRAMING_START; // maybe should always be 0xFF..  not sure
  
  checksum += sendSliderByte((byte)packet.Command);

  checksum += sendSliderByte(packet.DataLength);
  
  for (byte i = 0; i < packet.DataLength; i++) {
    checksum += sendSliderByte(packet.Data[i]);
  }

  // invalid packets should have an incorrect checksum
  // this might be useful for testing
  if (!packet.IsValid)
    checksum += 39;

  sendSliderByte(checksum);

  #if SLIDER_SERIAL_TEXT_MODE
    serialStream->write("\n");
  #endif // SLIDER_SERIAL_TEXT_MODE
}

// read new any new serial data into the internal buffer
// returns whether new data was available
bool segaSlider::readSerial() {
  if (serialStream->available()) {
    // read data from serial into a ring buffer, ending at the current write position
    #if SLIDER_SERIAL_TEXT_MODE
      
      // while serial is available, read space separated strings into buffer bytes
      while (serialStream->available()) {
        if (serialTextReadlen >= sizeof(serialTextBuf) / sizeof(serialTextBuf[0]))
          serialTextReadlen = 0; // reset serialTextBuf pos if it overruns (this shouldn't happen)
        
        serialTextBuf[serialTextReadlen] = serialStream->read();
        if (serialTextBuf[serialTextReadlen] == ' ') { // if found a space (end of byte)
          serialTextBuf[serialTextReadlen] = 0; // null-terminate buffer
          if (serialTextReadlen > 0) {
            serialTextReadlen = 0; // reset serialTextBuf pos
            serialInBuf[serialBufWritePos] = atoi(serialTextBuf);
            serialBufWritePos++;
            serialBufWritePos %= SLIDER_SERIAL_BUF_SIZE;
            if (serialBufWritePos == serialBufReadPos)
              break; // don't write past the read pos
          }
        }
        else {
          serialTextReadlen++;
        }
      }
      
    #else // SLIDER_SERIAL_TEXT_MODE
      if (serialBufReadPos > serialBufWritePos) {
        serialBufWritePos += serialStream->readBytes(&serialInBuf[serialBufWritePos], serialBufReadPos - serialBufWritePos); // read from serialBufWritePos to serialBufReadPos
      }
      else {
        serialBufWritePos += serialStream->readBytes(&serialInBuf[serialBufWritePos], SLIDER_SERIAL_BUF_SIZE - serialBufWritePos); // read from serialBufWritePos to SLIDER_SERIAL_BUF_SIZE
        if (serialBufWritePos >= SLIDER_SERIAL_BUF_SIZE)
        {
          serialBufWritePos %= SLIDER_SERIAL_BUF_SIZE;
          serialBufWritePos += serialStream->readBytes(&serialInBuf[serialBufWritePos], serialBufReadPos - serialBufWritePos); // read from serialBufWritePos to serialBufReadPos
        }
      }
      
    #endif // SLIDER_SERIAL_TEXT_MODE
    
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

