/*
  M3U8Player
  Http Live Streaming player for M5Stack and M5StickC
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
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <Arduino.h>
#include <HTTPClient.h>
#include <AudioFileSourceHTTPStream.h>
#include <AudioOutputI2S.h>
#include "HLSUrl.h"
#include "AudioFileSourceHLSBuffer.h"
#include "AudioGeneratorTS.h"

enum class M3U8Player_State
{
  SETUP,
  STANDBY,
  PLAYING,
  CHANNEL_CHANGING,
  RECOVERY_SEGMENT,
  OTHERS
};

class M3U8Player {
public:
  M3U8Player(String url);
  M3U8Player(String url, const float &startVolume);
  M3U8Player(String url, const float &startVolume, const bool &isAutoStart);
  M3U8Player(String url, const float &startVolume, const bool &isAutoStart, const uint32_t &bufferSize);
  M3U8Player(String url, const float &startVolume, const bool &isAutoStart, const uint32_t &bufferSize, const bool &isCore2);
  M3U8Player(String url, const float &startVolume, const bool &isAutoStart, const uint32_t &bufferSize, const bool &isCore2, const char* rootca);
  ~M3U8Player();
  bool start();
  void setVolume(const float &newVolume);
  float getVolume();
  bool changeStationURL(const String &url);
  String getStationURL();
  M3U8Player_State getState();
private:
  M3U8Player_State state;
  String stationUrl;
  float volume;
  TaskHandle_t scrapeAACHandle;
  TaskHandle_t playAACHandle;
  AudioGeneratorTS *ts;
  AudioFileSourceHLSBuffer* buff;
  AudioFileSourceHLSBuffer* nextBuff;
  AudioOutputI2S *out;
  uint32_t buffSize;
  uint8_t targetDuration;
  HLSUrl* urls;
  HLSUrl* nextUrls;
  const char* rca;
  void setBuffer(HLSUrl* url);
  bool recovery();
  void changeChannel();
  static void scrapeAAC(void *args);
  static void playAAC(void *args);
};
