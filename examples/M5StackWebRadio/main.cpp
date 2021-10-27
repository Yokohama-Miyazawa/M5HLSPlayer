#include <M5Stack.h>
#include <WiFi.h>
#include "M3U8Player.h"

// Write your SSID and Password here
#define SSID "********"
#define PASSWD "********"

M3U8Player *player;

void setup() {
  M5.begin();
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWD);
  M5.Lcd.printf("WiFi Connecting...\n");
  log_i("WiFi Connecting...");
  while (!WiFi.isConnected()){ delay(10); }
  M5.Lcd.printf("WiFi Connected\n");
  log_i("WiFi Connected");

  String stationUrl = "http://xxxxx/xxxxx/xxxxx.m3u8";  // Write URL of m3u8 here
  player = new M3U8Player(stationUrl);
  delay(3000);
  log_i("setup completed");
  M5.Lcd.printf("setup completed\n");
  M5.Lcd.setTextSize(2);
}

void loop() {
  M5.update();
  if (M5.BtnB.isPressed()){
    player->playAAC();
  }
  delay(10);
}
