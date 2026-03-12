# ESP32-CAM-RTSP

RTSP video streaming server for the AI Thinker ESP32-CAM module. Once flashed and connected to a local network, the device streams live MJPEG video over RTSP and can be viewed or recorded with any compatible client, such as VLC.

## How to connect

Open the network stream at the following address:

```
rtsp://192.168.1.10:554/mjpeg/1
```

A second stream URL is also accepted: `rtsp://192.168.1.10:554/mjpeg/2`. Both map to the same physical camera; the distinction exists in the session layer but the image source is identical.

## How it works

At boot the firmware initialises the OV2640 camera using the AI Thinker pin mapping, with XCLK running at 20 MHz, JPEG pixel format, XGA resolution (1024×768), and a quality value of 10 (lower means higher quality). If the camera fails to initialise the device halts immediately, since streaming cannot proceed without it.

After the camera is ready, the firmware attempts to configure a static IP address (192.168.1.10) and connects to the Wi-Fi network defined in the source. If the static IP negotiation fails it falls back to DHCP. If no connection is established within 20 seconds, a warning is printed to the serial console and the device continues running, though it will never accept any client.

Once the network is up, a TCP server starts listening on port 554, the standard RTSP port. The main loop then runs continuously: it services any pending RTSP control messages from connected clients, and when at least one session is active it pushes a new JPEG frame approximately every 100 ms, targeting around 10 frames per second. Whenever encoding or transmission takes longer than that interval, the overrun is reported on the serial console. New clients are accepted on each iteration of the loop and their sessions are registered with the streamer automatically.

Each frame is captured via `CAM32Streamer`, which calls `cam.run()` to queue the next acquisition, retrieves the JPEG buffer, forwards it to the RTP layer via `streamFrame()`, and then releases the buffer. Transport can be either UDP (default) or TCP interleaved, negotiated during SETUP.

## RTSP command reference

The device implements a subset of RTSP/1.0. All commands are sent by the client (e.g. VLC) automatically as part of normal session establishment; manual use is only needed when integrating a custom player or debugging.

**OPTIONS** — asks the server which commands it supports. The device responds advertising `DESCRIBE`, `SETUP`, `TEARDOWN`, `PLAY`, and `PAUSE`.

**DESCRIBE** — requests the session description (SDP) for a given URL. The device validates the path (`mjpeg/1` or `mjpeg/2`) and returns an SDP document declaring a video stream using RTP payload type 26 (MJPEG). An unknown path returns `404 Stream Not Found`.

**SETUP** — establishes the transport for a session. The client specifies its RTP and RTCP port numbers; the device records them and allocates the corresponding UDP sockets, or switches to TCP interleaved transport if the client requests `RTP/AVP/TCP`. The response includes the assigned session ID and the server-side RTP/RTCP ports.

**PLAY** — instructs the device to start sending RTP packets. From this point on the main loop begins delivering JPEG frames to that session. The response includes the session ID and a `Range: npt=0.000-` header indicating an open-ended, live stream.

**TEARDOWN** — closes the session. The device marks the session as stopped and releases the UDP transport. The TCP connection is then closed. This command is recognised by the parser but does not generate a dedicated response beyond closing the socket.

Any message whose first character is not `O`, `D`, `S`, `P`, or `T` is silently discarded. If the client closes the TCP connection without sending TEARDOWN, the device detects the closed socket on the next read and terminates the session automatically.

## Configuration

Before flashing, edit the following lines in `src/main.cpp`:

```cpp
#define WIFI_SSID   "Your_SSID"
#define WIFI_PASSWD "Your_PASS"
```

The static IP, gateway, and subnet can be adjusted in the `setupWiFi()` function. Frame size and JPEG quality can be changed in `setupCamera()`.

## Dependencies

This project relies on an RTSP streaming library that provides `CRtspSession`, `CAM32Streamer`, and `CStreamer`. It is intended to be built with PlatformIO targeting the ESP32 Arduino framework.
