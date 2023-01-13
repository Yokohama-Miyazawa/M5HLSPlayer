#include "HttpCommunicator.h"

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

String decompressGZIPStream(WiFiClient *gzipStream){
  unsigned int dlen;
  unsigned char *source, *dest;
  int res;

  uzlib_init();

  const unsigned int len = gzipStream->available();
  log_i("len: %d", len);
  source = (unsigned char *)malloc(len);
  const unsigned int c = gzipStream->readBytes(source, len);
  if (c != len)
  {
    log_e("Error reading bytes. stream length: %d, actualy read: %d", len, c);
    exit(1);
  }

  log_i("source[len - 4]: %d, source[len - 3]: %d, source[len - 2]: %d,  source[len - 1]: %d",
        source[len - 4], source[len - 3], source[len - 2], source[len - 1]);
  // 解凍後の長さを計算
  dlen = source[len - 1];
  dlen = 256 * dlen + source[len - 2];
  dlen = 256 * dlen + source[len - 3];
  dlen = 256 * dlen + source[len - 4];

  const unsigned int outlen = dlen;
  log_i("outlen: %d", outlen);

  // 計算したdlenと実際の長さが異なる場合があるので、余分に長さを確保する
  dlen++;
  dest = (unsigned char *)malloc(dlen);

  struct uzlib_uncomp d;
  uzlib_uncompress_init(&d, NULL, 0);

  d.source = source;
  d.source_limit = source + len - 4;
  d.source_read_cb = NULL;

  res = uzlib_gzip_parse_header(&d);
  if (res != TINF_OK)
  {
    log_e("Error parsing header: %d", res);
    exit(1);
  }

  d.dest_start = d.dest = dest;

  while (dlen)
  {
    unsigned int chunk_len = dlen < OUT_CHUNK_SIZE ? dlen : OUT_CHUNK_SIZE;
    d.dest_limit = d.dest + chunk_len;
    res = uzlib_uncompress_chksum(&d);
    dlen -= chunk_len;
    log_v("res: %d, dlen: %d", res, dlen);
    if (res != TINF_OK)
    {
      break;
    }
  }

  if (res != TINF_DONE)
  {
    log_e("Error during decompression: %d", res);
    exit(-res);
  }

  log_i("decompressed %lu bytes", d.dest - dest);
  log_d("RESULT\n%s", dest);
  String unzipped = String((char *)dest).substring(0, outlen);
  free(source);
  free(dest);
  log_d("decompressed text: %s\ndecompressed text end", unzipped.c_str());

  return unzipped;
}

response getRequest(const String &url, const char* rootca)
{
  HTTPClient http;
  response response;
  log_i("URL: %s", url.c_str());

  const size_t headerKeysCount = 1;
  const char *headerKeys[headerKeysCount] = {"Content-Encoding"};
  http.collectHeaders(headerKeys, headerKeysCount);

  if(rootca == NULL || url.indexOf("https") != 0){
    log_i("HTTP");
    http.begin(convertHTTPStoHTTP(url).c_str());
  }else{
    log_i("HTTPS");
    http.begin(url.c_str(), rootca);
  }
  int httpCode = http.GET();
  response.code = httpCode;
  if (httpCode > 0)
  {
    log_i("[HTTP] GET... code: %d", httpCode);
    if (httpCode == HTTP_CODE_OK)
    {
      log_d("Content-Encoding: %s", http.header("Content-Encoding").c_str());
      if (!http.header("Content-Encoding").compareTo("gzip"))
      {
        log_i("GZIP");
        response.payload = decompressGZIPStream(http.getStreamPtr());
      } else {
        log_i("not GZIP");
        response.payload = http.getString();
      }
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
      // or
      // final line without lf
      currentLine = payload.substring(currentHead, length);
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
        m3u8Urls.push(newUrl);
      }
      else if (!m3u8Urls.search(newUrl))
      {
        m3u8Urls.push(newUrl);
      }
    }
  }
}
