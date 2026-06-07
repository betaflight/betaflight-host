#include "http_status.h"
#include "usb_cdc_host.h"
#include "tcp_server.h"

#include <stdio.h>

#include "esp_http_server.h"
#include "esp_log.h"

static const char *TAG = "http";

// Server-rendered page; a <meta refresh> keeps it current without any JS.
static const char *PAGE_FMT =
    "<!DOCTYPE html><html><head>"
    "<meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<meta http-equiv=\"refresh\" content=\"2\">"
    "<title>betaflight-host</title>"
    "<style>"
    "body{font-family:system-ui,sans-serif;background:#1b1b1b;color:#eee;"
    "margin:0;padding:2rem;}"
    "h1{font-size:1.3rem;color:#f80;margin:0 0 1.2rem;}"
    "table{border-collapse:collapse;font-size:1.05rem;}"
    "td{padding:.45rem .9rem;border-bottom:1px solid #333;}"
    ".k{color:#aaa;}"
    ".up{color:#4caf50;font-weight:600;}"
    ".down{color:#888;}"
    "code{color:#8cf;}"
    "</style></head><body>"
    "<h1>betaflight-host</h1>"
    "<table>"
    "<tr><td class=\"k\">FC (USB VCP)</td><td>%s</td></tr>"
    "<tr><td class=\"k\">Configurator (TCP %d)</td><td>%s</td></tr>"
    "</table></body></html>";

static esp_err_t root_get(httpd_req_t *req)
{
    uint16_t vid = 0, pid = 0;
    bool usb = usb_cdc_host_status(&vid, &pid);
    bool tcp = tcp_server_client_connected();

    char usb_cell[64];
    if (usb) {
        snprintf(usb_cell, sizeof(usb_cell),
                 "<span class=\"up\">connected</span> <code>%04x:%04x</code>", vid, pid);
    } else {
        snprintf(usb_cell, sizeof(usb_cell), "<span class=\"down\">waiting…</span>");
    }

    const char *tcp_cell = tcp ? "<span class=\"up\">connected</span>"
                               : "<span class=\"down\">none</span>";

    char body[1024];
    int n = snprintf(body, sizeof(body), PAGE_FMT, usb_cell, TCP_SERVER_PORT, tcp_cell);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, body, n);
    return ESP_OK;
}

void http_status_start(void)
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port = 80;
    cfg.lru_purge_enable = true;   // free idle sockets so the page stays reachable

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "failed to start HTTP server");
        return;
    }

    const httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get,
    };
    httpd_register_uri_handler(server, &root);
    ESP_LOGI(TAG, "status page on http://<ip>/ (port 80)");
}
