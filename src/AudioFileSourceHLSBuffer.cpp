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
  src = source;
  length = 0;
  filled = false;
  isTSBuffer = isTSData;
}

AudioFileSourceHLSBuffer::AudioFileSourceHLSBuffer(AudioFileSource *source, void *inBuff, uint32_t buffSizeBytes, bool isTSData)
{
  buffSize = buffSizeBytes;
  buffer = (uint8_t*)inBuff;
  deallocateBuffer = false;
  writePtr = 0;
  readPtr = 0;
  src = source;
  length = 0;
  filled = false;
  isTSBuffer = isTSData;
}

AudioFileSourceHLSBuffer::~AudioFileSourceHLSBuffer()
{
  if (deallocateBuffer) free(buffer);
  buffer = NULL;
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
  return sourceQueue.length() >= QUEUESIZE;
}

void AudioFileSourceHLSBuffer::addSource(AudioFileSource *src)
{
  sourceQueue.push(src);
  return;
}

bool AudioFileSourceHLSBuffer::changeSource()
{
  if(sourceQueue.length() == 0) return false;
  delete src;
  src = sourceQueue.pop();
  return true;
}

uint32_t AudioFileSourceHLSBuffer::read(void *data, uint32_t len)
{
  if (!buffer) return src->read(data, len);

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

  if (length < buffSize) {
    // Now try and opportunistically fill the buffer
    if (readPtr > writePtr) {
      if (readPtr == writePtr+1) return;
      uint32_t bytesAvailMid = readPtr - writePtr - 1;
      int cnt = src->readNonBlock(&buffer[writePtr], bytesAvailMid);
      if(cnt == 0 && changeSource()) cnt = src->readNonBlock(&buffer[writePtr], bytesAvailMid);
      length += cnt;
      writePtr = (writePtr + cnt) % buffSize;
      return;
    }

    if (buffSize > writePtr) {
      uint32_t bytesAvailEnd = buffSize - writePtr;
      int cnt = src->readNonBlock(&buffer[writePtr], bytesAvailEnd);
      if(cnt == 0 && changeSource()) cnt = src->readNonBlock(&buffer[writePtr], bytesAvailEnd);
      length += cnt;
      writePtr = (writePtr + cnt) % buffSize;
      if (cnt != (int)bytesAvailEnd) return;
    }

    if (readPtr > 1) {
      uint32_t bytesAvailStart = readPtr - 1;
      int cnt = src->readNonBlock(&buffer[writePtr], bytesAvailStart);
      if(cnt == 0 && changeSource()) cnt = src->readNonBlock(&buffer[writePtr], bytesAvailStart);
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

