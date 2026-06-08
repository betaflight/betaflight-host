// Byte bridge between the USB CDC (FC side) and the TCP client (Configurator
// side). Two FreeRTOS stream buffers decouple the producer/consumer contexts so
// neither the USB driver callback nor the socket recv loop can stall the other.
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Initialise the bridge stream buffers. Call once at startup before any of the
// push/pop helpers below.
void bridge_init(void);

// FC -> Configurator. Called from the USB CDC RX callback context. Non-blocking;
// returns the number of bytes actually queued (may be < len if the buffer is
// full, which means the TCP side is not draining fast enough).
size_t bridge_usb_to_net_push(const uint8_t *data, size_t len);

// FC -> Configurator drain. Called by the TCP TX task. Blocks up to
// timeout_ms for at least one byte. Returns bytes written into out.
size_t bridge_usb_to_net_pop(uint8_t *out, size_t max_len, uint32_t timeout_ms);

// Configurator -> FC. Called from the TCP RX task. Non-blocking; returns bytes
// queued.
size_t bridge_net_to_usb_push(const uint8_t *data, size_t len);

// Configurator -> FC drain. Called by the USB TX task. Blocks up to timeout_ms.
size_t bridge_net_to_usb_pop(uint8_t *out, size_t max_len, uint32_t timeout_ms);

// Discard any buffered data in both directions. Called when a client connects or
// disconnects so a new session starts clean (avoids replaying stale MSP bytes).
void bridge_reset(void);

// Monotonic count of bytes moved to/from the FC (either direction). Sample it
// to drive an activity indicator: a change since the last sample means traffic.
uint32_t bridge_fc_activity(void);
