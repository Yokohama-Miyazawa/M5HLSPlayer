#include "HttpCommunicator.h"

String hex[16] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F"};

String dToH(int decimal)
{
  String result = hex[decimal / 16] + hex[decimal % 16];
  return result;
}

String urlEncode(const String &url)
{
  String encoded = "";
  for (int i = 0; i < url.length(); i++)
  {
    int code = (int)url[i];
    if ((code == 38) ||                // &
        (45 <= code && code <= 58) ||  // -./: and 0-9
        (code == 61 || code == 63) ||  // = and ?
        (65 <= code && code <= 90) ||  // A-Z
        (code == 95) ||                // _
        (97 <= code && code <= 122) || // a-z
        (code == 126))                 // ~
    {
      encoded += url[i];
    }
    else
    {
      encoded += "%" + dToH(code);
    }
  }
  return encoded;
}

String convertHTTPStoHTTP(const String &url)
{
  if (url.indexOf("https") < 0 && url.indexOf("http") == 0)
    return url;
  if (url.indexOf("https") == 0)
    return "http" + url.substring(5);

  log_i("The url is wrong format.");
  return "";
}

bool isCode3XX(const int &code){
  return (code == HTTP_CODE_MOVED_PERMANENTLY || code == HTTP_CODE_FOUND || code == HTTP_CODE_SEE_OTHER ||
          code == HTTP_CODE_TEMPORARY_REDIRECT || code == HTTP_CODE_PERMANENT_REDIRECT);
}

response getRequest(const String &url)
{
  HTTPClient http;
  response response;
  log_i("URL: %s", url.c_str());
  String encodedUrl = urlEncode(url);
  log_i("encoded URL: %s", encodedUrl.c_str());

  http.begin(encodedUrl.c_str());
  int httpCode = http.GET();
  response.code = httpCode;
  if (httpCode > 0)
  {
    log_i("[HTTP] GET... code: %d", httpCode);
    if (httpCode == HTTP_CODE_OK)
    {
      String payload = http.getString();
      log_d("%s", payload.c_str());
      response.payload = payload;
    }
    else if (isCode3XX(httpCode))
    {
      response.payload = http.getLocation();
      log_d("CODE 3XX LOCATION: %s", response.payload.c_str());
    }
    else
    {
      response.payload = "NO PAYLOAD";
    }
  }
  else
  {
    log_i("[HTTP] GET... failed, error: %s", http.errorToString(httpCode).c_str());
    response.payload = "ERROR: " + http.errorToString(httpCode);
  }

  http.end();
  return response;
}

enum ParseResponseStatus parseResponse(const response &res, uint8_t &duration, Stack<String> &m3u8Urls, IndexQueue<String> &aacUrls)
{
  ParseResponseStatus status = OTHERS;
  int httpCode = res.code;
  String payload = res.payload;
  int32_t length = payload.length();
  int16_t currentHead = 0;
  int16_t cr, lf;
  String currentLine, newUrl;

  while (true)
  {
    if (currentHead >= length) return status;
    cr = payload.indexOf('\r', currentHead);
    lf = payload.indexOf('\n', currentHead);
    log_v("CURRENT HEAD: %d CR: %d LF: %d", currentHead, cr, lf);
    if (cr >= 0)
    { // CRLF
      currentLine = payload.substring(currentHead, cr);
    }
    else if (lf >= 0)
    { // LF
      currentLine = payload.substring(currentHead, lf);
    }
    else
    { // only one line
      currentLine = payload;
    }
    currentHead = (lf >= 0) ? lf + 1 : length;
    log_d("CURRENT LINE: %s", currentLine.c_str());
    if (currentLine.indexOf("#EXT-X-TARGETDURATION:") == 0)
    {
      duration = currentLine.substring(22).toInt();
    }
    if (currentLine.indexOf(".aac") >= 0 || currentLine.indexOf(".ts") >= 0)
    {
      if (status == OTHERS)
        status = AAC_OR_TS;
      if (currentLine.indexOf("http") == 0)
      {
        newUrl = currentLine;
      }
      else
      {
        while(!m3u8Urls.depth()){ delay(100); }
        String latestM3u8Url = m3u8Urls.peek();
        uint32_t lastSlashOfM3u8 = latestM3u8Url.lastIndexOf('/');
        newUrl = latestM3u8Url.substring(0, lastSlashOfM3u8 + 1) + currentLine;
        log_v("cr: %d, lf: %d, null: %d, length: %d",
              newUrl.indexOf('\r', 0), newUrl.indexOf('\n', 0), newUrl.indexOf('\0', 0), newUrl.length());
      }
      if (!aacUrls.search(newUrl))
        aacUrls.push(newUrl);
    }
    else if (currentLine.indexOf(".m3u8") >= 0)
    {
      if (status == OTHERS)
        status = M3U8;
      if (currentLine.indexOf("http") == 0)
      {
        newUrl = currentLine;
      }
      else
      {
        while(!m3u8Urls.depth()){ delay(100); }
        String latestM3u8Url = m3u8Urls.peek();
        uint32_t lastSlashOfM3u8 = latestM3u8Url.lastIndexOf('/');
        newUrl = latestM3u8Url.substring(0, lastSlashOfM3u8 + 1) + currentLine;
      }
      if (isCode3XX(httpCode))
      {
        m3u8Urls.pop();
        m3u8Urls.push(convertHTTPStoHTTP(newUrl));
      }
      else if (!m3u8Urls.search(newUrl))
      {
        m3u8Urls.push(convertHTTPStoHTTP(newUrl));
      }
    }
  }
}
