# betaflight-host

[![build](https://github.com/betaflight/betaflight-host/actions/workflows/build.yml/badge.svg)](https://github.com/betaflight/betaflight-host/actions/workflows/build.yml)

ESP32-S3 USB-host-to-WiFi bridge for Betaflight. The board acts as USB **host**
to a flight controller's Virtual COM Port (VCP) and exposes that serial stream
over TCP, so Betaflight Configurator can connect wirelessly from a phone or
laptop.

```
[FC USB VCP] <--USB host--> [ESP32-S3 mini] <--WiFi / TCP:5761--> [Configurator]
```

It is a transparent byte bridge — no MSP parsing happens on the ESP32.

## Hardware

- **ESP32-S3** mini (the S3's native USB-OTG peripheral is required for USB host).
- A USB-A host port / OTG adapter wired to the S3 OTG pins (D+ GPIO20, D- GPIO19).
- 5 V supply able to power both the ESP32 and the attached FC.

## Layout

| Path | Role |
|------|------|
| `src/main/main.c` | Startup: NVS, bridge, WiFi, TCP server, USB host |
| `src/main/usb_cdc_host.c` | USB host + CDC-ACM; opens the FC VCP, pumps bytes |
| `src/main/tcp_server.c` | TCP listener on 5761; one Configurator client at a time |
| `src/main/wifi.c` | SoftAP (always) + optional station, creds in NVS |
| `src/main/http_status.c` | Status web page on port 80 (USB + TCP state) |
| `src/main/bridge.c` | Two stream buffers decoupling the USB and TCP contexts |
| `esp-idf/` | Pinned ESP-IDF (git submodule, `release/v5.4`, shallow) |

## Setup

ESP-IDF is vendored as a submodule. After cloning this repo:

```sh
# Host prerequisites (Debian/Ubuntu): ESP-IDF's installer needs a working
# venv + pip to build its Python environment.
sudo apt install git wget cmake ninja-build python3-venv python3-pip

git submodule update --init --depth 1 esp-idf
./esp-idf/install.sh esp32s3
. ./esp-idf/export.sh
```

## Build & flash

```sh
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

## Connecting

1. Power the board with the FC plugged into the host port.
2. Join the WiFi network **`betaflight-host`** (default password `betaflight`).
3. In Configurator choose the **TCP** connection, host `192.168.4.1`, port `5761`.

Browse to `http://192.168.4.1/` for a live status page (USB and TCP state).

To instead join an existing network (station mode), store credentials in NVS
under namespace `wifi` (keys `ssid` / `pass`) — see `wifi_set_station()` — and
reboot. The SoftAP stays up alongside station mode.

## Notes

- Known FC VCP USB IDs (ST / Artery / Geehy) are listed in `usb_cdc_host.c`;
  add new vendors there.
- Single client at a time — Configurator opens one connection.
- `TCP_NODELAY` is set and Nagle effectively disabled to keep MSP latency low.

## Licence

GPL-3.0, matching Betaflight. See [LICENSE](LICENSE).
