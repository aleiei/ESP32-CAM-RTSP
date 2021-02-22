/*
   MIT License

   Copyright (c) 2021 Alessandro Orlando

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute,
   sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
   TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVEN
   SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
   OR OTHER DEALINGS IN THE SOFTWARE.
   
   Description:
   Simple I2S MEMS microphone Audio Streaming via UDP transmitter
   Needs a UDP listener like netcat on port 16500 on listener PC
   Needs a SoX with mp3 handler for Recorder
   Under Linux for listener:
   netcat -u -p 16500 -l | play -t raw -r 16000 -b 16 -c 2 -e signed-integer -
   Under Linux for recorder (give for file.mp3 the name you prefer) : 
   netcat -u -p 16500 -l | rec -t raw -r 16000 -b 16 -c 2 -e signed-integer - file.mp3
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>

// Micro-RTSP Library
#include "SimStreamer.h"
#include "OV2640Streamer.h"
#include "CRtspSession.h"

//AI Thinker ESP32 CAM PINS
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

// WIFI SETTINGS.
#define WIFI_SSID  "Your_SSID"
#define WIFI_PASSWD "YOUR_PASS"


OV2640 cam;
CStreamer *streamer;

WiFiServer rtspServer(554); // RTSP default transport layer port.

// STATIC IP.
IPAddress local_IP(192, 168, 1, 10);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

void setupWiFi() {
  IPAddress ip;
  
  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("Failed to configure IP");
  }
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
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

  if(psramFound()){
    Serial.println("psram found");
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10; //10-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    Serial.println("psram not found");
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  cam.init(config);
}

void setupStreaming() {
    rtspServer.begin();  
    //streamer = new SimStreamer(true);             // our streamer for UDP/TCP based RTP transport
    streamer = new OV2640Streamer(cam);             // our streamer for UDP/TCP based RTP transport
}

void handleStreaming() {
    uint32_t msecPerFrame = 100;
    static uint32_t lastimage = millis();

    // If we have an active client connection, just service that until gone
    streamer->handleRequests(0); // we don't use a timeout here,
    // instead we send only if we have new enough frames
    uint32_t now = millis();
    if(streamer->anySessions()) {
        if(now > lastimage + msecPerFrame || now < lastimage) { // handle clock rollover
            streamer->streamImage(now);
            lastimage = now;

            // check if we are overrunning our max frame rate
            now = millis();
            if(now > lastimage + msecPerFrame) {
                printf("warning exceeding max frame rate of %d ms\n", now - lastimage);
            }
        }
    }
    
    WiFiClient rtspClient = rtspServer.accept();
    if(rtspClient) {
        Serial.print("client: ");
        Serial.print(rtspClient.remoteIP());
        Serial.println();
        streamer->addSession(rtspClient);
    }
}

void setup()
{
  Serial.begin(115200);
  while (!Serial);            //wait for serial connection.
  setupCamera();
  setupWiFi();   
  setupStreaming();
}

void loop()
{ 
   handleStreaming();
}
