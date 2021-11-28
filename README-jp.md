[English README](./README.md)
# M5M3U8Player  
M5StackでM3U8形式のWebラジオを再生するプログラム  
.aacのみ対応。現在 .tsには非対応  

## 前提ライブラリ  
[ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio)  
ただし、下記の通りコードを追加する必要あり。  

### AudioFileSourceHTTPStream.cpp  
```
bool AudioFileSourceHTTPStream::close()
{
  http.end();
  client.stop();  // このコードを追加する
  return true;
}
```

## 使い方  
### PlatformIO  
プロジェクトを作成し、libディレクトリにこのリポジトリを配置する。  
examplesディレクトリの例を参考に、main.cppからこのライブラリを呼び出す。  
