#include "HLSUrl.h"

HLSUrl::HLSUrl(String url)
{
  targetDuration = 10;
  searchPlaylistUrl(url);
}

HLSUrl::~HLSUrl()
{
}

void HLSUrl::searchPlaylistUrl(const String url)
{
  String res;
  uint8_t status;
  m3u8Urls.push(url);
  do
  {
    res = getRequest(m3u8Urls.peek());
    status = parseResponse(res, targetDuration, m3u8Urls, segmentUrls);
    log_v("status: %d", status);
    delay(100);
  } while (status != 1);
  playlistUrl = m3u8Urls.peek();
  return;
}

bool HLSUrl::crawlSegmentUrl()
{
  if(!m3u8Urls.depth()) return false;
  String res = getRequest(playlistUrl);
  uint8_t status = parseResponse(res, targetDuration, m3u8Urls, segmentUrls);
  if(status == 0) return false;
  return true;
}

uint8_t HLSUrl::getTargetDuration()
{
  return targetDuration;
}

int HLSUrl::length()
{
  return segmentUrls.length();
}

String HLSUrl::pop()
{
  return segmentUrls.pop();
}
