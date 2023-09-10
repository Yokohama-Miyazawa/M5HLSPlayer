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

#include "M3U8Player.h"

M3U8Player::M3U8Player(String url, const float &startVolume, const bool &isAutoStart, const uint32_t &bufferSize, const bool &isCore2, const char* rootca)
{
  state = M3U8Player_State::SETUP;
  scrapeAACHandle = NULL;
  playAACHandle = NULL;
  volume = startVolume;
  buffSize = bufferSize;
  stationUrl = url;
  buff = NULL;
  nextBuff = NULL;
  nextUrls = NULL;
  rca = rootca;

  urls = new HLSUrl(stationUrl, rca);

  if(isCore2) {
    out = new AudioOutputI2S(0, 0);
    out->SetPinout(12, 0, 2);
  } else {
    out = new AudioOutputI2S(0, 1);
  }
  out->SetOutputModeMono(true);
  out->SetGain(volume / 100.0);
  ts = new AudioGeneratorTS();
  targetDuration = urls->getTargetDuration();
  log_e("Target Duration: %d", targetDuration);

  const BaseType_t resScrapeAAC = xTaskCreatePinnedToCore(this->scrapeAAC, "scrapeAAC", 2048 * 2, this, 0, &scrapeAACHandle, 0);
  if(resScrapeAAC != pdPASS){
    Serial.printf("Creating task for scrapeAAC failed. pdFREERTOS_ERRNO: %d\n", resScrapeAAC);
    exit(1);
  }

  state = M3U8Player_State::STANDBY;
  if(isAutoStart) start();
}

M3U8Player::M3U8Player(String url) : M3U8Player(url, 5.0, false, 4096, false, NULL){}

M3U8Player::M3U8Player(String url, const float &startVolume) : M3U8Player(url, startVolume, false, 4096, false, NULL){}

M3U8Player::M3U8Player(String url, const float &startVolume, const bool &isAutoStart) : M3U8Player(url, startVolume, isAutoStart, 4096, false, NULL){}

M3U8Player::M3U8Player(String url, const float &startVolume, const bool &isAutoStart, const uint32_t &bufferSize) : M3U8Player(url, startVolume, isAutoStart, bufferSize, false, NULL) {}

M3U8Player::M3U8Player(String url, const float &startVolume, const bool &isAutoStart, const uint32_t &bufferSize, const bool &isCore2) : M3U8Player(url, startVolume, isAutoStart, bufferSize, isCore2, NULL) {}

M3U8Player::~M3U8Player(){
  vTaskDelete(scrapeAACHandle);
  vTaskDelete(playAACHandle);
  ts->stop();
  delete out;
  delete ts;
  delete buff;
  delete urls;
  log_d("M3U8Player destructed.");
}

void M3U8Player::setBuffer(HLSUrl* urlForBuff)
{
  while(!urlForBuff->margin()) delay(100);
  String convertedUrl = convertHTTPStoHTTP(urlForBuff->next());
  log_e("Making a buffer for %s", convertedUrl.c_str());
  AudioFileSourceHTTPSStream *file = new AudioFileSourceHTTPSStream(convertedUrl.c_str(), rca);
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
  log_e("Changing channel completed.");
}

void M3U8Player::scrapeAAC(void* m3u8PlayerInstance)
{
  M3U8Player* instance = (M3U8Player*) m3u8PlayerInstance;
  uint32_t lastRequested = 0;

  while (true)
  {
    if (instance->state == M3U8Player_State::CHANNEL_CHANGING)
    {
      log_e("Now channel changing...");
      instance->setBuffer(instance->nextUrls);
      while(instance->ts->isRunning()) delay(10);
      instance->changeChannel();
      instance->state = M3U8Player_State::STANDBY;
      continue;
    }
    if (instance->state == M3U8Player_State::RECOVERY_SEGMENT)
    {
      log_e("Recovery Segments...");
      while (!instance->recovery())
      {
        delay(10);
        log_e("Recovery failed. Retrying...");
      }
      log_e("Recovery succeeded! Playback restarting...");
      instance->state = M3U8Player_State::STANDBY;
      continue;
    }
    if ((millis() - lastRequested >= instance->targetDuration * KILO))
    {
      if(instance->urls->crawlSegmentUrl()) lastRequested = millis();
    }
    while (instance->buff && instance->buff->isSetup() && !instance->buff->isFullSourceQueue() && instance->urls->margin())
    {
      String convertedUrl = convertHTTPStoHTTP(instance->urls->next());
      log_e("%s", convertedUrl.c_str());
      AudioFileSourceHTTPSStream *file = new AudioFileSourceHTTPSStream(convertedUrl.c_str(), instance->rca);
      instance->buff->addSource(file);
    }
    delay(1);
  }
}

void M3U8Player::playAAC(void *m3u8PlayerInstance)
{
  M3U8Player *instance = (M3U8Player *)m3u8PlayerInstance;
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
      goto restart;
    } else {
      instance->state = M3U8Player_State::PLAYING;
    }
    while (instance->ts->isRunning())
    {
      if (!instance->ts->loop() || instance->state == M3U8Player_State::CHANNEL_CHANGING)
      {
        Serial.println("Playback stopped.");
        instance->ts->stop();
        if (instance->state != M3U8Player_State::CHANNEL_CHANGING) {
          instance->ts->reset();
          instance->state = M3U8Player_State::RECOVERY_SEGMENT;
        }
        while(instance->state != M3U8Player_State::STANDBY){
          delay(100);
        }
        Serial.println("Playback starting...");
        break;
      }
      delay(1);
    }
    delay(1);
  }
}

bool M3U8Player::start()
{
  if(playAACHandle != NULL) {
    Serial.println("Already Started.");
    return false;
  }

  Serial.println("Player Start.");
  const BaseType_t resPlayAAC = xTaskCreatePinnedToCore(this->playAAC, "playAAC", 2048 * 2, this, 2, &playAACHandle, 1);
  if (resPlayAAC != pdPASS) {
    Serial.printf("Creating task for playAAC failed. pdFREERTOS_ERRNO: %d\n", resPlayAAC);
    exit(1);
  }
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
  stationUrl = url;
  nextUrls = new HLSUrl(stationUrl, rca);
  // After created nextUrls, move to CHANNEL_CHANGING.
  state = M3U8Player_State::CHANNEL_CHANGING;
  return true;
}

String M3U8Player::getStationURL()
{
  return stationUrl;
}
