/*
   Copyright (C) 2021 Alessandro Orlando

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published
   by the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include "SimStreamer.h"
#include "CAM32Streamer.h"
#include "CRtspSession.h"

#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
#define WIFI_SSID  "Your_SSID"
#define WIFI_PASSWD "Your_PASS"


CAM32 cam;
CStreamer *streamer = nullptr;
WiFiServer rtspServer(554);
IPAddress local_IP(192, 168, 1, 10);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

void setupWiFi() {
  if (!WiFi.config(local_IP, gateway, subnet)) {
      Serial.println("Warning: failed to configure static IP, using DHCP");
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWD);

  unsigned long start = millis();
  const unsigned long timeout = 20000;
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout) {
      delay(500);
      Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
      Serial.println("\nERROR: could not connect to WiFi");
      return;
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; //PIXFORMAT_YUV422 PIXFORMAT_GRAYSCALE PIXFORMAT_RGB565 PIXFORMAT_JPEG
 
  /*
  FRAMESIZE_UXGA (1600 x 1200)
  FRAMESIZE_QVGA (320 x 240)
  FRAMESIZE_CIF (352 x 288)
  FRAMESIZE_VGA (640 x 480)
  FRAMESIZE_SVGA (800 x 600)
  FRAMESIZE_XGA (1024 x 768)
  FRAMESIZE_SXGA (1280 x 1024)
  */

  config.frame_size = FRAMESIZE_XGA;
  config.jpeg_quality = 10; //10-63 lower number means higher quality
  config.fb_count = 1;

  esp_err_t err = cam.init(config);
  if (err != ESP_OK) {
      Serial.printf("Camera initialization failed (err=%d)\n", err);
      while (true) {
          delay(1000);
      }
  }
}

void setupStreaming() {
    rtspServer.begin();  
    streamer = new CAM32Streamer(cam);
}

void handleStreaming() {
    const uint32_t msecPerFrame = 100;
    static uint32_t lastimage = millis();

    if (!streamer)
        return;

    
    streamer->handleRequests(0);

    uint32_t now = millis();
    if (streamer->anySessions()) {
        if ((now - lastimage) >= msecPerFrame) {
            streamer->streamImage(now);
            lastimage = now;

            uint32_t later = millis();
            if ((later - now) > msecPerFrame) {
                Serial.printf("warning: exceeding max frame rate by %u ms\n", later - now);
            }
        }
    }

    WiFiClient rtspClient = rtspServer.accept();
    if (rtspClient) {
        Serial.print("client: ");
        Serial.println(rtspClient.remoteIP());
        streamer->addSession(rtspClient);
    }
}

void setup()
{
  Serial.begin(115200);
  delay(100);
  setupCamera();
  setupWiFi();
  setupStreaming();
}

void loop()
{ 
   handleStreaming();
}
