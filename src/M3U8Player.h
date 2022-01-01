#include <Arduino.h>
#include <HTTPClient.h>
#include <AudioFileSourceHTTPStream.h>
#include <AudioOutputI2S.h>
#include "HLSUrl.h"
#include "AudioFileSourceHLSBuffer.h"
#include "AudioGeneratorTS.h"


class M3U8Player {
public:
  M3U8Player(String url);
  M3U8Player(String url, const float &startVolume);
  ~M3U8Player();
  bool start();
  void setVolume(const float &newVolume);
  float getVolume();
  bool changeStationURL(const String &url);
  String getStationURL();
private:
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
  bool isReferringUrls;
  bool isChannelChanging;
  bool isPlaying;
  void setBuffer(HLSUrl* url);
  static void scrapeAAC(void *args);
  static void playAAC(void *args);
};
