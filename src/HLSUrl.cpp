#include "HLSUrl.h"

HLSUrl::HLSUrl(String url)
{
  m3u8Urls = new Stack<String>;
  segmentUrls = new IndexQueue<String>;
  targetDuration = 10;
  searchPlaylistUrl(url);
}

HLSUrl::~HLSUrl()
{
  delete m3u8Urls;
  delete segmentUrls;
}

void HLSUrl::searchPlaylistUrl(const String url)
{
  String res;
  uint8_t status;
  m3u8Urls->push(url);
  do
  {
    res = getRequest(m3u8Urls->peek());
    status = parseResponse(res, targetDuration, *m3u8Urls, *segmentUrls);
    log_v("status: %d", status);
    delay(100);
  } while (status != 1);
  playlistUrl = m3u8Urls->peek();
  return;
}

bool HLSUrl::crawlSegmentUrl()
{
  if(!m3u8Urls->depth()) return false;
  String res = getRequest(playlistUrl);
  uint8_t status = parseResponse(res, targetDuration, *m3u8Urls, *segmentUrls);
  if(status != 1) return false;
  return true;
}

uint8_t HLSUrl::getTargetDuration()
{
  return targetDuration;
}

int HLSUrl::length()
{
  return segmentUrls->length();
}

int HLSUrl::margin()
{
  return segmentUrls->margin();
}

String HLSUrl::next()
{
  return segmentUrls->next();
}
