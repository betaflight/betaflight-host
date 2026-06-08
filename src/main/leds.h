// Status LEDs, all optional and board-defined (see board.h):
//   BOARD_WIFI_LED_GPIO  - plain GPIO LED, blink cadence reflects WiFi state
//   BOARD_RGB_LED_GPIO   - WS2812 NeoPixel, colour reflects FC/OTG-USB comms
// A board that defines neither gets a no-op. Call once after the bridge, WiFi,
// TCP and USB-host subsystems are started.
#pragma once

void leds_start(void);
