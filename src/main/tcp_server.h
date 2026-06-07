// TCP server that Betaflight Configurator connects to (TCP transport, MSP over a
// raw stream). Accepts a single client at a time and bridges it to the FC VCP.
#pragma once

#include <stdbool.h>

// Default listen port. Matches Betaflight SITL / Configurator's TCP default.
#ifndef TCP_SERVER_PORT
#define TCP_SERVER_PORT  5761
#endif

// Spawn the listener/accept/RX task. Returns immediately.
void tcp_server_start(void);

// True while a Configurator client is connected.
bool tcp_server_client_connected(void);
