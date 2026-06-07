// USB Host CDC-ACM front-end: enumerates the flight controller's Virtual COM
// Port and pumps bytes to/from the bridge.
#pragma once

#include <stdbool.h>
#include <stdint.h>

// Start the USB host stack and the CDC connect/disconnect handling. Spawns the
// USB library event task and the net->USB TX task. Returns once the host stack
// is installed (device attach happens asynchronously thereafter).
void usb_cdc_host_start(void);

// True while a FC VCP is open and ready to carry traffic.
bool usb_cdc_host_is_connected(void);

// Connection status plus the VID/PID of the open VCP (0 if not connected).
// vid/pid may be NULL. Returns the connected state.
bool usb_cdc_host_status(uint16_t *vid, uint16_t *pid);
