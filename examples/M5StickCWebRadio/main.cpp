#include <M5StickC.h>
#include <WiFi.h>
#include <M3U8Player.h>
#include <SD.h>

// Write your SSID and Password here
#define SSID "********"
#define PASSWD "********"

M3U8Player *player;
bool isMuted = false;

void print(const char *text) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("%s", text);
}

void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextSize(2);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWD);
  print("WiFi Connecting...");
  log_i("WiFi Connecting...");
  while (!WiFi.isConnected()){ delay(10); }
  print("WiFi Connected");
  log_i("WiFi Connected");

  String stationUrl = "http://xxxxx/xxxxx/xxxxx.m3u8"; // Write URL of m3u8 here
  player = new M3U8Player(stationUrl, 100.0);
  delay(3000);
  print("setup completed");
  log_i("setup completed");
  player->start();
  print("now playing");
}

void loop() {
  M5.update();
  if (M5.BtnB.isPressed()){
    float nextVolume = isMuted ? 100.0 : 0.0;
    player->setVolume(nextVolume);
    isMuted = !isMuted;
    delay(100);
  }
  delay(10);
}
