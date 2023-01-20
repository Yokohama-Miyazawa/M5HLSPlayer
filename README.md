[日本語版README](./README-jp.md)
# M5HLSPlayer  
Web radio player for M5Stack and M5StickC in .m3u8 format  
Support for .aac and .ts  

## Contents
1. [Prerequisite](./README.md#prerequisite)
2. [Usage](./README.md#usage)
3. [Description of the Member Functions](./README.md#description-of-the-member-functions)
4. [(Option) Acceleration of the channel change](./README.md#option-acceleration-of-the-channel-change)

## Prerequisite  
### Each M5 Controller Library  
[M5Stack](https://github.com/m5stack/M5Stack)  
[M5StickC](https://github.com/m5stack/M5StickC)  
[M5Core2](https://github.com/m5stack/M5Core2)  

### All M5 Controllers Common  
[ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio)  
[uzlib](https://github.com/pfalcon/uzlib)  

## Usage  
### PlatformIO  
Create a project and place this repository and the prerequisite library in the lib directory.  
Call this library from main.cpp (see examples for details).  

### Arduino IDE  
Create a sketch and place this repository and the prerequisite library in the `Arduino/libraries/` directory.  
Call this library from .ino file (see examples for details).  

## Description of the Member Functions
### M3U8Player()
#### Description:
Constructors of `M3U8Player` class  
If `isAutoStart` is set to `true`, then the instance starts to play automatically.  
If `isAutoStart` is set to `false`, then you need to call `start()` explicitly.  

#### Syntax:
`M3U8Player(String url);`  
`M3U8Player(String url, const float &startVolume);`  
`M3U8Player(String url, const float &startVolume, const bool &isAutoStart);`  
`M3U8Player(String url, const float &startVolume, const bool &isAutoStart, const uint32_t &bufferSize);`  
`M3U8Player(String url, const float &startVolume, const bool &isAutoStart, const uint32_t &bufferSize, const bool &isCore2);`  

#### Function argument:
| argument | type | Description |
| :---: | :---: | :---: |
| url | String | URL of HLS playlist |
| startVolume | float | volume of initial setting (0-100, default 5) |
| isAutoStart | bool | start playback automatically or not (default `false`) |
| bufferSize | uint32_t | byte size of a buffer for audio data (default 4096) |
| isCore2 | bool | M5Stack Core2 or not (default `false`) |

#### Example of use:
example 1  
```C++
#include <M3U8Player.h>

M3U8Player *player;
String stationUrl = "http://xxxx/xxxx/playlist.m3u8";
float defaultVolume = 10.0;
player = new M3U8Player(stationUrl, defaultVolume, true);
```
example 2  
```C++
#include <M3U8Player.h>

M3U8Player *player;
String stationUrl = "http://xxxx/xxxx/playlist.m3u8";
player = new M3U8Player(stationUrl);
player->start();
```

### ~M3U8Player()  
#### Description:
The destructor of `M3U8Player` class  

### start()
#### Description:
Start playback  
If an argument `isAutoStart` of the constructor is set to `true`, then you do not need to call this member function.    

#### Syntax:
`bool start();`  

#### Function return value:
If the instance have already started playback, then returns `false`, otherwise `true`.  

#### Example of use:  
```C++
#include <M3U8Player.h>

M3U8Player *player;
String stationUrl = "http://xxxx/xxxx/playlist.m3u8";
player = new M3U8Player(stationUrl);
player->start();
```

### setVolume()
#### Description:
Change the volume  

#### Syntax:
`void setVolume(const float &newVolume);`  

#### Function argument:
| argument | type | Description |
| :---: | :---: | :---: |
| newVolume | float | volume (0.0-100.0) |

#### Example of use:
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
#### Description:
Get the current volume value  

#### Syntax:
`float getVolume();`  

#### Function return value:
Current volume (0-100)  

#### Example of use:
```C++
#include <M3U8Player.h>

M3U8Player *player;

float volume = player->getVolume();
Serial.println(volume);
```

### changeStationURL()
#### Description:
Change HLS playlist for playback    

#### Syntax:
`bool changeStationURL(const String &url);`

#### Function argument:
| argument | type | Description |
| :---: | :---: | :---: |
| url | String | URL of HLS playlist |

#### Function return value:
If `url` doesn't start from http or https, then it returns `false` and doesn't change the URL, otherwise returns `true` and changes the URL.  

#### Example of use:
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
#### Description:
Get the URL of the HLS playlist that is currently being played back  

#### Syntax:
`String getStationURL();`  

#### Function return value:
URL of the HLS playlist that is currently being played back  

#### Example of use:
```C++
#include <M3U8Player.h>

M3U8Player *player;

String currentUrl = player->getStationURL();
Serial.println(volume);
```

### getState()
#### Description:
Get current state  

#### Syntax:
`M3U8Player_State getState();`

#### Function return value:
A state value which is represented by `enam class M3U8Player_State`  
Values and desrciptions are below  

| value | Description |
| :---: | :---: |
| SETUP | setup in progress |
| STANDBY | setup completed and able to start |
| PLAYING | currently playing back |
| CHANNEL_CHANGING | changing the url of HLS playlist |
| OTHERS| the others |

#### Example of use

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

## (Option) Acceleration of the channel change  
This is optional, but not required.  
This will speed up the execution of channel changes.  

In `HTTPClient.cpp` of `arduino-esp32`, rewrite the codes below  
```C++
void HTTPClient::disconnect(bool preserveClient)
{
    if(connected()) {
        if(_client->available() > 0) {
            log_d("still data in buffer (%d), clean up.\n", _client->available());
            while(_client->available() > 0) {  // from
                _client->read();               // remove
            }                                  // to
        }
  // the rest omitted
}
```
with  
```C++
void HTTPClient::disconnect(bool preserveClient)
{
    if(connected()) {
        if(_client->available() > 0) {
            log_d("still data in buffer (%d), clean up.\n", _client->available());
            _client->flush();  // Add this line
        }
  // the rest omitted
}
```

### References  
1. https://github.com/espressif/arduino-esp32/issues/828
2. https://github.com/h3ndrik/arduino-esp32/commit/1ca53494d2f983bbf60d2b9a2333ccad177e6678
