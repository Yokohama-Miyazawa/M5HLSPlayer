#include "M3U8Player.h"

M3U8Player::M3U8Player(String url)
{
  scrapeAACHandle = NULL;
  setBufferHandle = NULL;
  playAACHandle = NULL;
  volume = 5.0;
  buffSize = 4096;
  isChannelChanged = false;
  isPlaying = false;
  stationUrl = url;
  urls = new HLSUrl(stationUrl);

  out = new AudioOutputI2S(0, 1);
  out->SetOutputModeMono(true);
  out->SetGain(volume / 100.0);
  ts = new AudioGeneratorTS();

  delay(1000);
  xTaskCreatePinnedToCore(this->scrapeAAC, "scrapeAAC", 2048 * 3, this, 0, &scrapeAACHandle, 0);
  xTaskCreatePinnedToCore(this->playAAC,   "playAAC",   2048 * 4, this, 2, &playAACHandle,   1);

  targetDuration = urls->getTargetDuration();
}

M3U8Player::M3U8Player(String url, const float &startVolume)
{
  scrapeAACHandle = NULL;
  setBufferHandle = NULL;
  playAACHandle = NULL;
  volume = startVolume;
  buffSize = 4096;
  isChannelChanged = false;
  isPlaying = false;
  stationUrl = url;
  urls = new HLSUrl(stationUrl);

  out = new AudioOutputI2S(0, 1);
  out->SetOutputModeMono(true);
  out->SetGain(volume / 100.0);
  ts = new AudioGeneratorTS();

  delay(1000);
  xTaskCreatePinnedToCore(this->scrapeAAC, "scrapeAAC", 2048 * 3, this, 0, &scrapeAACHandle, 0);
  xTaskCreatePinnedToCore(this->playAAC,   "playAAC",   2048 * 4, this, 2, &playAACHandle,   1);

  targetDuration = urls->getTargetDuration();
}

M3U8Player::~M3U8Player(){
  vTaskDelete(scrapeAACHandle);
  vTaskDelete(setBufferHandle);
  vTaskDelete(playAACHandle);
  delete out;
  delete ts;
  delete urls;
  log_d("M3U8Player destructed.");
}

void M3U8Player::scrapeAAC(void* m3u8PlayerInstance)
{
  M3U8Player* instance = (M3U8Player*) m3u8PlayerInstance;
  uint32_t lastRequested = 0;

  while (true)
  {
    if (!instance->isPlaying) {
      log_e("Not Playing Now...");
      delay(100);
      continue;
    }
    if (millis() - lastRequested >= instance->targetDuration * KILO)
    {
      log_i("playAAC Stack: %d", uxTaskGetStackHighWaterMark(instance->playAACHandle));
      instance->urls->crawlSegmentUrl();
      lastRequested = millis();

      while (instance->urls->length() && !instance->buff->isFullSourceQueue())
      {
        instance->buff->addSource(new AudioFileSourceHTTPStream(instance->urls->pop().c_str()));
      }
    }
    delay(100);
  }
}

void M3U8Player::setBuffer(HLSUrl* urlForBuff)
{
  String convertedUrl = convertHTTPStoHTTP(urlForBuff->pop());
  log_i("%s", convertedUrl.c_str());
  AudioFileSourceHTTPStream *file = new AudioFileSourceHTTPStream(convertedUrl.c_str());
  bool isTS = (convertedUrl.indexOf(".ts") >= 0) ? true : false;
  nextBuff = new AudioFileSourceHLSBuffer(file, buffSize, isTS);
}

void M3U8Player::playAAC(void *m3u8PlayerInstance)
{
  M3U8Player *instance = (M3U8Player *)m3u8PlayerInstance;
  restart:
  while (!instance->isPlaying){ delay(1000); }
  Serial.println("restarted or started.");
  instance->setBuffer(instance->urls);
  while (!instance->nextBuff){ delay(100); }
  instance->buff = instance->nextBuff;
  instance->ts->reset();
  instance->ts->switchMode(instance->buff->isTS());
  instance->nextBuff = NULL;

  while (true)
  {
    if (!instance->ts->begin(instance->buff, instance->out))
    {
      Serial.println("Player start failed.");
      if (instance->buff){
        instance->buff->close();
        delete instance->buff;
      }
      if(instance->urls) delete instance->urls;
      goto restart;
    }
    while (instance->ts->isRunning())
    {
      if (!instance->ts->loop())
      {
        instance->ts->stop();
        instance->buff->close();
        delete instance->buff;
        delete instance->urls;
        Serial.println("End of Play");
        instance->isPlaying = false;
        goto restart;
      }
      if (instance->isChannelChanged && instance->nextBuff)
      {
        Serial.println("Change Channel");
        instance->ts->stop();
        instance->buff->close();
        delete instance->buff;
        delete instance->urls;
        instance->buff = instance->nextBuff;
        instance->urls = instance->nextUrls;
        instance->nextBuff = NULL;
        instance->nextUrls = NULL;
        instance->isChannelChanged = false;
        break;
      }
      delay(1);
    }
    delay(1);
  }
}

bool M3U8Player::start()
{
  if(isPlaying) {
    Serial.println("Already Started.");
    return false;
  }
  isPlaying = true;
  Serial.println("Player Start.");
  return true;
}

void M3U8Player::setVolume(const float &newVolume)
{
  volume = newVolume;
  out->SetGain(volume / 100.0);
  return;
}

float M3U8Player::getVolume()
{
  return volume;
}

bool M3U8Player::changeStationURL(const String &url)
{
  if(url.indexOf("http") != 0)
  {
    log_i("invalid url");
    return false;
  }
  nextUrls = new HLSUrl(url);
  setBuffer(nextUrls);
  isChannelChanged = true;
  return true;
}

String M3U8Player::getStationURL()
{
  return stationUrl;
}
