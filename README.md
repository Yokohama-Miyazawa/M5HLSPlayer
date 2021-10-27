# M5M3U8Player  
M5StackでM3U8形式のWebラジオを再生するプログラム  

## 前提ライブラリ  
[ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio)  
ただし、下記2箇所にコードを追加する必要あり。  

### AudioFileSourceHTTPStream.cpp  
```
bool AudioFileSourceHTTPStream::close()
{
  http.end();
  client.stop();  // このコードを追加する
  return true;
}
```

### AudioFileSourceBuffer.cpp  
```
AudioFileSourceBuffer::~AudioFileSourceBuffer()
{
  if (deallocateBuffer) free(buffer);
  buffer = NULL;
  delete src;  // このコードを追加する
}
```

## 使い方  
### PlatformIO  
プロジェクトを作成し、libディレクトリにこのリポジトリを配置する。  
examplesディレクトリの例を参考に、main.cppからこのライブラリを呼び出す。  
