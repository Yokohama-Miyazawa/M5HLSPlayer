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

#include <Arduino.h>
#include "HttpCommunicator.h"

class HLSUrl {
public:
  HLSUrl(String url);
  ~HLSUrl();
  int length();
  int margin();
  int rearMargin();
  String next();
  String former();
  bool crawlSegmentUrl();
  uint8_t getTargetDuration();
private:
  uint8_t targetDuration;
  IndexQueue<String> *segmentUrls;
  Stack<String> *m3u8Urls;
  void searchPlaylistUrl(String url);
};
