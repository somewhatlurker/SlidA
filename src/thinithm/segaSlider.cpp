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

#include "segaSlider.h"


// sends a single escaped byte. return value is how much to adjust checksum by
byte segaSlider::sendEscapedByte(byte data) {
  // the special SLIDER_FRAMING_ESCAPE and SLIDER_FRAMING_START values must be escaped
  // escaped bytes are represented as SLIDER_FRAMING_ESCAPE followed by the original byte minus 1
  
  #if SLIDER_SERIAL_TEXT_MODE
    static String STR_SPACE = String(" ");
    static String STR_ESCAPE = String(SLIDER_FRAMING_ESCAPE);
    static String STR_ESCAPE_SUB1 = String(SLIDER_FRAMING_ESCAPE - 0x1);
    static String STR_START_SUB1 = String(SLIDER_FRAMING_ESCAPE - 0x1);

    // compute lengths ahead of time to ensure strlen won't be called at runtime
    static byte STR_SPACE_LEN = STR_SPACE.length();
    static byte STR_ESCAPE_LEN = STR_ESCAPE.length();
    static byte STR_ESCAPE_SUB1_LEN = STR_ESCAPE_SUB1.length();
    static byte STR_START_SUB1_LEN = STR_START_SUB1.length();
    
    if (data == SLIDER_FRAMING_ESCAPE) {
      #if !SLIDER_USE_STREAM
      // make sure a three digit number can be sent in each location
      if (serialStream->availableForWrite() >= 8)
      #endif
      {        
        sendError |= (serialStream->write(STR_ESCAPE.c_str(), STR_ESCAPE_LEN) != STR_ESCAPE_LEN);
        sendError |= (serialStream->write(STR_SPACE.c_str(), STR_SPACE_LEN) != STR_SPACE_LEN);
        sendError |= (serialStream->write(STR_ESCAPE_SUB1.c_str(), STR_ESCAPE_SUB1_LEN) != STR_ESCAPE_SUB1_LEN);
        sendError |= (serialStream->write(STR_SPACE.c_str(), STR_SPACE_LEN) != STR_SPACE_LEN);
      }
    }
    else if (data == SLIDER_FRAMING_START) {
      #if !SLIDER_USE_STREAM
      // make sure a three digit number can be sent in each location
      if (serialStream->availableForWrite() >= 8)
      #endif
      {        
        sendError |= (serialStream->write(STR_ESCAPE.c_str(), STR_ESCAPE_LEN) != STR_ESCAPE_LEN);
        sendError |= (serialStream->write(STR_SPACE.c_str(), STR_SPACE_LEN) != STR_SPACE_LEN);
        sendError |= (serialStream->write(STR_START_SUB1.c_str(), STR_START_SUB1_LEN) != STR_START_SUB1_LEN);
        sendError |= (serialStream->write(STR_SPACE.c_str(), STR_SPACE_LEN) != STR_SPACE_LEN);
      }
    }
    else {
      #if !SLIDER_USE_STREAM
      // make sure a three digit number can be sent
      if (serialStream->availableForWrite() >= 4)
      #endif
      {
        String dataStr = String(data);
        sendError |= (serialStream->write(dataStr.c_str()) != dataStr.length());
        sendError |= (serialStream->write(STR_SPACE.c_str(), STR_SPACE_LEN) != STR_SPACE_LEN);
      }
    }
    
  #else // SLIDER_SERIAL_TEXT_MODE
    if (data == SLIDER_FRAMING_ESCAPE) {
      #if !SLIDER_USE_STREAM
      if (serialStream->availableForWrite() >= 2)
      #endif
      {
        sendError |= (serialStream->write(SLIDER_FRAMING_ESCAPE) != 1);
        sendError |= (serialStream->write(SLIDER_FRAMING_ESCAPE - 0x1) != 1);
      }
    }
    else if (data == SLIDER_FRAMING_START) {
      #if !SLIDER_USE_STREAM
      if (serialStream->availableForWrite() >= 2)
      #endif
      {
        sendError |= (serialStream->write(SLIDER_FRAMING_ESCAPE) != 1);
        sendError |= (serialStream->write(SLIDER_FRAMING_START - 0x1) != 1);
      }
    }
    else {
      #if !SLIDER_USE_STREAM
      if (serialStream->availableForWrite() >= 1)
      #endif
      {
        sendError |= (serialStream->write(data) != 1);
      }
    }
    
  #endif // SLIDER_SERIAL_TEXT_MODE
  
  return 0 - data; // ckecksum is based on unescaped data
}

// verify a packet's checksum is valid
bool segaSlider::checkPacketSum(const sliderPacket packet, byte expectedSum) {
  byte checksum = 0;
  
  checksum -= SLIDER_FRAMING_START; // maybe should always be 0xFF..  not sure
  checksum -= (byte)packet.Command;
  checksum -= packet.DataLength;

  for (byte i = 0; i < packet.DataLength; i++) {
    checksum -= packet.Data[i];
  }

  return checksum == expectedSum;
}

#if SLIDER_SERIAL_TEXT_MODE 
  // tries to read a text "byte" from serial
  // (returns -1 if needs more data)
  int segaSlider::tryReadSerialTextByte() {
    // reset serialTextBuf pos if it overruns (this shouldn't happen)
    if (serialTextReadlen >= sizeof(serialTextBuf) / sizeof(serialTextBuf[0]))
      serialTextReadlen = 0;
    
    serialTextBuf[serialTextReadlen] = serialStream->read();
  
    // use space to find end of a "byte"
    if (serialTextBuf[serialTextReadlen] == ' ') {
      // null-terminate buffer
      serialTextBuf[serialTextReadlen] = 0;
  
      // if the buffer contains data, read it
      if (serialTextReadlen > 0) {
        serialTextReadlen = 0; // reset serialTextBuf pos
        return atoi(serialTextBuf) & 0xFF;
      }
    }
    else {
      serialTextReadlen++;
    }
  
    return -1; // this will only be reached when data isn't ready
  }
#endif // SLIDER_SERIAL_TEXT_MODE 

// read new serial data into the internal buffer
// returns whether new data was available
bool segaSlider::readSerial() {
  // fast path for no new data
  if (!serialStream->available())
    return false;
  
  unsigned long startMillis = millis();
  unsigned long lastAvailableMillis = millis();
  
  // catch unexpected fault conditions and reset on them
  if (serialInBufPos >= SLIDER_SERIAL_BUF_SIZE)
    serialInBufPos = 0;
  
  bool foundStart = serialInBufPos > 0;
  bool wroteData = false;

  // the start of the next packet may have been read after the last data already
  static bool foundStartAfterLast = false;  
  if(foundStartAfterLast) {
    foundStart = true;
    serialInBuf[0] = SLIDER_FRAMING_START;
    serialInBufPos = 1;
    foundStartAfterLast = false;
  }

  // while serial is available, read space separated strings into buffer bytes
  while (serialInBufPos != SLIDER_SERIAL_BUF_SIZE) {
    if (serialStream->available()) {
      lastAvailableMillis = millis();
      
      #if SLIDER_SERIAL_TEXT_MODE 
        int val_int = tryReadSerialTextByte();
        if (val_int == -1)
          continue;
        byte val = val_int;
      #else // SLIDER_SERIAL_TEXT_MODE
        byte val = serialStream->read();
      #endif // SLIDER_SERIAL_TEXT_MODE
      
      if (val == SLIDER_FRAMING_START) {
        if (!foundStart) {
          // start of the current packet -- set the initial data
          foundStart = true;
          serialInBuf[0] = SLIDER_FRAMING_START;
          serialInBufPos = 1;
        }
        else {
          // start of a future packet -- set a flag so we can handle it later and break
          foundStartAfterLast = true;
          break;
        }
      }
      else {
        serialInBuf[serialInBufPos++] = val;
        wroteData = true;
      }
    }
    
    // wait until X ms after last received byte before returning from readSerial
    if (millis() - lastAvailableMillis >= SLIDER_SERIAL_RECEIVE_TIMEOUT)
      break;

    // wait maximum of X ms from starting to receive bytes before returning from readSerial (even if new bytes can still be read)
    if (millis() - startMillis >= SLIDER_SERIAL_RECEIVE_MAX_MS)
      break;
  }

  if (wroteData)
    return true;
  
  return false;
}

#if SLIDER_USE_STREAM
  segaSlider::segaSlider(Stream* serial) {
    serialStream = serial;
  }
#else
  segaSlider::segaSlider(Serial_* serial) {
    serialStream = serial;
  }
#endif

// sends a slider packet (checksum is calculated automatically)
// returns whether the packet was successfully sent
bool segaSlider::sendPacket(const sliderPacket packet) {
  #if !SLIDER_USE_STREAM
    unsigned long startMillis = millis();
    
    // make sure the host is ready to receive data and there's _probably_ enough space (dropping packets is better than locking)
    // this will actually be very inaccurate for text mode but whatever
    #if SLIDER_SERIAL_TEXT_MODE
      while (serialStream->availableForWrite() < (packet.DataLength + 4) * 3) { // assumes average number length of two digits
        // wait maximum of X ms for there to be enough output capacity to send a packet
        if (millis() - startMillis >= SLIDER_SERIAL_SEND_WAIT_MS)
          return false;
      }
    #else // SLIDER_SERIAL_TEXT_MODE
      while (serialStream->availableForWrite() < packet.DataLength + 4) {
        // wait maximum of X ms for there to be enough output capacity to send a packet
        if (millis() - startMillis >= SLIDER_SERIAL_SEND_WAIT_MS)
          return false;
      }
    #endif // SLIDER_SERIAL_TEXT_MODE
  #endif // !SLIDER_USE_STREAM
  
  byte checksum = 0;
  sendError = false;

  #if SLIDER_SERIAL_TEXT_MODE
    serialStream->write(String(SLIDER_FRAMING_START).c_str()); // packet start (should be sent raw)
    serialStream->write(" ");
  #else // SLIDER_SERIAL_TEXT_MODE
    serialStream->write(SLIDER_FRAMING_START); // packet start (should be sent raw)
  #endif // SLIDER_SERIAL_TEXT_MODE
  
  checksum -= SLIDER_FRAMING_START; // maybe should always be 0xFF..  not sure
  
  checksum += sendEscapedByte((byte)packet.Command);

  checksum += sendEscapedByte(packet.DataLength);
  
  for (byte i = 0; i < packet.DataLength; i++) {
    checksum += sendEscapedByte(packet.Data[i]);
  }

  // invalid packets should have an incorrect checksum
  // this might be useful for testing
  if (!packet.IsValid)
    checksum += 39;

  sendEscapedByte(checksum);

  #if SLIDER_SERIAL_TEXT_MODE
    serialStream->write("\n");
  #endif // SLIDER_SERIAL_TEXT_MODE

  if (sendError)
    return false;
  else
    return true;
}

// read new serial data and return a slider packet from the serial buffer
// invalid packets will have IsValid set to false
// if there was no data or the buffer was incomplete, `Command` will equal `(sliderCommand)0`
sliderPacket segaSlider::getPacket() {
  static byte packetData[MAX_SLIDER_PACKET_SIZE];
  static sliderPacket outPkt;

  outPkt.Command = (sliderCommand)0;
  outPkt.DataLength = 0;
  outPkt.IsValid = false;

  // read serial
  // if no data was read, return early
  if (!readSerial())
    return outPkt;

  // check if the buffer has been used
  if (serialInBuf[0] != SLIDER_FRAMING_START)
    return outPkt;

  
  // unescape and copy bytes
  // broken into two loops so up until data length can be read first, then everything else
  byte inpos = 1;  // define in this scope so it can be used later
  byte outpos = 0; // define in this scope so it can be used later

  // read until data length
  for (inpos = 1; inpos < serialInBufPos && outpos < 2; inpos++)
  {
    if (serialInBuf[inpos] == SLIDER_FRAMING_ESCAPE) { // escaped byte sequence
      inpos++; // skip SLIDER_FRAMING_ESCAPE
      if (inpos == serialInBufPos) // check data hasn't ended
        break;
      packetData[outpos] = serialInBuf[inpos] + 1;
    }
    else {
      packetData[outpos] = serialInBuf[inpos];
    }
    outpos++;
  }

  // if outpos isn't what should be expected, or serialInBufPos can't possibly contain enough data, return now
  // serialInBufPos should contain at least data plus four bytes: framing, command, length, checksum
  if (outpos != 2 || (outPkt.DataLength = packetData[1], (outPkt.DataLength + 4) > serialInBufPos))
      return outPkt;

  // finish reading
  // note that here the outpos max is only plus three bytes because it omits the framing byte
  for (inpos = inpos; inpos < serialInBufPos && outpos < outPkt.DataLength + 3; inpos++)
  {
    if (serialInBuf[inpos] == SLIDER_FRAMING_ESCAPE) { // escaped byte sequence
      inpos++; // skip SLIDER_FRAMING_ESCAPE
      if (inpos == serialInBufPos) // check data hasn't ended
        break;
      packetData[outpos] = serialInBuf[inpos] + 1;
    }
    else {
      packetData[outpos] = serialInBuf[inpos];
    }
    outpos++;
  }

  if (outpos >= outPkt.DataLength + 3) { // read enough data
    outPkt.Command = (sliderCommand)packetData[0];
    outPkt.Data = &packetData[2];

    outPkt.IsValid = checkPacketSum(outPkt, packetData[outPkt.DataLength + 2]);

    // write an empty packet to ensure the buffer definitely gets written to next time
    memset(serialInBuf, 0, 4);

    serialInBufPos = 0;
  }
  
  return outPkt;
}

