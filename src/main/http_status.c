#include "http_status.h"
#include "usb_cdc_host.h"
#include "tcp_server.h"
#include "wifi.h"
#include "ota.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "esp_http_server.h"
#include "esp_log.h"

static const char *TAG = "http";

#define MAX_SCAN_APS  20

// Single-page UI. Status fields and the network list are filled by JS polling
// /status and /scan, so the page itself is static and cacheable.
static const char PAGE[] =
    "<!DOCTYPE html><html><head>"
    "<meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>betaflight-host</title>"
    "<style>"
    "body{font-family:system-ui,sans-serif;background:#1b1b1b;color:#eee;"
    "margin:0;padding:1.5rem;max-width:30rem;}"
    "h1{font-size:1.3rem;color:#f80;margin:0 0 1rem;}"
    "h2{font-size:1rem;color:#f80;margin:1.6rem 0 .6rem;}"
    "table{border-collapse:collapse;font-size:1.02rem;width:100%;}"
    "td{padding:.4rem .6rem;border-bottom:1px solid #333;}"
    ".k{color:#aaa;width:42%;}"
    ".up{color:#4caf50;font-weight:600;}"
    ".down{color:#888;}"
    ".warn{color:#f80;}"
    "code{color:#8cf;}"
    "label{display:block;margin:.6rem 0 .2rem;color:#aaa;font-size:.9rem;}"
    "select,input{width:100%;box-sizing:border-box;padding:.5rem;background:#262626;"
    "color:#eee;border:1px solid #444;border-radius:6px;font-size:1rem;}"
    "button{margin-top:.9rem;padding:.55rem 1rem;background:#f80;color:#111;border:0;"
    "border-radius:6px;font-size:1rem;font-weight:600;cursor:pointer;}"
    "button.sec{background:#333;color:#eee;margin-left:.5rem;}"
    "#msg{margin-top:.7rem;min-height:1.2rem;}"
    "</style></head><body>"
    "<h1>betaflight-host</h1>"
    "<table>"
    "<tr><td class=\"k\">FC (USB VCP)</td><td id=\"usb\">…</td></tr>"
    "<tr><td class=\"k\">Configurator (TCP)</td><td id=\"tcp\">…</td></tr>"
    "<tr><td class=\"k\">WiFi network</td><td id=\"sta\">…</td></tr>"
    "<tr><td class=\"k\">IP address</td><td id=\"ip\">…</td></tr>"
    "<tr><td class=\"k\">Gateway</td><td id=\"gw\">…</td></tr>"
    "<tr><td class=\"k\">Netmask</td><td id=\"mask\">…</td></tr>"
    "<tr><td class=\"k\">Access point</td><td id=\"ap\">…</td></tr>"
    "<tr><td class=\"k\">Board</td><td id=\"board\">…</td></tr>"
    "<tr><td class=\"k\">Firmware slot</td><td id=\"slot\">…</td></tr>"
    "</table>"
    "<h2>Join a WiFi network</h2>"
    "<label for=\"ssid\">Network</label>"
    "<select id=\"ssid\" onchange=\"pick()\"><option value=\"\">— scanning… —</option>"
    "<option value=\"__manual__\">Other (enter manually)…</option></select>"
    "<div id=\"manualwrap\" style=\"display:none\">"
    "<label for=\"manual\">SSID</label><input id=\"manual\" autocapitalize=\"none\"></div>"
    "<label for=\"pass\">Password</label>"
    "<input id=\"pass\" type=\"password\" placeholder=\"(blank for open networks)\">"
    "<div><button onclick=\"join()\">Join network</button>"
    "<button class=\"sec\" onclick=\"scan()\">Rescan</button>"
    "<button class=\"sec\" onclick=\"forget()\">Forget</button></div>"
    "<div id=\"msg\"></div>"
    "<h2>Firmware update</h2>"
    "<input type=\"file\" id=\"fw\" accept=\".bin\">"
    "<div><button onclick=\"upload()\">Upload &amp; reboot</button></div>"
    "<div id=\"up\"></div>"
    "<script>"
    "var $=function(i){return document.getElementById(i)};"
    "function pick(){$('manualwrap').style.display=$('ssid').value=='__manual__'?'block':'none'}"
    "function bars(r){return r>=-55?'\\u2588':r>=-67?'\\u2586':r>=-78?'\\u2584':'\\u2582'}"
    "function status(){fetch('/status').then(function(r){return r.json()}).then(function(s){"
    "$('usb').innerHTML=s.usb.up?('<span class=\\\"up\\\">connected</span> <code>'+s.usb.id+'</code>'):'<span class=\\\"down\\\">waiting…</span>';"
    "$('tcp').innerHTML=s.tcp.up?'<span class=\\\"up\\\">connected</span> :'+s.tcp.port:'<span class=\\\"down\\\">none</span> :'+s.tcp.port;"
    "var w=s.wifi,h;"
    "if(w.state=='connected')h='<span class=\\\"up\\\">'+w.ssid+'</span>';"
    "else if(w.state=='connecting')h='<span class=\\\"warn\\\">connecting to '+w.ssid+'…</span>';"
    "else if(w.state=='failed')h='<span class=\\\"warn\\\">failed: '+w.ssid+'</span>';"
    "else h='<span class=\\\"down\\\">none</span>';"
    "$('sta').innerHTML=h;"
    "$('ip').innerHTML=w.ip?'<code>'+w.ip+'</code>':'<span class=\\\"down\\\">—</span>';"
    "$('gw').innerHTML=w.gw?'<code>'+w.gw+'</code>':'<span class=\\\"down\\\">—</span>';"
    "$('mask').innerHTML=w.netmask?'<code>'+w.netmask+'</code>':'<span class=\\\"down\\\">—</span>';"
    "$('ap').innerHTML=w.ap?'<span class=\\\"warn\\\">broadcasting (setup mode)</span>':'<span class=\\\"down\\\">off</span>';"
    "$('board').innerHTML='<code>'+s.ota.board+'</code>';"
    "$('slot').innerHTML='<code>'+s.ota.slot+'</code> '+(s.ota.valid?'<span class=\\\"up\\\">valid</span>':'<span class=\\\"warn\\\">pending verify</span>');"
    "}).catch(function(){})}"
    "function scan(){var s=$('ssid');s.innerHTML='<option>— scanning… —</option>';"
    "fetch('/scan').then(function(r){return r.json()}).then(function(l){"
    "var o='<option value=\\\"\\\">— select —</option>';"
    "l.forEach(function(a){o+='<option value=\\\"'+a.ssid.replace(/\"/g,'&quot;')+'\\\">'+bars(a.rssi)+' '+a.ssid+(a.secure?' \\uD83D\\uDD12':'')+'</option>'});"
    "o+='<option value=\\\"__manual__\\\">Other (enter manually)…</option>';"
    "s.innerHTML=o}).catch(function(){s.innerHTML='<option>scan failed</option>'})}"
    "function join(){var v=$('ssid').value;if(v=='__manual__')v=$('manual').value;"
    "if(!v){$('msg').textContent='Pick or enter a network.';return}"
    "$('msg').textContent='Saving & connecting…';"
    "fetch('/wifi',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},"
    "body:'ssid='+encodeURIComponent(v)+'&pass='+encodeURIComponent($('pass').value)})"
    ".then(function(r){$('msg').textContent=r.ok?'Saved. Connecting — watch the status above.':'Error saving.'})"
    ".catch(function(){$('msg').textContent='Request failed.'})}"
    "function forget(){$('msg').textContent='Clearing…';"
    "fetch('/wifi',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'ssid='})"
    ".then(function(){$('msg').textContent='Stored network cleared.'})}"
    "function upload(){var f=$('fw').files[0];if(!f){$('up').textContent='Pick a .bin file.';return}"
    "var x=new XMLHttpRequest();x.open('POST','/update');"
    "x.upload.onprogress=function(e){if(e.lengthComputable)$('up').textContent='Uploading '+Math.round(e.loaded/e.total*100)+'%…'};"
    "x.onload=function(){$('up').textContent=x.status==200?'Update OK — rebooting, reconnect in ~10s.':'Update failed: '+x.responseText};"
    "x.onerror=function(){$('up').textContent='Upload connection lost.'};"
    "x.send(f)}"
    "status();setInterval(status,2000);scan();"
    "</script></body></html>";

// Append src to dst as a JSON string body (no surrounding quotes), escaping the
// characters JSON requires. Caller supplies the quotes. Truncates safely.
static void json_escape(char *dst, size_t dst_len, const char *src)
{
    size_t o = 0;
    for (; *src && o + 2 < dst_len; src++) {
        unsigned char c = (unsigned char)*src;
        if (c == '"' || c == '\\') {
            dst[o++] = '\\';
            dst[o++] = c;
        } else if (c < 0x20) {
            if (o + 6 >= dst_len) break;
            o += snprintf(dst + o, dst_len - o, "\\u%04x", c);
        } else {
            dst[o++] = c;
        }
    }
    dst[o] = '\0';
}

// In-place URL-decode (%xx and '+'); used on form values.
static void url_decode(char *s)
{
    char *w = s;
    for (char *r = s; *r; r++) {
        if (*r == '%' && isxdigit((unsigned char)r[1]) && isxdigit((unsigned char)r[2])) {
            char hex[3] = { r[1], r[2], 0 };
            *w++ = (char)strtol(hex, NULL, 16);
            r += 2;
        } else if (*r == '+') {
            *w++ = ' ';
        } else {
            *w++ = *r;
        }
    }
    *w = '\0';
}

static esp_err_t root_get(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, PAGE, sizeof(PAGE) - 1);
}

static esp_err_t status_get(httpd_req_t *req)
{
    uint16_t vid = 0, pid = 0;
    bool usb = usb_cdc_host_status(&vid, &pid);
    bool tcp = tcp_server_client_connected();

    wifi_status_t w;
    wifi_sta_status(&w);
    const char *state = w.state == WIFI_STA_CONNECTED  ? "connected"
                      : w.state == WIFI_STA_CONNECTING ? "connecting"
                      : w.state == WIFI_STA_FAILED     ? "failed" : "idle";

    char ssid_esc[200];
    json_escape(ssid_esc, sizeof(ssid_esc), w.ssid);

    char slot[16];
    bool img_valid = true;
    ota_running_info(slot, sizeof(slot), &img_valid);

    char body[700];
    int n = snprintf(body, sizeof(body),
        "{\"usb\":{\"up\":%s,\"id\":\"%04x:%04x\"},"
        "\"tcp\":{\"up\":%s,\"port\":%d},"
        "\"wifi\":{\"state\":\"%s\",\"ap\":%s,\"ssid\":\"%s\","
        "\"ip\":\"%s\",\"gw\":\"%s\",\"netmask\":\"%s\"},"
        "\"ota\":{\"board\":\"%s\",\"slot\":\"%s\",\"valid\":%s}}",
        usb ? "true" : "false", vid, pid,
        tcp ? "true" : "false", TCP_SERVER_PORT,
        state, w.ap_active ? "true" : "false", ssid_esc,
        w.ip, w.gw, w.netmask,
        ota_board_id(), slot, img_valid ? "true" : "false");

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, body, n);
}

static esp_err_t scan_get(httpd_req_t *req)
{
    wifi_scan_ap_t aps[MAX_SCAN_APS];
    int count = wifi_scan(aps, MAX_SCAN_APS);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr_chunk(req, "[");
    for (int i = 0; i < count; i++) {
        char ssid_esc[200];
        json_escape(ssid_esc, sizeof(ssid_esc), aps[i].ssid);
        char item[256];
        int n = snprintf(item, sizeof(item),
            "%s{\"ssid\":\"%s\",\"rssi\":%d,\"secure\":%s}",
            i ? "," : "", ssid_esc, aps[i].rssi, aps[i].secure ? "true" : "false");
        httpd_resp_send_chunk(req, item, n);
    }
    httpd_resp_sendstr_chunk(req, "]");
    return httpd_resp_send_chunk(req, NULL, 0);   // end response
}

static esp_err_t wifi_post(httpd_req_t *req)
{
    char buf[256];
    int len = req->content_len < (int)sizeof(buf) - 1 ? req->content_len : (int)sizeof(buf) - 1;
    int got = httpd_req_recv(req, buf, len);
    if (got <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "no body");
        return ESP_FAIL;
    }
    buf[got] = '\0';

    char ssid[64] = {0};
    char pass[128] = {0};
    httpd_query_key_value(buf, "ssid", ssid, sizeof(ssid));
    httpd_query_key_value(buf, "pass", pass, sizeof(pass));
    url_decode(ssid);
    url_decode(pass);

    ESP_LOGI(TAG, "web request to join '%s'", ssid[0] ? ssid : "(clear)");
    wifi_set_station(ssid, pass);   // persists to NVS + applies live

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

void http_status_start(void)
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port = 80;
    cfg.lru_purge_enable = true;   // free idle sockets so the page stays reachable
    cfg.stack_size = 8192;         // headroom for the blocking scan + JSON

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "failed to start HTTP server");
        return;
    }

    const httpd_uri_t routes[] = {
        { .uri = "/",       .method = HTTP_GET,  .handler = root_get   },
        { .uri = "/status", .method = HTTP_GET,  .handler = status_get },
        { .uri = "/scan",   .method = HTTP_GET,  .handler = scan_get   },
        { .uri = "/wifi",   .method = HTTP_POST, .handler = wifi_post  },
    };
    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); i++) {
        httpd_register_uri_handler(server, &routes[i]);
    }
    ota_register(server);   // POST /update
    ESP_LOGI(TAG, "web UI on http://%s/ (port 80)", WIFI_AP_IP);
}
