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

#ifndef _AudioGeneratorTS_H
#define _AudioGeneratorTS_H

#include "AudioGenerator.h"
#include "libhelix-aac/aacdec.h"

#define TS_PACKET_SIZE 188
#define PID_ARRAY_LEN 4
typedef struct
{
  int number;
  int pids[PID_ARRAY_LEN];
} pid_array;

class AudioGeneratorTS : public AudioGenerator
{
  public:
    AudioGeneratorTS();
    AudioGeneratorTS(void *preallocateData, int preallocateSize);
    virtual ~AudioGeneratorTS() override;
    virtual bool begin(AudioFileSource *source, AudioOutput *output) override;
    virtual bool loop() override;
    virtual bool stop() override;
    virtual bool isRunning() override;
    void switchMode(bool isTS);
    void reset();

  protected:
    void *preallocateSpace;
    int preallocateSize;

    // Helix AAC decoder
    HAACDecoder hAACDecoder;

    // Input buffering
    const int buffLen = 1693;
    uint8_t *buff; //[1600]; // File buffer required to store at least a whole compressed frame
    int16_t buffValid;
    int16_t lastFrameEnd;
    bool FillBufferWithValidFrame(); // Read until we get a valid syncword and min(feof, 2048) butes in the buffer

    // Mpeg2-ts converting
    uint8_t packetBuff[TS_PACKET_SIZE];
    uint32_t readFile(void *data, uint32_t len);
    void parsePAT(uint8_t *pat);
    void parsePMT(uint8_t *pat);
    int parsePES(uint8_t *pat, int posOfPacketStart, uint8_t *data);
    int parsePacket(uint8_t *packet, uint8_t *data);
    void showBinary(uint8_t *data, int len, String comment);
    bool isSyncByteFound;
    pid_array pidsOfPMT;
    int16_t pidOfAAC;
    int16_t pesDataLength;

    // Mode switching (Mpeg2-ts or aac)
    bool isInputTs;

    // Output buffering
    int16_t *outSample; //[1024 * 2]; // Interleaved L/R
    int16_t validSamples;
    int16_t curSample;

    // Each frame may change this if they're very strange, I guess
    unsigned int lastRate;
    int lastChannels;

};

#endif

