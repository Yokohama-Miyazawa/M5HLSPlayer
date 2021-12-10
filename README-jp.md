[English README](./README.md)
# M5M3U8Player  
M5StackおよびM5StickCでM3U8形式のWebラジオを再生するプログラム   
.aacと.tsに対応  

## 前提ライブラリ  
[ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio)  
このプログラムは最新版のESP8266Audioには非対応なので、[Release 1.9.3](https://github.com/earlephilhower/ESP8266Audio/releases/tag/1.9.3)を使用すること。  
また、下記の通りコードを追加する必要あり。  

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
プロジェクトを作成し、libディレクトリにこのリポジトリと前提ライブラリを配置する。  
examplesディレクトリの例を参考に、main.cppからこのライブラリを呼び出す。  
