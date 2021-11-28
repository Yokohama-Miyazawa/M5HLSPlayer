#include <Arduino.h>
#include <HTTPClient.h>
#include "Stack.h"
#include "Queue.h"

#define KILO 1000

String dToH(int decimal);
String urlEncode(const String &url);
String convertHTTPStoHTTP(const String &url);
String getRequest(const String &url);
int parseResponse(const String &res, uint8_t &duration, Stack<String> &m3u8Urls, Queue<String> &aacUrls);
