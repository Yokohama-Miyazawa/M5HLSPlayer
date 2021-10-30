#include <Arduino.h>
#include <HTTPClient.h>
#include "AudioFileSourceHTTPStream.h"
#include "AudioGeneratorAAC.h"
#include "AudioFileSourceBuffer.h"
#include "AudioOutputI2S.h"
#include "HttpCommunicator.h"

class M3U8Player {
public:
  M3U8Player(String url);
  ~M3U8Player();
  String stationUrl;
  float volume;
  void playAAC();
private:
  TaskHandle_t scrapeAACHandle;
  TaskHandle_t setBufferHandle;
  AudioGeneratorAAC *aac;
  AudioFileSourceBuffer *buff;
  AudioFileSourceBuffer *nextBuff;
  AudioOutputI2S *out;
  uint32_t buffSize;
  uint8_t targetDuration;
  StringQueue aacUrls;
  StringStack m3u8Urls;
  bool isChannelChanged;
  bool needNextBuff;
  void scrapeM3U8();
  static void scrapeAAC(void *args);
  static void setBuffer(void *args);
};
