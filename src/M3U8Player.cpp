#include "M3U8Player.h"


M3U8Player::M3U8Player(String url)
{
  scrapeAACHandle = NULL;
  setBufferHandle = NULL;
  volume = 5.0;
  buffSize = 4096;
  targetDuration = 10;
  isChannelChanged = false;
  needNextBuff = false;
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
}

M3U8Player::~M3U8Player(){
  vTaskDelete(scrapeAACHandle);
  vTaskDelete(setBufferHandle);
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
    }
    delay(100);
  }
}

void M3U8Player::playAAC()
{
  bool isNextBuffPrepared = false;
  needNextBuff = true;
  while (aacUrls.length() == 0)
  {
    Serial.println(".aac queue is empty.");
    delay(1000);
  }
  while (!nextBuff){ delay(100); }
  buff = nextBuff;

  while (true)
  {
    if (!aac->begin(buff, out))
    {
      Serial.println("Player start failed.");
      M5.Lcd.println("Player start failed.");
      return;
    }
    while (aac->isRunning())
    {
      if (!isNextBuffPrepared && buff->getSize() < 3 * buff->getPos())
      {
        isNextBuffPrepared = true;
        log_i("urls: %d", aacUrls.length());
        if (aacUrls.length() == 0){ continue; }
        log_i("prepare next buffer");
        needNextBuff = true;
      }
      if (!aac->loop())
      {
        aac->stop();
        buff->close();
        delete buff;
        buff = nextBuff;
        nextBuff = NULL;
        isNextBuffPrepared = false;
      }
    }
  }
}
