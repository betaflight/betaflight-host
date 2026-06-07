#include "wifi.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "wifi";

#define NVS_NAMESPACE   "wifi"
#define STA_MAX_RETRY   5

static int s_sta_retries;

static void on_wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT) {
        switch (id) {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            if (s_sta_retries++ < STA_MAX_RETRY) {
                ESP_LOGW(TAG, "STA disconnected, retry %d", s_sta_retries);
                esp_wifi_connect();
            } else {
                ESP_LOGW(TAG, "STA giving up; AP remains available");
            }
            break;
        case WIFI_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG, "client joined AP");
            break;
        default:
            break;
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
        s_sta_retries = 0;
        ESP_LOGI(TAG, "STA got IP " IPSTR " (port 5761)", IP2STR(&event->ip_info.ip));
    }
}

// Returns true and fills ssid/pass (NUL-terminated) if station creds exist.
static bool load_sta_creds(char *ssid, size_t ssid_len, char *pass, size_t pass_len)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) {
        return false;
    }
    bool ok = nvs_get_str(h, "ssid", ssid, &ssid_len) == ESP_OK &&
              nvs_get_str(h, "pass", pass, &pass_len) == ESP_OK &&
              ssid[0] != '\0';
    nvs_close(h);
    return ok;
}

static void start_ap(void)
{
    esp_netif_create_default_wifi_ap();

    wifi_config_t ap = {
        .ap = {
            .channel = WIFI_AP_CHANNEL,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strlcpy((char *)ap.ap.ssid, WIFI_AP_SSID, sizeof(ap.ap.ssid));
    ap.ap.ssid_len = strlen(WIFI_AP_SSID);
    strlcpy((char *)ap.ap.password, WIFI_AP_PASS, sizeof(ap.ap.password));
    if (strlen(WIFI_AP_PASS) == 0) {
        ap.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap));
    ESP_LOGI(TAG, "SoftAP '%s' up at 192.168.4.1", WIFI_AP_SSID);
}

static bool start_sta_if_configured(void)
{
    char ssid[33] = {0};
    char pass[65] = {0};
    if (!load_sta_creds(ssid, sizeof(ssid), pass, sizeof(pass))) {
        return false;
    }

    esp_netif_create_default_wifi_sta();

    wifi_config_t sta = {0};
    strlcpy((char *)sta.sta.ssid, ssid, sizeof(sta.sta.ssid));
    strlcpy((char *)sta.sta.password, pass, sizeof(sta.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta));
    ESP_LOGI(TAG, "joining station network '%s'", ssid);
    return true;
}

void wifi_start(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_event, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &on_wifi_event, NULL, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    bool sta = start_sta_if_configured();
    start_ap();
    ESP_ERROR_CHECK(esp_wifi_set_mode(sta ? WIFI_MODE_APSTA : WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_set_station(const char *ssid, const char *pass)
{
    nvs_handle_t h;
    ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h));
    if (ssid && ssid[0]) {
        ESP_ERROR_CHECK(nvs_set_str(h, "ssid", ssid));
        ESP_ERROR_CHECK(nvs_set_str(h, "pass", pass ? pass : ""));
    } else {
        nvs_erase_key(h, "ssid");
        nvs_erase_key(h, "pass");
    }
    ESP_ERROR_CHECK(nvs_commit(h));
    nvs_close(h);
    ESP_LOGI(TAG, "station creds updated; reboot to apply");
}
