[English README](./README.md)
# M5HLSPlayer  
M5StackおよびM5StickCでM3U8形式のWebラジオを再生するプログラム   
.aacと.tsに対応  

## 目次
1. [前提ライブラリ](./README-jp.md#前提ライブラリ)
2. [使い方](./README-jp.md#使い方)
3. [メンバ関数の説明](./README-jp.md#メンバ関数の説明)
4. [(任意)チャンネル切り替えの高速化](./README-jp.md#任意チャンネル切り替えの高速化)

## 前提ライブラリ  
### [M5Stack](https://github.com/m5stack/M5Stack)  
### [ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio)  
また、下記の通りコードを追加する必要あり。  

#### AudioFileSourceHTTPStream.cpp  
```C++
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

## メンバ関数の説明
### M3U8Player()
#### 説明:
`M3U8Player`クラスのコンストラクタ。  
`isAutoStart`の値を`true`にすると、セットアップ完了後に自動で再生を開始する。  
`isAutoStart`の値を`false`にした場合は、`start()`を明示的に呼び出す必要がある。  

#### 構文:
`M3U8Player(String url);`  
`M3U8Player(String url, const float &startVolume);`  
`M3U8Player(String url, const float &startVolume, const bool &isAutoStart);`  
`M3U8Player(String url, const float &startVolume, const bool &isAutoStart, const uint32_t &bufferSize);`  
`M3U8Player(String url, const float &startVolume, const bool &isAutoStart, const uint32_t &bufferSize, const bool &isCore2);`  

#### 引数:
| 引数 | 型 | 説明 |
| :---: | :---: | :---: |
| url | String | HLSプレイリストのURL |
| startVolume | float | 初期設定の音量(0-100, デフォルトは5) |
| isAutoStart | bool | セットアップ後、再生開始するか否か(デフォルトは`false`) |
| bufferSize | uint32_t | 音声データを保持するバッファのバイトサイズ(デフォルトは4096) |
| isCore2 | bool | M5Stack Core2 か否か (デフォルトは `false`) |

#### 使用例:
例1  
```C++
#include <M3U8Player.h>

M3U8Player *player;
String stationUrl = "http://xxxx/xxxx/playlist.m3u8";
float defaultVolume = 10.0;
player = new M3U8Player(stationUrl, defaultVolume, true);
```
例2  
```C++
#include <M3U8Player.h>

M3U8Player *player;
String stationUrl = "http://xxxx/xxxx/playlist.m3u8";
player = new M3U8Player(stationUrl);
player->start();
```

### ~M3U8Player()  
#### 説明:
`M3U8Player`クラスのデストラクタ。  

### start()
#### 説明:
再生を開始する。  
コンストラクタ引数の`isAutoStart`を`true`にしている場合は呼び出す必要なし。  

#### 構文:
`bool start();`  

#### 返り値:
すでに再生開始されている場合は`false`を、それ以外の場合は`true`を返す。  

#### 使用例:  
```C++
#include <M3U8Player.h>

M3U8Player *player;
String stationUrl = "http://xxxx/xxxx/playlist.m3u8";
player = new M3U8Player(stationUrl);
player->start();
```

### setVolume()
#### 説明:
音量を変更する。  

#### 構文:
`void setVolume(const float &newVolume);`  

#### 引数:
| 引数 | 型 | 説明 |
| :---: | :---: | :---: |
| newVolume | float | 音量(0-100) |

#### 使用例:
```C++
#include <M3U8Player.h>

M3U8Player *player;
String stationUrl = "http://xxxx/xxxx/playlist.m3u8";
float defaultVolume = 50.0;
player = new M3U8Player(stationUrl, defaultvolume, true);

if(M5.BtnA.wasPressed){
  player->setVolume(70);
}
```

### getVolume()
#### 説明:
現在の音量値を返す。  

#### 構文:
`float getVolume();`  

#### 返り値:
現在の音量(0-100)  

#### 使用例:
```C++
#include <M3U8Player.h>

M3U8Player *player;

float volume = player->getVolume();
Serial.println(volume);
```

### changeStationURL()
#### 説明:
再生するHLSプレイリストを変更する。  

#### 構文:
`bool changeStationURL(const String &url);`

#### 引数:
| 引数 | 型 | 説明 |
| :---: | :---: | :---: |
| url | String | HLSプレイリストのURL |

#### 返り値:
`url`の先頭がhttpもしくはhttpsでない場合はURLを変更せずに`false`を、
それ以外の場合はURLを変更して`true`を返す。

#### 使用例:
```C++
#include <M3U8Player.h>

M3U8Player *player;
String initialUrl = "http://xxxx/xxxx/playlist.m3u8";
player = new M3U8Player(initialUrl, 10.0, true);

if(M5.BtnA.wasPressed){
  String newUrl = "http://yyyy/yyyy/playlist.m3u8";
  player->changeStationURL(newUrl);
}
```

### getStationURL()
#### 説明:
現在再生しているHLSプレイリストのURLを返す。  

#### 構文:
`String getStationURL();`  

#### 返り値:
現在再生しているHLSプレイリストのURL  

#### 使用例:
```C++
#include <M3U8Player.h>

M3U8Player *player;

String currentUrl = player->getStationURL();
Serial.println(volume);
```

### getState()
#### 説明:
現在の状態を返す。  

#### 構文:
`M3U8Player_State getState();`

#### 返り値:
`enam class M3U8Player_State`で表現された状態値。  
取り得る値は以下の通り。  

| 値 | 説明 |
| :---: | :---: |
| SETUP | セットアップ中(コンストラクタの処理が未完了) |
| STANDBY | セットアップ完了 |
| PLAYING | 再生中 |
| CHANNEL_CHANGING | HLSプレイリストのURLを変更中 |
| OTHERS| それ以外 |

#### 使用例:

```C++
#include <M3U8Player.h>

M3U8Player *player;

M3U8Player_State state = player->getState();

switch(state){
  case M3U8Player_State::SETUP:
    Serial.println("setup...");
    break;
  case M3U8Player_State::STANDBY:
    Serial.println("standby now");
    break;
  case M3U8Player_State::PLAYING:
    Serial.println("now playing");
    break;
  case M3U8Player_State::CHANNEL_CHANGING:
    Serial.println("ch changing");
    break;
  default:
    Serial.println("something error");
}
```

## (任意)チャンネル切り替えの高速化  
この項目に記述されている作業は必須ではない。  
この変更により、チャンネル切り替えの処理が高速化される。  

arduino-esp32のHTTPClient.cppにある下記コード  
```C++
void HTTPClient::disconnect(bool preserveClient)
{
    if(connected()) {
        if(_client->available() > 0) {
            log_d("still data in buffer (%d), clean up.\n", _client->available());
            while(_client->available() > 0) {  // ここから
                _client->read();               // 削除する
            }                                  // ここまで
        }
  //後略
}
```
を次のように書き換える
```C++
void HTTPClient::disconnect(bool preserveClient)
{
    if(connected()) {
        if(_client->available() > 0) {
            log_d("still data in buffer (%d), clean up.\n", _client->available());
            _client->flush();  // このコードを追加する
        }
  //後略
}
```

### 参考URL  
1. https://github.com/espressif/arduino-esp32/issues/828
2. https://github.com/h3ndrik/arduino-esp32/commit/1ca53494d2f983bbf60d2b9a2333ccad177e6678
