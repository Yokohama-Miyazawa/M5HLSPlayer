[English README](./README.md)
# M5HLSPlayer  
M5StackおよびM5StickCでM3U8形式のWebラジオを再生するプログラム   
.aacと.tsに対応  

## 前提ライブラリ  
### [M5Stack](https://github.com/m5stack/M5Stack)  
### [ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio)  
また、下記の通りコードを追加する必要あり。  

#### AudioFileSourceHTTPStream.cpp  
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

### Arduino IDE  
スケッチを作成し、`Arduino/libraries/`ディレクトリにこのリポジトリと前提ライブラリを配置する。  
examplesディレクトリの例を参考に、.inoファイルからこのライブラリを呼び出す。  

