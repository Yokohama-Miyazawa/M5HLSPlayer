/*
  AudioFileSourceTSConvertor
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

#include <AudioFileSource.h>

#define TS_PACKET_SIZE 188
#define TS_HEADER_SIZE 4
#define PID_ARRAY_LEN 4

enum class FileFormat
{
  AAC,
  MP3,
  UNKNOWN
};

class AudioFileSourceTSConvertor : public AudioFileSource
{
  public:
    AudioFileSourceTSConvertor(AudioFileSource *in, const bool isTSData);
    virtual ~AudioFileSourceTSConvertor() override;

    virtual uint32_t read(void *data, uint32_t len) override;
    virtual bool seek(int32_t pos, int dir) override;
    virtual bool close() override;
    virtual bool isOpen() override;
    virtual uint32_t getSize() override;
    virtual uint32_t getPos() override;
    virtual bool loop() override;
  
    FileFormat search();

  private:
    void parsePAT(uint8_t *pat);
    void parsePMT(uint8_t *pat);
    int parsePES(uint8_t *pat, const int posOfPacketStart, const bool isNewPayload, uint8_t *data);
    int parsePacket(uint8_t *packet, uint8_t *data, const bool isSearch = false);

  private:
    typedef struct
    {
      int number;
      int pids[PID_ARRAY_LEN];
    } pid_array;
  
  private:
    bool isInputTS;
    AudioFileSource *src;
    uint8_t packetBuff[TS_PACKET_SIZE];
    bool isSyncByteFound;
    pid_array pidsOfPMT;
    int16_t pidOfAudio;
    FileFormat format;
    int16_t pesDataLength;
};
