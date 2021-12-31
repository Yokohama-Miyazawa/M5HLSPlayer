[日本語版README](./README-jp.md)
# M5HLSPlayer  
Web radio player for M5Stack and M5StickC in .m3u8 format  
Support for .aac and .ts  

## Prerequisite 　
### [ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio)  

In addition, you need to add a code like the following.  

#### AudioFileSourceHTTPStream.cpp  
```
bool AudioFileSourceHTTPStream::close()
{
  http.end();
  client.stop();  // Add this line
  return true;
}
```

## Usage  
### PlatformIO  
Create a project and place this repository and the prerequisite library in the lib directory.  
Call this library from main.cpp (see examples for details).  

### Arduino IDE  
Create a sketch and place this repository and the prerequisite library in the `Arduino/libraries/` directory.  
Call this library from .ino file (see examples for details).  

