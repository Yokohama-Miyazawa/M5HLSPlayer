/*
  AudioGeneratorTS
  Audio output generator using the Helix AAC decoder
  
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
  pidsOfAAC.number = 0;
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
  pidsOfAAC.number = 0;
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
  pidsOfAAC.number = 0;
}

void AudioGeneratorTS::parsePAT(uint8_t *pat)
{
  int startOfProgramNums = 8;
  int lengthOfPATValue = 4;
  int sectionLength = ((pat[1] & 0x0F) << 8) | (pat[2] & 0xFF);
  log_v("Section Length: %d", sectionLength);
  int program_number, program_map_PID;
  int indexOfPids = 0;
  for (int i = startOfProgramNums; i <= sectionLength; i += lengthOfPATValue)
  {
    program_number = ((pat[i] & 0xFF) << 8) | (pat[i + 1] & 0xFF);
    program_map_PID = ((pat[i + 2] & 0x1F) << 8) | (pat[i + 3] & 0xFF);
    log_v("Program Num: 0x%04X(%d) PMT PID: 0x%04X(%d)",
          program_number, program_number, program_map_PID, program_map_PID);
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

  int indexOfPids = pidsOfAAC.number;
  int cursor = staticLengthOfPMT + programInfoLength;
  while (cursor < sectionLength - 1)
  {
    int streamType = pat[cursor] & 0xFF;
    int elementaryPID = ((pat[cursor + 1] & 0x1F) << 8) | (pat[cursor + 2] & 0xFF);
    log_v("Stream Type: 0x%02X(%d) Elementary PID: 0x%04X(%d)",
          streamType, streamType, elementaryPID, elementaryPID);

    if (streamType == 0x0F || streamType == 0x11)
    {
      log_v("found AAC PID");
      pidsOfAAC.pids[indexOfPids++] = elementaryPID;
    }

    int esInfoLength = ((pat[cursor + 3] & 0x0F) << 8) | (pat[cursor + 4] & 0xFF);
    log_v("ES Info Length: 0x%04X(%d)", esInfoLength, esInfoLength);
    cursor += 5 + esInfoLength;
  }
  pidsOfAAC.number = indexOfPids;
}

void AudioGeneratorTS::showBinary(uint8_t *data, int len, String comment="Show Binary")
{
  if(len <= 0) return;
  log_v("%s", comment.c_str());
  //log_v("data[0]: %02X", data[0]);
  log_v("len: %d", len);
  log_v("data[0]: %02X, data[-1]: %02X", data[0], data[len-1]);
  //log_v("%s", (comment+" Whole").c_str());
  //for (int i = 0; i < len; i++){Serial.printf("%02X", data[i]);}
  //log_v("\n%s", (comment+" END").c_str());
}

int AudioGeneratorTS::parsePES(uint8_t *pat, int posOfPacketStart, uint8_t *data)
{
  log_v("Address of pat: %d, of data %d", pat, data);
  int dataSize;
  int firstByte = pat[0] & 0xFF;
  int secondByte = pat[1] & 0xFF;
  int thirdByte = pat[2] & 0xFF;
  log_v("First 3 bytes: %02X %02X %02X", firstByte, secondByte, thirdByte);
  if (firstByte == 0x00 && secondByte == 0x00 && thirdByte == 0x01)
  {
    int PESRemainingPacketLength = ((pat[4] & 0xFF) << 8) | (pat[5] & 0xFF);
    log_v("PES Packet length: %d", PESRemainingPacketLength);
    int posOfHeaderLength = 8;
    int PESRemainingHeaderLength = pat[posOfHeaderLength] & 0xFF;
    log_v("PES Header length: %d", PESRemainingHeaderLength);
    int startOfData = posOfHeaderLength + PESRemainingHeaderLength + 1;
    log_v("First AAC data byte: %02X", pat[startOfData]);
    // fwrite(&pat[startOfData], 1, (TS_PACKET_SIZE - posOfPacketStart) - startOfData, wfp);
    dataSize = (TS_PACKET_SIZE - posOfPacketStart) - startOfData;
    log_v("dataSize: %d", dataSize);
    memcpy(data, &pat[startOfData], dataSize);
    log_v("pat tail: %02X, data tail: %02X", pat[TS_PACKET_SIZE - posOfPacketStart - 1], data[dataSize - 1]);
  }
  else
  {
    log_v("First AAC data byte: %02X", pat[0]);
    // fwrite(pat, 1, TS_PACKET_SIZE - posOfPacketStart, wfp);
    dataSize = TS_PACKET_SIZE - posOfPacketStart;
    log_v("dataSize: %d", dataSize);
    memcpy(data, pat, dataSize);
    log_v("pat tail: %02X, data tail: %02X", pat[dataSize - 1], data[dataSize - 1]);
  }
  log_v("First 3 bytes of Copied Data: %02X %02X %02X", data[0], data[1], data[2]);
  //showBinary(data, dataSize, "@parsePES");
  //log_v("dataSize: %d\n", dataSize);
  return dataSize;
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
    log_v("parse PAT");
    parsePAT(&packet[payloadStart]);
  } else if (pidsOfAAC.number){
    for (int i = 0; i < pidsOfAAC.number; i++){
      if (pid == pidsOfAAC.pids[i]){
        log_v("found AAC");
        int posOfPacketSrart = 4;
        if (remainingAdaptationFieldLength >= 0){
          posOfPacketSrart = 5 + remainingAdaptationFieldLength;
          read = parsePES(&packet[posOfPacketSrart], posOfPacketSrart, data);
        }else{
          read = parsePES(&packet[posOfPacketSrart], posOfPacketSrart, data);
        }
      }
    }
  } else if (pidsOfPMT.number){
    for (int i = 0; i < pidsOfPMT.number; i++){
      if (pid == pidsOfPMT.pids[i]){
        log_v("found PMT");
        parsePMT(&packet[payloadStart]);
      }
    }
  }
  showBinary(data, read, "@parsePacket");
  if(read > 0){
    log_v("read: %d\n", read);
  } else {
    log_v("PID: %d", pid);
  }
  return read;
}

uint32_t AudioGeneratorTS::readFile(void *data, uint32_t len)
{
  // Shorten len to multiples of the length of an mpeg2-ts packet.
  len -= len % TS_PACKET_SIZE;
  if(len == 0 || len < TS_PACKET_SIZE) { return 0; }

  int read;
  int aacRead = 0;
  do {
    if(!isSyncByteFound) {
      uint8_t oneByte;
      do { log_e("finding sync byte...");file->read(&oneByte, 1); log_e("One Byte: 0x%02X", oneByte);} while (oneByte != 0x47);
      isSyncByteFound = true;
      read = file->read(&packetBuff[1], TS_PACKET_SIZE-1);
    } else {
      read = file->read(packetBuff, TS_PACKET_SIZE);
    }
    if(read){ aacRead += parsePacket(packetBuff, &reinterpret_cast<uint8_t*>(data)[aacRead]); }
  } while ((len - aacRead) >= TS_PACKET_SIZE && read);

  showBinary(reinterpret_cast<uint8_t*>(data), aacRead, "# READFILE #");

  log_v("aacRead: %d", aacRead);
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
        //buffValid = file->read(buff+1, buffLen-1);
        buffValid = isInputTs ? readFile(buff+1, buffLen-1) : file->read(buff+1, buffLen-1);
        log_v("buffValid: %d", buffValid);
        if (buffValid==0) return false; // No data available, EOF
      } else { // Try a whole new buffer
        //buffValid = file->read(buff, buffLen-1);
        buffValid = isInputTs ? readFile(buff, buffLen-1) : file->read(buff, buffLen-1);
        log_v("buffValid: %d", buffValid);
        if (buffValid==0) return false; // No data available, EOF
      }
    }
  } while (nextSync == -1);

  // Move the frame to start at offset 0 in the buffer
  buffValid -= nextSync; // Throw out prior to nextSync
  memmove(buff, buff+nextSync, buffValid);

  // We have a sync word at 0 now, try and fill remainder of buffer
  //buffValid += file->read(buff + buffValid, buffLen - buffValid);
  buffValid += isInputTs ? readFile(buff + buffValid, buffLen - buffValid) : file->read(buff + buffValid, buffLen - buffValid);
  log_v("buffValid: %d", buffValid);

  return true;
}

bool AudioGeneratorTS::loop()
{
  log_d("AudioGeneratorTS::loop() is called.");
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
    log_v("bytesLeft: %d", bytesLeft);
    log_v("AACDecode will be called.");
    int ret = AACDecode(hAACDecoder, &inBuff, &bytesLeft, outSample);
    log_v("ret: %d", ret);
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

  output->begin();
  
  // AAC always comes out at 16 bits
  output->SetBitsPerSample(16);
 

  memset(packetBuff, 0xFF, TS_PACKET_SIZE);
  memset(buff, 0, buffLen);
  memset(outSample, 0, 1024*2*sizeof(int16_t));

 
  running = true;
  
  return true;
}


