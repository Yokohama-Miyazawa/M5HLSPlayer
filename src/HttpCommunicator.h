#include <Arduino.h>
#include <HTTPClient.h>
#include "Stack.h"
#include "IndexQueue.h"

#define KILO 1000

typedef struct{
  int code;
  String payload;
} response;

String dToH(int decimal);
String urlEncode(const String &url);
String convertHTTPStoHTTP(const String &url);
bool isCode3XX(const int &code);
response getRequest(const String &url);
int parseResponse(const response &res, uint8_t &duration, Stack<String> &m3u8Urls, IndexQueue<String> &aacUrls);
