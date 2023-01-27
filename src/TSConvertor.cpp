/*
  TSConvertor
  Convertor from TS packet data to AAC or MP3 audio data
  Copyright (C) 2023  Osamu Miyazawa

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "TSConvertor.h"

TSConvertor::TSConvertor()
{
  format = FileFormat::UNKNOWN;
  isSyncByteFound = false;
  pidsOfPMT.number = 0;
  pidOfAudio = -1;
  pesDataLength = -1;
}

TSConvertor::~TSConvertor(){}

FileFormat TSConvertor::getOutputFormat()
{
  return format;
}

void TSConvertor::reset()
{
  format = FileFormat::UNKNOWN;
  isSyncByteFound = false;
  pidsOfPMT.number = 0;
  pidOfAudio = -1;
  pesDataLength = -1;
}

uint32_t TSConvertor::convert(AudioFileSource *source, void *data, uint32_t len)
{
  // If len is too short, return 0
  if(len < (TS_PACKET_SIZE - TS_HEADER_SIZE)) { return 0; }

  int read;
  int aacRead = 0;
  do {
    if(!isSyncByteFound) {
      uint8_t oneByte;
      do {
        if(!source->read(&oneByte, 1)) return 0;
      } while (oneByte != 0x47);
      isSyncByteFound = true;
      read = source->read(&packetBuff[1], TS_PACKET_SIZE-1);
    } else {
      read = source->read(packetBuff, TS_PACKET_SIZE);
    }
    if(read){ aacRead += parsePacket(packetBuff, &reinterpret_cast<uint8_t*>(data)[aacRead]); }
  } while ((len - aacRead) >= (TS_PACKET_SIZE - TS_HEADER_SIZE) && read);

  return aacRead;
}

void TSConvertor::parsePAT(uint8_t *pat)
{
  const int startOfProgramNums = 8;
  const int lengthOfPATValue = 4;
  const int sectionLength = ((pat[1] & 0x0F) << 8) | (pat[2] & 0xFF);
  log_v("Section Length: %d", sectionLength);
  int indexOfPids = 0;
  for (int i = startOfProgramNums; i <= sectionLength; i += lengthOfPATValue)
  {
    //int program_number = ((pat[i] & 0xFF) << 8) | (pat[i + 1] & 0xFF);
    //log_v("Program Num: 0x%04X(%d)", program_number, program_number);
    const int program_map_PID = ((pat[i + 2] & 0x1F) << 8) | (pat[i + 3] & 0xFF);
    log_v("PMT PID: 0x%04X(%d)", program_map_PID, program_map_PID);
    pidsOfPMT.pids[indexOfPids++] = program_map_PID;
  }
  pidsOfPMT.number = indexOfPids;
}

void TSConvertor::parsePMT(uint8_t *pat)
{
  const int staticLengthOfPMT = 12;
  const int sectionLength = ((pat[1] & 0x0F) << 8) | (pat[2] & 0xFF);
  log_v("Section Length: %d", sectionLength);
  const int programInfoLength = ((pat[10] & 0x0F) << 8) | (pat[11] & 0xFF);
  log_v("Program Info Length: %d", programInfoLength);

  int cursor = staticLengthOfPMT + programInfoLength;
  while (cursor < sectionLength - 1)
  {
    const int streamType = pat[cursor] & 0xFF;
    const int elementaryPID = ((pat[cursor + 1] & 0x1F) << 8) | (pat[cursor + 2] & 0xFF);
    log_v("Stream Type: 0x%02X(%d) Elementary PID: 0x%04X(%d)",
          streamType, streamType, elementaryPID, elementaryPID);

    if (streamType == 0x04)
    {
      format = FileFormat::MP3;
      pidOfAudio = elementaryPID;
    }
    else if (streamType == 0x0F || streamType == 0x11)
    {
      format = FileFormat::AAC;
      pidOfAudio = elementaryPID;
    }

    const int esInfoLength = ((pat[cursor + 3] & 0x0F) << 8) | (pat[cursor + 4] & 0xFF);
    log_v("ES Info Length: 0x%04X(%d)", esInfoLength, esInfoLength);
    cursor += 5 + esInfoLength;
  }
}

int TSConvertor::parsePES(uint8_t *pat, const int posOfPacketStart, const bool isNewPayload, uint8_t *data)
{
  if (isNewPayload)
  {
    const uint8_t streamID = pat[3] & 0xFF;
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
    const uint16_t PESRemainingPacketLength = ((pat[4] & 0xFF) << 8) | (pat[5] & 0xFF);
    log_v("PES Packet length: %d", PESRemainingPacketLength);
    pesDataLength = PESRemainingPacketLength;
    const uint8_t posOfHeaderLength = 8;
    const uint8_t PESRemainingHeaderLength = pat[posOfHeaderLength] & 0xFF;
    log_v("PES Header length: %d", PESRemainingHeaderLength);
    const int startOfData = posOfHeaderLength + PESRemainingHeaderLength + 1;
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

int TSConvertor::parsePacket(uint8_t *packet, uint8_t *data)
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
  } else if (pid == pidOfAudio){
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
