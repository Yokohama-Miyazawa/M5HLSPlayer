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
  response res;
  enum ParseResponseStatus status;
  m3u8Urls->push(url);
  do
  {
    res = getRequest(m3u8Urls->peek());
    status = parseResponse(res, targetDuration, *m3u8Urls, *segmentUrls);
  } while (status != AAC_OR_TS);
  return;
}

bool HLSUrl::crawlSegmentUrl()
{
  if(!m3u8Urls->depth()) return false;
  response res = getRequest(m3u8Urls->peek());
  enum ParseResponseStatus status = parseResponse(res, targetDuration, *m3u8Urls, *segmentUrls);
  if(status != AAC_OR_TS) return false;
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
