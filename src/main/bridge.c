#include "bridge.h"

#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"

// Sized for MSP traffic: frames are small but dumps (e.g. `diff all`, blackbox
// config) can burst several KB. 8 KB each direction gives comfortable slack.
#define BRIDGE_BUF_SIZE   (8 * 1024)
// Wake a draining task as soon as a single byte is available — MSP is latency
// sensitive and we do our own batching on the read side.
#define BRIDGE_TRIGGER    1

static StreamBufferHandle_t s_usb_to_net;  // FC  -> Configurator
static StreamBufferHandle_t s_net_to_usb;  // Configurator -> FC

void bridge_init(void)
{
    s_usb_to_net = xStreamBufferCreate(BRIDGE_BUF_SIZE, BRIDGE_TRIGGER);
    s_net_to_usb = xStreamBufferCreate(BRIDGE_BUF_SIZE, BRIDGE_TRIGGER);
    configASSERT(s_usb_to_net);
    configASSERT(s_net_to_usb);
}

size_t bridge_usb_to_net_push(const uint8_t *data, size_t len)
{
    // Zero timeout: never block the USB driver callback. Dropped bytes here are
    // visible as a stalled/overflowing TCP client rather than a wedged FC link.
    return xStreamBufferSend(s_usb_to_net, data, len, 0);
}

size_t bridge_usb_to_net_pop(uint8_t *out, size_t max_len, uint32_t timeout_ms)
{
    return xStreamBufferReceive(s_usb_to_net, out, max_len, pdMS_TO_TICKS(timeout_ms));
}

size_t bridge_net_to_usb_push(const uint8_t *data, size_t len)
{
    return xStreamBufferSend(s_net_to_usb, data, len, 0);
}

size_t bridge_net_to_usb_pop(uint8_t *out, size_t max_len, uint32_t timeout_ms)
{
    return xStreamBufferReceive(s_net_to_usb, out, max_len, pdMS_TO_TICKS(timeout_ms));
}

void bridge_reset(void)
{
    if (s_usb_to_net) {
        xStreamBufferReset(s_usb_to_net);
    }
    if (s_net_to_usb) {
        xStreamBufferReset(s_net_to_usb);
    }
}
