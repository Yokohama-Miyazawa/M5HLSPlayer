#include "M3U8Player.h"

M3U8Player::M3U8Player(String url)
{
  scrapeAACHandle = NULL;
  playAACHandle = NULL;
  volume = 5.0;
  buffSize = 4096;
  isReferringUrls = false;
  isChannelChanging = false;
  isPlaying = false;
  stationUrl = url;
  buff = NULL;
  nextBuff = NULL;
  nextUrls = NULL;

  urls = new HLSUrl(stationUrl);

  out = new AudioOutputI2S(0, 1);
  out->SetOutputModeMono(true);
  out->SetGain(volume / 100.0);
  ts = new AudioGeneratorTS();

  xTaskCreatePinnedToCore(this->scrapeAAC, "scrapeAAC", 2048 * 3, this, 0, &scrapeAACHandle, 0);
  xTaskCreatePinnedToCore(this->playAAC,   "playAAC",   2048 * 4, this, 2, &playAACHandle,   1);

  targetDuration = urls->getTargetDuration();
}

M3U8Player::M3U8Player(String url, const float &startVolume)
{
  scrapeAACHandle = NULL;
  playAACHandle = NULL;
  volume = startVolume;
  buffSize = 4096;
  isReferringUrls = false;
  isChannelChanging = false;
  isPlaying = false;
  stationUrl = url;
  buff = NULL;
  nextBuff = NULL;
  nextUrls = NULL;

  urls = new HLSUrl(stationUrl);

  out = new AudioOutputI2S(0, 1);
  out->SetOutputModeMono(true);
  out->SetGain(volume / 100.0);
  ts = new AudioGeneratorTS();

  xTaskCreatePinnedToCore(this->scrapeAAC, "scrapeAAC", 2048 * 3, this, 0, &scrapeAACHandle, 0);
  xTaskCreatePinnedToCore(this->playAAC,   "playAAC",   2048 * 4, this, 2, &playAACHandle,   1);

  targetDuration = urls->getTargetDuration();
}

M3U8Player::~M3U8Player(){
  vTaskDelete(scrapeAACHandle);
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
      log_i("Not Playing Now...");
      delay(100);
      continue;
    }
    if (instance->isChannelChanging) {
      log_e("Now channel changing...");
      lastRequested = 0;
      delay(100);
      continue;
    }
    if ((millis() - lastRequested >= instance->targetDuration * KILO))
    {
      instance->isReferringUrls = true;
      log_e("playAAC Stack: %d", uxTaskGetStackHighWaterMark(instance->playAACHandle));
      if(instance->urls->crawlSegmentUrl()) lastRequested = millis();
      instance->isReferringUrls = false;
    }
    while (instance->buff->isSetup() && !instance->buff->isFullSourceQueue() && instance->urls->margin())
    {
      instance->isReferringUrls = true;
      String convertedUrl = convertHTTPStoHTTP(instance->urls->next());
      log_e("%s", convertedUrl.c_str());
      AudioFileSourceHTTPStream *file = new AudioFileSourceHTTPStream(convertedUrl.c_str());
      instance->isReferringUrls = false;
      instance->buff->addSource(file);
    }
    delay(1);
  }
}

void M3U8Player::setBuffer(HLSUrl* urlForBuff)
{
  while(!urlForBuff->margin()) delay(100);
  String convertedUrl = convertHTTPStoHTTP(urlForBuff->next());
  log_e("Making a buffer for %s", convertedUrl.c_str());
  AudioFileSourceHTTPStream *file = new AudioFileSourceHTTPStream(convertedUrl.c_str());
  bool isTS = (convertedUrl.indexOf(".ts") >= 0) ? true : false;
  log_e("isTS:%d", isTS);
  nextBuff = new AudioFileSourceHLSBuffer(file, buffSize, isTS);
  log_e("setBuffer Complete.");
}

void M3U8Player::playAAC(void *m3u8PlayerInstance)
{
  M3U8Player *instance = (M3U8Player *)m3u8PlayerInstance;
  while (!instance->isPlaying){ delay(1000); }
  restart:
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
        //delete instance->urls;
        Serial.println("End of Play");
        instance->isPlaying = false;
        goto restart;
      }
      if (instance->isChannelChanging && instance->nextBuff && instance->nextBuff->isSetup())
      {
        log_e("Change Channel");
        instance->ts->stop();
        instance->buff->close();
        delete instance->buff;
        delete instance->urls;
        instance->buff = instance->nextBuff;
        instance->urls = instance->nextUrls;
        instance->targetDuration = instance->urls->getTargetDuration();
        instance->ts->reset();
        instance->ts->switchMode(instance->buff->isTS());
        instance->nextBuff = NULL;
        instance->nextUrls = NULL;
        instance->isChannelChanging = false;
        log_e("Changing channel completed.");
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
  while(isReferringUrls) delay(100);
  isChannelChanging = true;
  nextUrls = new HLSUrl(url);
  setBuffer(nextUrls);
  return true;
}

String M3U8Player::getStationURL()
{
  return stationUrl;
}
