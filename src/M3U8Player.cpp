#include "M3U8Player.h"

M3U8Player::M3U8Player(String url, const float &startVolume, const bool &isAutoStart, const uint32_t &bufferSize)
{
  state = M3U8Player_State::SETUP;
  scrapeAACHandle = NULL;
  playAACHandle = NULL;
  volume = startVolume;
  buffSize = bufferSize;
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

  xTaskCreatePinnedToCore(this->scrapeAAC, "scrapeAAC", 2048 * 2, this, 0, &scrapeAACHandle, 0);
  xTaskCreatePinnedToCore(this->playAAC,   "playAAC",   2048 * 2, this, 2, &playAACHandle, 1);

  targetDuration = urls->getTargetDuration();
  log_e("Target Duration: %d", targetDuration);
  state = M3U8Player_State::STANDBY;
  if(isAutoStart) start();
}

M3U8Player::M3U8Player(String url) : M3U8Player(url, 5.0, false, 4096){}

M3U8Player::M3U8Player(String url, const float &startVolume) : M3U8Player(url, startVolume, false, 4096){}

M3U8Player::M3U8Player(String url, const float &startVolume, const bool &isAutoStart) : M3U8Player(url, startVolume, isAutoStart, 4096){}

M3U8Player::~M3U8Player(){
  vTaskDelete(scrapeAACHandle);
  vTaskDelete(playAACHandle);
  delete out;
  delete ts;
  delete urls;
  log_d("M3U8Player destructed.");
}

void M3U8Player::setBuffer(HLSUrl* urlForBuff)
{
  while(!urlForBuff->margin()) delay(100);
  String convertedUrl = convertHTTPStoHTTP(urlForBuff->next());
  log_e("Making a buffer for %s", convertedUrl.c_str());
  AudioFileSourceHTTPStream *file = new AudioFileSourceHTTPStream(convertedUrl.c_str());
  bool isTS = (convertedUrl.indexOf(".ts") >= 0) ? true : false;
  log_e("isTS: %s", isTS ? "true" : "false");
  nextBuff = new AudioFileSourceHLSBuffer(file, buffSize, isTS);
  log_e("setBuffer Complete.");
}

bool M3U8Player::recovery()
{
  int margin = urls->margin();
  if (margin >= SOURCE_QUEUE_CAPACITY) return true;
  int rearMargin = urls->rearMargin();
  int lengthHaveToBack = SOURCE_QUEUE_CAPACITY - margin;
  if (lengthHaveToBack <= rearMargin)
  {
    for (int i = 0; i < lengthHaveToBack; i++) urls->former();
    return true;
  }
  return urls->crawlSegmentUrl();
}

void M3U8Player::changeChannel()
{
  log_e("Change Channel");
  ts->stop();
  delete buff;
  delete urls;
  buff = nextBuff;
  urls = nextUrls;
  targetDuration = urls->getTargetDuration();
  log_e("Target Duration: %d", targetDuration);
  ts->reset();
  ts->switchMode(buff->isTS());
  nextBuff = NULL;
  nextUrls = NULL;
  isChannelChanging = false;
  state = M3U8Player_State::PLAYING;
  log_e("Changing channel completed.");
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
      log_e("playAAC   Stack: %d", uxTaskGetStackHighWaterMark(instance->playAACHandle));
      log_e("scrapeAAC Stack: %d", uxTaskGetStackHighWaterMark(instance->scrapeAACHandle));
      if(instance->urls->crawlSegmentUrl()) lastRequested = millis();
      instance->isReferringUrls = false;
    }
    while (instance->buff && instance->buff->isSetup() && !instance->buff->isFullSourceQueue() && instance->urls->margin())
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
    } else {
      instance->state = M3U8Player_State::PLAYING;
    }
    while (instance->ts->isRunning())
    {
      if (!instance->ts->loop())
      {
        instance->state = M3U8Player_State::OTHERS;
        Serial.println("Playback stopped.");
        if(!instance->recovery())
        {
          unsigned long lastStopped = millis();
          while (millis() - lastStopped < instance->targetDuration * KILO) delay(10);
        }
        Serial.println("Playback starting...");
        instance->state = M3U8Player_State::PLAYING;
        continue;
      }
      if (instance->isChannelChanging && instance->nextBuff && instance->nextBuff->isSetup())
      {
        instance->changeChannel();
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

M3U8Player_State M3U8Player::getState()
{
  return state;
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
  state = M3U8Player_State::CHANNEL_CHANGING;
  isChannelChanging = true;
  stationUrl = url;
  nextUrls = new HLSUrl(stationUrl);
  setBuffer(nextUrls);
  return true;
}

String M3U8Player::getStationURL()
{
  return stationUrl;
}
