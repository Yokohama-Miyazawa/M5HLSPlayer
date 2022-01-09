#include <Arduino.h>
#include <HTTPClient.h>
#include "Stack.h"
#include "IndexQueue.h"

#define KILO 1000

enum ParseResponseStatus {
  AAC_OR_TS,
  M3U8,
  OTHERS
};

typedef struct{
  int code;
  String payload;
} response;

String dToH(int decimal);
String urlEncode(const String &url);
String convertHTTPStoHTTP(const String &url);
bool isCode3XX(const int &code);
response getRequest(const String &url);
enum ParseResponseStatus parseResponse(const response &res, uint8_t &duration, Stack<String> &m3u8Urls, IndexQueue<String> &aacUrls);
