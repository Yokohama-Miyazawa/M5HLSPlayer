/*
  AudioFileSourceHLSBuffer
  Double-buffered file source using system RAM for http live streaming
  
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

#include <Arduino.h>
#include "AudioFileSourceHLSBuffer.h"

#pragma GCC optimize ("O3")

AudioFileSourceHLSBuffer::AudioFileSourceHLSBuffer(AudioFileSource *source, uint32_t buffSizeBytes, bool isTSData)
{
  buffSize = buffSizeBytes;
  buffer = (uint8_t*)malloc(sizeof(uint8_t) * buffSize);
  if (!buffer) audioLogger->printf_P(PSTR("Unable to allocate AudioFileSourceHLSBuffer::buffer[]\n"));
  deallocateBuffer = true;
  writePtr = 0;
  readPtr = 0;
  src = NULL;
  length = 0;
  filled = false;
  isTSBuffer = isTSData;
  sourceQueue = new Queue<AudioFileSource*>(SOURCE_QUEUE_CAPACITY);
  
  addSource(source);
  changeSource();  
}

AudioFileSourceHLSBuffer::AudioFileSourceHLSBuffer(AudioFileSource *source, void *inBuff, uint32_t buffSizeBytes, bool isTSData)
{
  buffSize = buffSizeBytes;
  buffer = (uint8_t*)inBuff;
  deallocateBuffer = false;
  writePtr = 0;
  readPtr = 0;
  src = NULL;
  length = 0;
  filled = false;
  isTSBuffer = isTSData;
  sourceQueue = new Queue<AudioFileSource*>(SOURCE_QUEUE_CAPACITY);

  addSource(source);
  changeSource();
}

AudioFileSourceHLSBuffer::~AudioFileSourceHLSBuffer()
{
  if (deallocateBuffer) free(buffer);
  buffer = NULL;
  delete sourceQueue;
}

bool AudioFileSourceHLSBuffer::seek(int32_t pos, int dir)
{
  if(dir == SEEK_CUR && (readPtr+pos) < length) {
    readPtr += pos;
    return true;
  } else {
    // Invalidate
    readPtr = 0;
    writePtr = 0;
    length = 0;
    return src->seek(pos, dir);
  }
}

bool AudioFileSourceHLSBuffer::close()
{
  if (deallocateBuffer) free(buffer);
  buffer = NULL;
  return src->close();
}

bool AudioFileSourceHLSBuffer::isOpen()
{
  return src->isOpen();
}

uint32_t AudioFileSourceHLSBuffer::getSize()
{
  return src->getSize();
}

uint32_t AudioFileSourceHLSBuffer::getPos()
{
  return src->getPos();
}

uint32_t AudioFileSourceHLSBuffer::getFillLevel()
{
  return length;
}

bool AudioFileSourceHLSBuffer::isTS()
{
  return isTSBuffer;
}

bool AudioFileSourceHLSBuffer::isFullSourceQueue()
{
  log_e("isFullSourceQueue is called. %d", sourceQueue->length() >= SOURCE_QUEUE_CAPACITY);
  return sourceQueue->length() >= SOURCE_QUEUE_CAPACITY;
}

void AudioFileSourceHLSBuffer::addSource(AudioFileSource *source)
{
  int sourceSize = source->getSize();
  if(sourceSize <= 0 || sourceSize > FILE_SIZE_LIMIT){
    log_e("Size:%d  Maybe Wrong Data. Didn't Add.", sourceSize);
    return;
  }
  sourceQueue->push(source);
  log_e("A File Source is Added. size:%d queue length:%d", source->getSize(), sourceQueue->length());
  return;
}

bool AudioFileSourceHLSBuffer::changeSource()
{
  if(sourceQueue->length() == 0){
    log_e("Source Queue is Empty.");
    return false;
  }
  if(src == NULL) {
    src = sourceQueue->pop();
  } else{
    AudioFileSource *oldSrc = src;
    src = sourceQueue->pop();
    oldSrc->close();
    delete oldSrc;
  }
  log_e("Source Changed. len:%d", sourceQueue->length());
  return true;
}

uint32_t AudioFileSourceHLSBuffer::read(void *data, uint32_t len)
{
  if (!buffer) return src->read(data, len);
  if(getSize() <= getPos()) changeSource();

  uint32_t bytes = 0;
  if (!filled) {
    // Fill up completely before returning any data at all
    cb.st(STATUS_FILLING, PSTR("Refilling buffer"));
    length = src->read(buffer, buffSize);
    writePtr = length % buffSize;
    filled = true;
  }

  // Pull from buffer until we've got none left or we've satisfied the request
  uint8_t *ptr = reinterpret_cast<uint8_t*>(data);
  uint32_t toReadFromBuffer = (len < length) ? len : length;
  if ( (toReadFromBuffer > 0) && (readPtr >= writePtr) ) {
    uint32_t toReadToEnd = (toReadFromBuffer < (uint32_t)(buffSize - readPtr)) ? toReadFromBuffer : (buffSize - readPtr);
    memcpy(ptr, &buffer[readPtr], toReadToEnd);
    readPtr = (readPtr + toReadToEnd) % buffSize;
    len -= toReadToEnd;
    length -= toReadToEnd;
    ptr += toReadToEnd;
    bytes += toReadToEnd;
    toReadFromBuffer -= toReadToEnd;
  }
  if (toReadFromBuffer > 0) { // We know RP < WP at this point
    memcpy(ptr, &buffer[readPtr], toReadFromBuffer);
    readPtr = (readPtr + toReadFromBuffer) % buffSize;
    len -= toReadFromBuffer;
    length -= toReadFromBuffer;
    ptr += toReadFromBuffer;
    bytes += toReadFromBuffer;
    toReadFromBuffer -= toReadFromBuffer;
  }

  if (len) {
    // Still need more, try direct read from src
    bytes += src->read(ptr, len);
    // We're out of buffered data, need to force a complete refill.  Thanks, @armSeb
    readPtr = 0;
    writePtr = 0;
    length = 0;
    filled = false;
    cb.st(STATUS_UNDERFLOW, PSTR("Buffer underflow"));
  }

  fill();

  return bytes;
}

void AudioFileSourceHLSBuffer::fill()
{
  if (!buffer) return;
  if(getSize() <= getPos()) changeSource();

  if (length < buffSize) {
    // Now try and opportunistically fill the buffer
    if (readPtr > writePtr) {
      if (readPtr == writePtr+1) return;
      uint32_t bytesAvailMid = readPtr - writePtr - 1;
      int cnt = src->readNonBlock(&buffer[writePtr], bytesAvailMid);
      int debugSize = getSize();
      int debugPos = getPos();
      if(debugPos < 300 || debugSize == debugPos) log_e("cnt:%d size:%d pos:%d bytesAvailMid:%d", cnt, debugSize, debugPos, bytesAvailMid);
      length += cnt;
      writePtr = (writePtr + cnt) % buffSize;
      return;
    }

    if (buffSize > writePtr) {
      uint32_t bytesAvailEnd = buffSize - writePtr;
      int cnt = src->readNonBlock(&buffer[writePtr], bytesAvailEnd);
      int debugSize = getSize();
      int debugPos = getPos();
      if(debugPos < 300 || debugSize == debugPos) log_e("cnt:%d size:%d pos:%d bytesAvailEnd:%d", cnt, debugSize, debugPos, bytesAvailEnd);
      length += cnt;
      writePtr = (writePtr + cnt) % buffSize;
      if (cnt != (int)bytesAvailEnd) return;
    }

    if (readPtr > 1) {
      uint32_t bytesAvailStart = readPtr - 1;
      int cnt = src->readNonBlock(&buffer[writePtr], bytesAvailStart);
      int debugSize = getSize();
      int debugPos = getPos();
      if(debugPos < 300 || debugSize == debugPos) log_e("cnt:%d size:%d pos:%d bytesAvailStart:%d", cnt, debugSize, debugPos, bytesAvailStart);
      length += cnt;
      writePtr = (writePtr + cnt) % buffSize;
    }
  }
}



bool AudioFileSourceHLSBuffer::loop()
{
  if (!src->loop()) return false;
  fill();
  return true;
}  

