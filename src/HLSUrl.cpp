/*
  HLSUrl
  Storage of HLS URLs
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

int HLSUrl::rearMargin()
{
  return segmentUrls->rearMargin();
}

String HLSUrl::former()
{
  return segmentUrls->former();
}
