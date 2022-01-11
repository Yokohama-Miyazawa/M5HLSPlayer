#include <M5Stack.h>
#include <WiFi.h>
#include <M3U8Player.h>

// Write your SSID and Password here
#define SSID "********"
#define PASSWD "********"

M3U8Player *player;
float currentVolume;
float maxVolume = 100.0;
float volumeStep = 5.0;

void showVolume(const float &volume) {
  M5.Lcd.setCursor(10, 20);
  M5.Lcd.printf("Volume: %03.0f", volume);
  return;
}

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
  float initialVolume = 5.0;
  player = new M3U8Player(stationUrl, initialVolume);
  delay(3000);
  log_i("setup completed");
  M5.Lcd.printf("setup completed\n");
  player->start();
  M5.Lcd.setTextSize(3);
  M5.Lcd.clear(BLACK);
  showVolume(initialVolume);
}

void loop() {
  M5.update();
  if (M5.BtnA.wasPressed()){
    currentVolume = player->getVolume();
    if(currentVolume >= volumeStep){
      player->setVolume(currentVolume - volumeStep);
      showVolume(currentVolume - volumeStep);
    }
  }
  if (M5.BtnC.wasPressed()){
    currentVolume = player->getVolume();
    if(currentVolume <= maxVolume - volumeStep){
      player->setVolume(currentVolume + volumeStep);
      showVolume(currentVolume + volumeStep);
    }
  }
  delay(10);
}
