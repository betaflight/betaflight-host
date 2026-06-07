// betaflight-host: ESP32-S3 USB-host-to-WiFi bridge.
//
// Acts as USB host to a Betaflight flight controller's Virtual COM Port (VCP)
// and exposes that serial stream over TCP, so Betaflight Configurator (TCP
// transport, default port 5761) can connect wirelessly from a phone or laptop.
//
//   [FC USB VCP] <--USB host--> [ESP32-S3] <--WiFi/TCP:5761--> [Configurator]
//
// Data flow is a transparent byte bridge; no MSP parsing happens here.

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "bridge.h"
#include "wifi.h"
#include "tcp_server.h"
#include "usb_cdc_host.h"
#include "http_status.h"

static const char *TAG = "main";

void app_main(void)
{
    // NVS holds WiFi station credentials (and is required by the WiFi stack).
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    bridge_init();
    wifi_start();
    http_status_start();
    tcp_server_start();
    usb_cdc_host_start();

    ESP_LOGI(TAG, "betaflight-host ready");
}
