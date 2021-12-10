#include <Arduino.h>
#include <HTTPClient.h>
#include <AudioFileSourceHTTPStream.h>
#include <AudioFileSourceBuffer.h>
#include <AudioOutputI2S.h>
#include "HttpCommunicator.h"
#include "AudioGeneratorTS.h"

typedef struct {
  bool isTS;
  AudioFileSourceBuffer *buffer;
} sourceBuffer;

class M3U8Player {
public:
  M3U8Player(String url);
  M3U8Player(String url, const float &startVolume);
  ~M3U8Player();
  String stationUrl;
  float volume;
  bool start();
  void setVolume(const float &newVolume);
  float getVolume();
  bool changeStationURL(const String &url);
  String getStationURL();
private:
  TaskHandle_t scrapeAACHandle;
  TaskHandle_t setBufferHandle;
  TaskHandle_t playAACHandle;
  AudioGeneratorTS *ts;
  sourceBuffer buff;
  sourceBuffer nextBuff;
  AudioOutputI2S *out;
  uint32_t buffSize;
  uint8_t targetDuration;
  Queue<String> aacUrls;
  Stack<String> m3u8Urls;
  Queue<AudioFileSourceHTTPStream*> fileQueue;
  bool isChannelChanged;
  bool needNextBuff;
  bool isPlaying;
  void scrapeM3U8();
  static void scrapeAAC(void *args);
  static void setBuffer(void *args);
  static void playAAC(void *args);
};
