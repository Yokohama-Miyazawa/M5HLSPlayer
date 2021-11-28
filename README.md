[日本語版README](./README-jp.md)
# M5M3U8Player  
Web radio player in .m3u8 format  
Only .aac, currently not .ts  

## Prerequisite 　
[ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio)  
In addition, you need to add a code like the following.  

### AudioFileSourceHTTPStream.cpp  
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
Create a project and place this repository in the lib directory.  
Call this library from main.cpp (see examples for details).  
