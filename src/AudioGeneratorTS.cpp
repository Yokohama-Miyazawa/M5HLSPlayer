/*
  AudioGeneratorTS
  Audio output generator using the Helix AAC decoder
  This program was made by modifying "AudioGeneratorAAC.cpp" of ESP8266Audio.

  Copyright (C) 2017  Earle F. Philhower, III
  Copyright (C) 2021  Osamu Miyazawa

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma GCC optimize ("O3")

#include "AudioGeneratorTS.h"

AudioGeneratorTS::AudioGeneratorTS()
{
  preallocateSpace = NULL;
  preallocateSize = 0;

  running = false;
  file = NULL;
  output = NULL;

  buff = (uint8_t*)malloc(buffLen);
  outSample = (int16_t*)malloc(1024 * 2 * sizeof(uint16_t));
  if (!buff || !outSample) {
    audioLogger->printf_P(PSTR("ERROR: Out of memory in AAC\n"));
    Serial.flush();
  }

  hAACDecoder = AACInitDecoder();
  if (!hAACDecoder) {
    audioLogger->printf_P(PSTR("Out of memory error! hAACDecoder==NULL\n"));
    Serial.flush();
  }

  buffValid = 0;
  lastFrameEnd = 0;
  validSamples = 0;
  curSample = 0;
  lastRate = 0;
  lastChannels = 0;

  isInputTs = true;
  isSyncByteFound = false;
  pidsOfPMT.number = 0;
  pidOfAAC = -1;
  pesDataLength = -1;
}

AudioGeneratorTS::AudioGeneratorTS(void *preallocateData, int preallocateSz)
{
  preallocateSpace = preallocateData;
  preallocateSize = preallocateSz;

  running = false;
  file = NULL;
  output = NULL;

  uint8_t *p = (uint8_t*)preallocateSpace;
  buff = (uint8_t*) p;
  p += (buffLen + 7) & ~7;
  outSample = (int16_t*) p;
  p += (1024 * 2 * sizeof(int16_t) + 7) & ~7;
  int used = p - (uint8_t*)preallocateSpace;
  int availSpace = preallocateSize - used;
  if (availSpace < 0 ) {
    audioLogger->printf_P(PSTR("ERROR: Out of memory in AAC\n"));
  }

  hAACDecoder = AACInitDecoderPre(p, availSpace);
  if (!hAACDecoder) {
    audioLogger->printf_P(PSTR("Out of memory error! hAACDecoder==NULL\n"));
    Serial.flush();
  }
  buffValid = 0;
  lastFrameEnd = 0;
  validSamples = 0;
  curSample = 0;
  lastRate = 0;
  lastChannels = 0;

  isInputTs = true;
  isSyncByteFound = false;
  pidsOfPMT.number = 0;
  pidOfAAC = -1;
  pesDataLength = -1;
}



AudioGeneratorTS::~AudioGeneratorTS()
{
  if (!preallocateSpace) {
    AACFreeDecoder(hAACDecoder);
    free(buff);
    free(outSample);
  }
}

bool AudioGeneratorTS::stop()
{
  running = false;
  output->stop();
  return file->close();
}

bool AudioGeneratorTS::isRunning()
{
  return running;
}

void AudioGeneratorTS::switchMode(bool isTS)
{
  isInputTs = isTS;
}

void AudioGeneratorTS::reset()
{
  isSyncByteFound = false;
  pidsOfPMT.number = 0;
  pidOfAAC = -1;
  pesDataLength = -1;
}

void AudioGeneratorTS::parsePAT(uint8_t *pat)
{
  int startOfProgramNums = 8;
  int lengthOfPATValue = 4;
  int sectionLength = ((pat[1] & 0x0F) << 8) | (pat[2] & 0xFF);
  log_v("Section Length: %d", sectionLength);
  int indexOfPids = 0;
  for (int i = startOfProgramNums; i <= sectionLength; i += lengthOfPATValue)
  {
    //int program_number = ((pat[i] & 0xFF) << 8) | (pat[i + 1] & 0xFF);
    //log_v("Program Num: 0x%04X(%d)", program_number, program_number);
    int program_map_PID = ((pat[i + 2] & 0x1F) << 8) | (pat[i + 3] & 0xFF);
    log_v("PMT PID: 0x%04X(%d)", program_map_PID, program_map_PID);
    pidsOfPMT.pids[indexOfPids++] = program_map_PID;
  }
  pidsOfPMT.number = indexOfPids;
}

void AudioGeneratorTS::parsePMT(uint8_t *pat)
{
  int staticLengthOfPMT = 12;
  int sectionLength = ((pat[1] & 0x0F) << 8) | (pat[2] & 0xFF);
  log_v("Section Length: %d", sectionLength);
  int programInfoLength = ((pat[10] & 0x0F) << 8) | (pat[11] & 0xFF);
  log_v("Program Info Length: %d", programInfoLength);

  int cursor = staticLengthOfPMT + programInfoLength;
  while (cursor < sectionLength - 1)
  {
    int streamType = pat[cursor] & 0xFF;
    int elementaryPID = ((pat[cursor + 1] & 0x1F) << 8) | (pat[cursor + 2] & 0xFF);
    log_v("Stream Type: 0x%02X(%d) Elementary PID: 0x%04X(%d)",
          streamType, streamType, elementaryPID, elementaryPID);

    if (streamType == 0x04) Serial.println("The type of this stream is MP3, which is not yet supported in this program.");
    if (streamType == 0x0F || streamType == 0x11) pidOfAAC = elementaryPID;

    int esInfoLength = ((pat[cursor + 3] & 0x0F) << 8) | (pat[cursor + 4] & 0xFF);
    log_v("ES Info Length: 0x%04X(%d)", esInfoLength, esInfoLength);
    cursor += 5 + esInfoLength;
  }
}

int AudioGeneratorTS::parsePES(uint8_t *pat, const int posOfPacketStart, const bool isNewPayload, uint8_t *data)
{
  if (isNewPayload)
  {
    uint8_t streamID = pat[3] & 0xFF;
    if(streamID < 0xC0 || streamID > 0xDF){
      Serial.printf("Stream ID:%02X ", streamID);
      if(0xE0 <= streamID && streamID <= 0xEF){
      Serial.println("This is a Stream ID for Video.");
      }else{
      Serial.println("Wrong Stream ID for Audio.");
      }
      exit(1);
    }
    const uint8_t posOfPacketLengthLatterHalf = 5;
    uint16_t PESRemainingPacketLength = ((pat[4] & 0xFF) << 8) | (pat[5] & 0xFF);
    log_v("PES Packet length: %d", PESRemainingPacketLength);
    pesDataLength = PESRemainingPacketLength;
    const uint8_t posOfHeaderLength = 8;
    uint8_t PESRemainingHeaderLength = pat[posOfHeaderLength] & 0xFF;
    log_v("PES Header length: %d", PESRemainingHeaderLength);
    int startOfData = posOfHeaderLength + PESRemainingHeaderLength + 1;
    const size_t dataSize = (TS_PACKET_SIZE - posOfPacketStart) - startOfData;
    memcpy(data, &pat[startOfData], dataSize);
    pesDataLength -= (TS_PACKET_SIZE - posOfPacketStart) - (posOfPacketLengthLatterHalf + 1);
    return dataSize;
  }
  else
  {
    const size_t dataSize = TS_PACKET_SIZE - posOfPacketStart;
    memcpy(data, pat, dataSize);
    pesDataLength -= dataSize;
    return dataSize;
  }
  return 0;
}

int AudioGeneratorTS::parsePacket(uint8_t *packet, uint8_t *data)
{
  int read = 0;

  int pid = ((packet[1] & 0x1F) << 8) | (packet[2] & 0xFF);
  log_v("PID: 0x%04X(%d)", pid, pid);
  int payloadUnitStartIndicator = (packet[1] & 0x40) >> 6;
  log_v("Payload Unit Start Indicator: %d", payloadUnitStartIndicator);
  int adaptionFieldControl = (packet[3] & 0x30) >> 4;
  log_v("Adaption Field Control: %d", adaptionFieldControl);
  int remainingAdaptationFieldLength = -1;
  if ((adaptionFieldControl & 0b10) == 0b10)
  {
    remainingAdaptationFieldLength = packet[4] & 0xFF;
    log_v("Adaptation Field Length: %d", remainingAdaptationFieldLength);
  }

  int payloadStart = payloadUnitStartIndicator ? 5 : 4;

  if (pid == 0){
    parsePAT(&packet[payloadStart]);
  } else if (pid == pidOfAAC){
    int posOfPacketStart = 4;
    if (remainingAdaptationFieldLength >= 0) posOfPacketStart = 5 + remainingAdaptationFieldLength;
    read = parsePES(&packet[posOfPacketStart], posOfPacketStart, payloadUnitStartIndicator ? true : false, data);
  } else if (pidsOfPMT.number){
    for (int i = 0; i < pidsOfPMT.number; i++){
      if (pid == pidsOfPMT.pids[i]){
        parsePMT(&packet[payloadStart]);
      }
    }
  }

  return read;
}

uint32_t AudioGeneratorTS::readFile(void *data, uint32_t len)
{
  // If len is too short, return 0
  if(len < (TS_PACKET_SIZE - TS_HEADER_SIZE)) { return 0; }

  int read;
  int aacRead = 0;
  do {
    if(!isSyncByteFound) {
      uint8_t oneByte;
      do {
        if(!file->read(&oneByte, 1)) return 0;
      } while (oneByte != 0x47);
      isSyncByteFound = true;
      read = file->read(&packetBuff[1], TS_PACKET_SIZE-1);
    } else {
      read = file->read(packetBuff, TS_PACKET_SIZE);
    }
    if(read){ aacRead += parsePacket(packetBuff, &reinterpret_cast<uint8_t*>(data)[aacRead]); }
  } while ((len - aacRead) >= (TS_PACKET_SIZE - TS_HEADER_SIZE) && read);

  return aacRead;
}

bool AudioGeneratorTS::FillBufferWithValidFrame()
{
  buff[0] = 0; // Destroy any existing sync word @ 0
  int nextSync;
  do {
    nextSync = AACFindSyncWord(buff + lastFrameEnd, buffValid - lastFrameEnd);
    if (nextSync >= 0) nextSync += lastFrameEnd;
    lastFrameEnd = 0;
    if (nextSync == -1) {
      if (buffValid && buff[buffValid-1]==0xff) { // Could be 1st half of syncword, preserve it...
        buff[0] = 0xff;
        buffValid = isInputTs ? readFile(buff+1, buffLen-1) : file->read(buff+1, buffLen-1);
        if (buffValid==0) return false; // No data available, EOF
      } else { // Try a whole new buffer
        buffValid = isInputTs ? readFile(buff, buffLen-1) : file->read(buff, buffLen-1);
        if (buffValid==0) return false; // No data available, EOF
      }
    }
  } while (nextSync == -1);

  // Move the frame to start at offset 0 in the buffer
  buffValid -= nextSync; // Throw out prior to nextSync
  memmove(buff, buff+nextSync, buffValid);

  // We have a sync word at 0 now, try and fill remainder of buffer
  buffValid += isInputTs ? readFile(buff + buffValid, buffLen - buffValid) : file->read(buff + buffValid, buffLen - buffValid);

  return true;
}

bool AudioGeneratorTS::loop()
{
  if (!running) goto done; // Nothing to do here!

  // If we've got data, try and pump it out...
  while (validSamples) {
    if (lastChannels == 1) {
       lastSample[0] = outSample[curSample];
       lastSample[1] = outSample[curSample];
    } else {
      lastSample[0] = outSample[curSample*2];
      lastSample[1] = outSample[curSample*2 + 1];
    }
    if (!output->ConsumeSample(lastSample)) goto done; // Can't send, but no error detected
    validSamples--;
    curSample++;
  }

  // No samples available, need to decode a new frame
  if (FillBufferWithValidFrame()) {
    // buff[0] start of frame, decode it...
    unsigned char *inBuff = reinterpret_cast<unsigned char *>(buff);
    int bytesLeft = buffValid;
    int ret = AACDecode(hAACDecoder, &inBuff, &bytesLeft, outSample);
    if (ret) {
      // Error, skip the frame...
      char buff[48];
      sprintf_P(buff, PSTR("AAC decode error %d"), ret);
      cb.st(ret, buff);
    } else {
      lastFrameEnd = buffValid - bytesLeft;
      AACFrameInfo fi;
      AACGetLastFrameInfo(hAACDecoder, &fi);
      if ((int)fi.sampRateOut != (int)lastRate) {
        output->SetRate(fi.sampRateOut);
        lastRate = fi.sampRateOut;
      }
      if (fi.nChans != lastChannels) {
        output->SetChannels(fi.nChans);
        lastChannels = fi.nChans;
      }
      curSample = 0;
      validSamples = fi.outputSamps / lastChannels;
    }
  } else {
    running = false; // No more data, we're done here...
  }

done:
  file->loop();
  output->loop();

  return running;
}

bool AudioGeneratorTS::begin(AudioFileSource *source, AudioOutput *output)
{
  if (!source) return false;
  file = source;
  if (!output) return false;
  this->output = output;
  if (!file->isOpen()) return false; // Error

  if(!output->begin()) return false;
  
  // AAC always comes out at 16 bits
  output->SetBitsPerSample(16);
 

  memset(packetBuff, 0xFF, TS_PACKET_SIZE);
  memset(buff, 0, buffLen);
  memset(outSample, 0, 1024*2*sizeof(int16_t));

 
  running = true;
  
  return true;
}


