#include "M3U8Player.h"

M3U8Player::M3U8Player(String url)
{
  scrapeAACHandle = NULL;
  setBufferHandle = NULL;
  playAACHandle = NULL;
  volume = 5.0;
  buffSize = 4096;
  targetDuration = 10;
  isChannelChanged = false;
  needNextBuff = false;
  isPlaying = false;
  stationUrl = url;
  m3u8Urls.push(stationUrl);

  out = new AudioOutputI2S(0, 1);
  out->SetOutputModeMono(true);
  out->SetGain(volume / 100.0);
  aac = new AudioGeneratorAAC();

  delay(1000);
  scrapeM3U8();
  xTaskCreatePinnedToCore(this->scrapeAAC, "scrapeAAC", 2048 * 3, this, 0, &scrapeAACHandle, 0);
  xTaskCreatePinnedToCore(this->setBuffer, "setBuffer", 2048 * 1, this, 1, &setBufferHandle, 0);
  xTaskCreatePinnedToCore(this->playAAC,   "playAAC",   2048 * 1, this, 2, &playAACHandle,   1);
}

M3U8Player::M3U8Player(String url, const float &startVolume)
{
  scrapeAACHandle = NULL;
  setBufferHandle = NULL;
  playAACHandle = NULL;
  volume = startVolume;
  buffSize = 4096;
  targetDuration = 10;
  isChannelChanged = false;
  needNextBuff = false;
  isPlaying = false;
  stationUrl = url;
  m3u8Urls.push(stationUrl);

  out = new AudioOutputI2S(0, 1);
  out->SetOutputModeMono(true);
  out->SetGain(volume / 100.0);
  aac = new AudioGeneratorAAC();

  delay(1000);
  scrapeM3U8();
  xTaskCreatePinnedToCore(this->scrapeAAC, "scrapeAAC", 2048 * 3, this, 0, &scrapeAACHandle, 0);
  xTaskCreatePinnedToCore(this->setBuffer, "setBuffer", 2048 * 1, this, 1, &setBufferHandle, 0);
  xTaskCreatePinnedToCore(this->playAAC,   "playAAC",   2048 * 1, this, 2, &playAACHandle,   1);
}

M3U8Player::~M3U8Player(){
  vTaskDelete(scrapeAACHandle);
  vTaskDelete(setBufferHandle);
  vTaskDelete(playAACHandle);
  delete out;
  delete aac;
  log_d("M3U8Player destructed.");
}

void M3U8Player::scrapeM3U8()
{
  String res;
  uint8_t status;
  do
  {
    res = getRequest(m3u8Urls.peek());
    status = parseResponse(res, targetDuration, m3u8Urls, aacUrls);
    log_v("status: %d", status);
    delay(100);
  } while (status != 1);
  isChannelChanged = false;
  return;
}

void M3U8Player::scrapeAAC(void* m3u8PlayerInstance)
{
  M3U8Player* instance = (M3U8Player*) m3u8PlayerInstance;
  String res;
  uint8_t status;
  uint32_t lastRequested = millis();

  while (true)
  {
    if (instance->isChannelChanged)
      instance->scrapeM3U8();
    if (millis() - lastRequested >= instance->targetDuration * KILO)
    {
      res = getRequest(instance->m3u8Urls.peek());
      status = parseResponse(res, instance->targetDuration, instance->m3u8Urls, instance->aacUrls);
      if (status == 0)
      {
        log_i("URL of .aac not found, retry...");
        delay(3 * KILO);
        continue;
      }
      lastRequested = millis();
    }
    delay(100);
  }
}

void M3U8Player::setBuffer(void *m3u8PlayerInstance)
{
  M3U8Player *instance = (M3U8Player *)m3u8PlayerInstance;
  AudioFileSourceHTTPStream *file;
  String convertedUrl;
  while (true)
  {
    if (instance->needNextBuff)
    {
      convertedUrl = convertHTTPStoHTTP(instance->aacUrls.pop());
      log_i("%s", convertedUrl.c_str());
      file = new AudioFileSourceHTTPStream(convertedUrl.c_str());
      instance->nextBuff = new AudioFileSourceBuffer(file, instance->buffSize);
      instance->needNextBuff = false;
      instance->fileQueue.push(file);
    }
    delay(100);
  }
}

void M3U8Player::playAAC(void *m3u8PlayerInstance)
{
  M3U8Player *instance = (M3U8Player *)m3u8PlayerInstance;
  restart:
  bool isNextBuffPrepared = false;
  instance->needNextBuff = true;
  while (!instance->isPlaying || instance->aacUrls.length() == 0){ delay(1000); }
  while (!instance->nextBuff){ delay(100); }
  instance->buff = instance->nextBuff;

  while (true)
  {
    if (!instance->aac->begin(instance->buff, instance->out))
    {
      Serial.println("Player start failed.");
      instance->buff->close();
      delete instance->buff;
      instance->nextBuff = NULL;
      goto restart;
    }
    while (instance->aac->isRunning())
    {
      if (!isNextBuffPrepared && instance->buff->getSize() < 3 * instance->buff->getPos())
      {
        isNextBuffPrepared = true;
        log_i("urls: %d", instance->aacUrls.length());
        if (instance->aacUrls.length() == 0){ continue; }
        log_i("prepare next buffer");
        instance->needNextBuff = true;
      }
      if (!instance->aac->loop())
      {
        instance->aac->stop();
        instance->buff->close();
        delete instance->fileQueue.pop();
        delete instance->buff;
        instance->buff = instance->nextBuff;
        instance->nextBuff = NULL;
        isNextBuffPrepared = false;
      }
    }
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
  m3u8Urls.clear();
  m3u8Urls.push(url);
  isChannelChanged = true;
  uint8_t leftNum = 1;
  if(aacUrls.length() >= leftNum){ aacUrls.tearOff(leftNum); }
  return true;
}

String M3U8Player::getStationURL()
{
  return stationUrl;
}
