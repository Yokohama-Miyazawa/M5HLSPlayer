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

String getRequest(const String &url)
{
  HTTPClient http;
  String response;
  log_i("URL: %s", url.c_str());
  String encodedUrl = urlEncode(url);
  log_i("encoded URL: %s", encodedUrl.c_str());

  http.begin(encodedUrl.c_str());
  int httpCode = http.GET();
  if (httpCode > 0)
  {
    log_i("[HTTP] GET... code: %d", httpCode);
    if (httpCode == HTTP_CODE_OK)
    {
      String payload = http.getString();
      log_d("%s", payload.c_str());
      response = payload;
    }
    else
    {
      response = "NO PAYLOAD";
    }
  }
  else
  {
    log_i("[HTTP] GET... failed, error: %s", http.errorToString(httpCode).c_str());
    response = "ERROR: " + http.errorToString(httpCode);
  }

  http.end();
  return response;
}

int parseResponse(const String &res, uint8_t &duration, StringStack &m3u8Urls, StringQueue &aacUrls)
{
  uint8_t status = 0; // found .acc: 1, found .m3u8: 2, others: 0
  int32_t length = res.length();
  int16_t currentHead = 0;
  int16_t cr, lf;
  String currentLine, newUrl;

  while (true)
  {
    if (currentHead >= length)
    {
      return status;
    }
    cr = res.indexOf('\r', currentHead);
    lf = res.indexOf('\n', currentHead);
    log_v("CURRENT HEAD: %d CR: %d LF: %d", currentHead, cr, lf);
    if (cr >= 0)
    { // CRLF
      currentLine = res.substring(currentHead, cr);
    }
    else
    { // LF
      currentLine = res.substring(currentHead, lf);
    }
    currentHead = lf + 1;
    log_d("CURRENT LINE: %s", currentLine.c_str());
    if (currentLine.indexOf("#EXT-X-TARGETDURATION:") == 0)
    {
      duration = currentLine.substring(22).toInt();
    }
    if (currentLine.indexOf(".aac") >= 0)
    {
      if (!status)
        status = 1;
      if (currentLine.indexOf("http") == 0)
      {
        newUrl = currentLine;
      }
      else
      {
        String latestM3u8Url = m3u8Urls.peek();
        uint32_t lastSlashOfM3u8 = latestM3u8Url.lastIndexOf('/');
        newUrl = latestM3u8Url.substring(0, lastSlashOfM3u8 + 1) + currentLine;
        log_v("cr: %d, lr: %d, null: %d, length: %d",
              newUrl.indexOf('\r', 0), newUrl.indexOf('\n', 0), newUrl.indexOf('\0', 0), newUrl.length());
      }
      if (!aacUrls.search(newUrl))
        aacUrls.push(newUrl);
    }
    else if (currentLine.indexOf(".m3u8") >= 0)
    {
      if (!status)
        status = 2;
      newUrl = currentLine;
      if (!m3u8Urls.search(newUrl))
        m3u8Urls.push(convertHTTPStoHTTP(newUrl));
    }
  }
}
