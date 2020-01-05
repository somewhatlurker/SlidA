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

// read new serial data into the internal buffer
// returns whether new data was available
bool segaSlider::readSerial() {
  byte pos = 0;
  bool foundFF = false;
  bool wrote = false;
  
  #if SLIDER_SERIAL_TEXT_MODE
    static bool alreadyFoundFF = false;    
    // while serial is available, read space separated strings into buffer bytes
    while (serialStream->available() && pos != SLIDER_SERIAL_BUF_SIZE) {
      if (serialTextReadlen >= sizeof(serialTextBuf) / sizeof(serialTextBuf[0]))
        serialTextReadlen = 0; // reset serialTextBuf pos if it overruns (this shouldn't happen)

      // the start of the next packet may have been found after the last data already
      if(alreadyFoundFF) {
        foundFF = true;
        alreadyFoundFF = false;
      }
      
      serialTextBuf[serialTextReadlen] = serialStream->read();
      if (serialTextBuf[serialTextReadlen] == ' ') { // if found a space (end of byte)
        serialTextBuf[serialTextReadlen] = 0; // null-terminate buffer
        if (serialTextReadlen > 0) {
          serialTextReadlen = 0; // reset serialTextBuf pos
          byte val = atoi(serialTextBuf);
          if (!foundFF) {
            if (val == SLIDER_FRAMING_START)
              foundFF = true;
          }
          else if (val == SLIDER_FRAMING_START) {
            alreadyFoundFF = true;
            break;
          }
          else {
            serialInBuf[pos++] = val;
            wrote = true;
          }
        }
      }
      else {
        serialTextReadlen++;
      }
    }
    
  #else // SLIDER_SERIAL_TEXT_MODE
    unsigned long startMillis = millis();
    
    while (!foundFF) {
      // find data start
      if (serialStream->available() && serialStream->read() == SLIDER_FRAMING_START)
        foundFF = true;

      // wait 1ms for packet start
      if (millis() - startMillis > 1)
        break;
    }
    if (foundFF) {
      while (pos != SLIDER_SERIAL_BUF_SIZE) {
        if (serialStream->available()) {
          if (serialStream->peek() == (byte)SLIDER_FRAMING_START)
            break;
            
          serialInBuf[pos++] = serialStream->read();
          wrote = true;
        }

        // absolute max of 20ms from start
        if (millis() - startMillis > 20)
          break;
      }
    }
    
  #endif // SLIDER_SERIAL_TEXT_MODE

  if (wrote)
    return true;
  
  return false;
}

// returns the slider packet from the serial buffer
// invalid packets will have IsValid set to false
sliderPacket segaSlider::getPacket() {
  static byte packetData[MAX_SLIDER_PACKET_SIZE];
  static sliderPacket outPkt;

  outPkt.Command = (sliderCommand)0;
  outPkt.DataLength = 0;
  outPkt.IsValid = false;

  // unescape and copy bytes
  byte outpos = 0; // define in this scope so it can be checked later
  for (byte i = 0; i < SLIDER_SERIAL_BUF_SIZE && outpos < MAX_SLIDER_PACKET_SIZE; i++)
  {
    if (serialInBuf[i] == SLIDER_FRAMING_ESCAPE) { // escaped byte sequence
      i++; // skip SLIDER_FRAMING_ESCAPE
      if (i == SLIDER_SERIAL_BUF_SIZE) // check data hasn't ended
        break;
      packetData[outpos] = serialInBuf[i] + 1;
    }
    else {
      packetData[outpos] = serialInBuf[i];
    }
    outpos++;
  }

  outPkt.DataLength = packetData[1];

  if (outpos >= outPkt.DataLength + 3) { // read enough data
    outPkt.Command = (sliderCommand)packetData[0];
    outPkt.Data = &packetData[2];

    outPkt.IsValid = checkSliderPacketSum(outPkt, packetData[outPkt.DataLength + 2]);
  }
  
  return outPkt;
}

