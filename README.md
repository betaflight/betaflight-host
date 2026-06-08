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
| `src/main/wifi.c` | Station-first WiFi: joins a stored network, SoftAP fallback, creds in NVS |
| `src/main/http_status.c` | Web UI on port 80: status + scan/join a network + firmware upload |
| `src/main/ota.c` | `POST /update` OTA handler; streams an uploaded .bin into the spare slot |
| `src/main/bridge.c` | Two stream buffers decoupling the USB and TCP contexts |
| `boards/<board>/` | Per-board flash size, partition table, PSRAM and identity |
| `esp-idf/` | Pinned ESP-IDF (git submodule, `release/v5.4`, shallow) |

## Boards

The board is selected at configure time with `-DBOARD=<name>`, where `<name>` is
a directory under `boards/`. Each board provides its own `sdkconfig.defaults`
(flash size, PSRAM, partition CSV) and `board.h`, layered on the shared
top-level `sdkconfig.defaults`. The USB-host pins (D- GPIO19 / D+ GPIO20) are
fixed on the ESP32-S3 and identical across boards.

| BOARD | Hardware |
|-------|----------|
| `esp32s3-zero` (default) | Waveshare ESP32-S3-ZERO — 4 MB flash, no PSRAM, single USB-C (native, shared with USB-host) |
| `esp32s3-wroom-freenove` | Freenove ESP32-S3-WROOM N8R8 — 8 MB flash, 8 MB octal PSRAM, dual USB-C (native + CH343 UART, so the console survives USB-host mode) |

A board identity is baked into the firmware (`esp_app_desc.version`) and checked
on OTA, so an image built for one board is refused on another (see below).

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

# Build for a board (default is esp32s3-zero). When switching boards, delete
# ./sdkconfig first so it regenerates from that board's defaults.
idf.py -DBOARD=esp32s3-wroom-freenove build
idf.py -p /dev/ttyACM0 flash monitor
```

On the dual-USB Freenove, flash/monitor over the CH343 UART port — it stays
connected even after the firmware switches the native USB into host mode. On the
single-port ZERO the console drops once host mode engages.

## Connecting

On first boot — or whenever no network has been configured — the board brings
up its own SoftAP so you can set it up:

1. Power the board with the FC plugged into the host port.
2. Join the WiFi network **`betaflight-host`** (default password `betaflight`).
   The board runs a DHCP server, so you'll get a `192.168.4.x` lease
   automatically with `192.168.4.1` as the gateway.
3. Browse to `http://192.168.4.1/`. The page shows live USB/TCP/WiFi status and
   lets you **scan for and join your home network**: pick an SSID (or type one),
   enter the password, and hit *Join*. Credentials are saved to NVS and applied
   immediately — the status panel shows the assigned IP and netmask.
4. In Configurator choose the **TCP** connection, host `192.168.4.1`, port
   `5761` (or the station IP once joined).

After a network is stored, **subsequent boots join it directly as a station and
the SoftAP is not started** — reach the web UI and Configurator at the IP your
router assigns (shown on the page). If that network is ever unreachable at boot,
the SoftAP comes back up automatically so you can reconfigure. Use *Forget* on
the page to clear the stored network and return to AP-only setup mode.

## Updating (OTA)

After the first serial flash, firmware is updated over WiFi — no cable. On the
web page use **Firmware update**: pick a `build/betaflight-host.bin` and hit
*Upload & reboot*. The image streams into the spare OTA slot, the boot partition
is switched, and the board restarts (~10 s); reconnect to the page afterwards.

The layout is dual-OTA (`ota_0`/`ota_1`) with rollback enabled: a freshly
uploaded image boots in *pending-verify* state and only sticks once it comes up
healthy (`ota_mark_valid()` in `main.c`). A bad image that fails to boot is
rolled back to the previous slot automatically.

Both boards are `esp32s3`, so the image validator can't catch a wrong-*board*
upload (different partition layout). The board id baked into each image is
therefore checked on upload: an image for another board is rejected with a 400
and the boot partition is left untouched. The running board and slot are shown
on the status page.

> The dual-OTA partition table only takes effect from a **serial** flash, so the
> initial `idf.py flash` below is the last one that needs the cable.

## Notes

- Known FC VCP USB IDs (ST / Artery / Geehy) are listed in `usb_cdc_host.c`;
  add new vendors there.
- Single client at a time — Configurator opens one connection.
- `TCP_NODELAY` is set and Nagle effectively disabled to keep MSP latency low.

## Licence

GPL-3.0, matching Betaflight. See [LICENSE](LICENSE).
