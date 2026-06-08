#pragma once
// ESP32-S3-ZERO (Waveshare). One USB-C, wired to the native USB pins
// (D- GPIO19 / D+ GPIO20), shared between flashing/console and USB-host — so
// the serial console drops out once USB-host mode engages.
#define BOARD_NAME          "esp32s3-zero"

// Onboard WS2812 NeoPixel — indicates FC/OTG-USB comms. (No separate WiFi LED.)
#define BOARD_RGB_LED_GPIO  21
