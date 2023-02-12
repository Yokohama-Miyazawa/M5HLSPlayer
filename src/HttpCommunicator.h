/*
  HttpCommunicator
  Program for HTTP requests and parsing the responses
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
