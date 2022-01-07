#include <Arduino.h>
#include "HttpCommunicator.h"

class HLSUrl {
public:
  HLSUrl(String url);
  ~HLSUrl();
  int length();
  int margin();
  String next();
  bool crawlSegmentUrl();
  uint8_t getTargetDuration();
private:
  uint8_t targetDuration;
  IndexQueue<String> *segmentUrls;
  Stack<String> *m3u8Urls;
  void searchPlaylistUrl(String url);
};
