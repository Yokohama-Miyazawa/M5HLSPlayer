#include <Arduino.h>
#include <HTTPClient.h>
#include <uzlib.h>
#include "Stack.h"
#include "IndexQueue.h"

#define KILO 1000
#define OUT_CHUNK_SIZE 1

enum ParseResponseStatus {
  AAC_OR_TS,
  M3U8,
  OTHERS
};

typedef struct{
  int code;
  String payload;
} response;

String convertHTTPStoHTTP(const String &url);
bool isCode3XX(const int &code);
String decompressGZIPStream(WiFiClient *gzipStream);
response getRequest(const String &url, const char *rootca = NULL);
enum ParseResponseStatus parseResponse(const response &res, uint8_t &duration, Stack<String> &m3u8Urls, IndexQueue<String> &aacUrls);
