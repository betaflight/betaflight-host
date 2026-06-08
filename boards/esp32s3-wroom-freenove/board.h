#pragma once
// Freenove ESP32-S3-WROOM (N8R8), dual USB-C. The native USB pins
// (D- GPIO19 / D+ GPIO20) drive the USB-host bridge; a separate CH343 UART
// bridge handles flashing/console, so the console survives USB-host mode.
#define BOARD_NAME           "esp32s3-wroom-freenove"

// v1.1: plain LED on GPIO2 reflects WiFi state (blink cadence).
#define BOARD_WIFI_LED_GPIO  2
// #define BOARD_WIFI_LED_ACTIVE_LOW   // uncomment if the LED is wired active-low

// Onboard WS2812 NeoPixel — indicates FC/OTG-USB comms.
#define BOARD_RGB_LED_GPIO   48
