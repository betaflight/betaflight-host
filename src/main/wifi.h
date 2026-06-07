// WiFi bring-up. Always starts a SoftAP so a phone/laptop can connect with no
// existing network; additionally joins a station network if credentials are
// stored in NVS (namespace "wifi", keys "ssid"/"pass").
#pragma once

// Default SoftAP identity. Override at build time with -D, or store station
// credentials in NVS to also join an existing network.
#ifndef WIFI_AP_SSID
#define WIFI_AP_SSID   "betaflight-host"
#endif
#ifndef WIFI_AP_PASS
#define WIFI_AP_PASS   "betaflight"   // >= 8 chars for WPA2; empty => open AP
#endif
#ifndef WIFI_AP_CHANNEL
#define WIFI_AP_CHANNEL 6
#endif

// Bring up netif/event loop and start AP (+STA if NVS creds present). Blocks
// only briefly; association happens asynchronously.
void wifi_start(void);

// Store station credentials in NVS and (re)connect as STA. Pass NULL/empty ssid
// to clear stored creds and run AP-only.
void wifi_set_station(const char *ssid, const char *pass);
