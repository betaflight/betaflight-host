// Minimal status web page on port 80: shows USB (FC) and TCP (Configurator)
// connection state. Read-only, auto-refreshing.
#pragma once

// Start the HTTP server. Call after WiFi is up.
void http_status_start(void);
