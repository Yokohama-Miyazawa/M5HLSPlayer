#include <Arduino.h>
#include <HTTPClient.h>
#include "StringQueue.h"
#include "StringStack.h"

#define KILO 1000

String dToH(int decimal);
String urlEncode(const String &url);
String convertHTTPStoHTTP(const String &url);
String getRequest(const String &url);
int parseResponse(const String &res, uint8_t &duration, StringStack &m3u8Urls, StringQueue &aacUrls);
