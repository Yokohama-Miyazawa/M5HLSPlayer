#include <Arduino.h>
#include "HttpCommunicator.h"

class HLSUrl {
public:
  HLSUrl(String url);
  ~HLSUrl();
  int length();
  String pop();
  bool crawlSegmentUrl();
  uint8_t getTargetDuration();
private:
  String playlistUrl;
  uint8_t targetDuration;
  Queue<String> segmentUrls;
  Stack<String> m3u8Urls;
  void searchPlaylistUrl(String url);
};
