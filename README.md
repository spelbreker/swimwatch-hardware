# SwimWatch

High-precision swim meet split timer with remote competition support, built for the LilyGO T-Display S3.

## Features

- **Precision timing**: `esp_timer_get_time()` — 1µs resolution, hardware counter, NTP-independent
- **NTP wall-clock**: `time()` synced every 60s for absolute race timestamps
- **Remote races**: WebSocket protocol for start/split/stop across multiple lanes
- **Configurable**: WiFi + server + NTP via captive portal (no code changes needed)
- **3-button control**: Start/Stop (GPIO0), Reset (GPIO14), Split (GPIO2)

## Hardware

| Component | Spec |
|-----------|------|
| MCU | ESP32-S3R8 Dual-core LX7, 16MB Flash, 8MB PSRAM |
| Display | 1.9" 170×320 ST7789V IPS LCD, 8-bit parallel |
| Power | 3.3V working supply |
| Board | LilyGO T-Display S3 |

### Buttons

```
GPIO0  (BUTTON1) — Start/Stop toggle    — onboard, active LOW
GPIO14 (BUTTON2) — Reset (when stopped) — onboard, active LOW
GPIO2  (external) — Split trigger        — active LOW, internal pull-up
```

### Pin Reference

Display pins are managed by TFT_eSPI via `User_Setup.h`:
TFT_MOSI=19, TFT_SCLK=18, TFT_CS=5, TFT_DC=16, TFT_BL=4.
GPIO15 must be HIGH for battery-powered display operation.

## Architecture

```
main.cpp                    — App entry, mode switching, button loop
├── StopwatchTimer          — Elapsed timing (esp_timer_get_time(), 1µs)
├── NTPManager              — Time sync via SNTP (60s interval, smooth mode)
├── WebSocketStopwatch      — Remote competition protocol (start/split/stop)
├── DisplayManager          — TFT display (50ms refresh, dirty-region tracking)
├── ButtonManager           — Hardware ISR + debounce for GPIO0/GPIO14/GPIO2
└── CaptivePortalManager    — WiFi + NTP server configuration via web form
```

### Timing Strategy

- **Elapsed**: `esp_timer_get_time()` — 1µs hardware counter, NTP-independent, ~±20ppm drift
- **Wall-clock**: `time()` / `getLocalTime()` — NTP-synced RTC for absolute timestamps
- **NTP**: Same server as WebSocket (default `192.168.1.10`), 60s sync interval, SNTP smooth mode
- **Never** `millis()` for precision timing — only for debounce/scheduling

## Display Layout

```
┌──────────────────────────┬──────────────┐
│   Stopwatch Display      │ WiFi Status  │
│     MM:SS.d (running)    │  bars + RSSI │
│     MM:SS.cc (stopped)   ├──────────────┤
├──────────────────────────┤ WebSocket    │
│   Split 1: MM:SS.cc     │  WS + ping   │
│   Split 2: MM:SS.cc     ├──────────────┤
│   Split 3: MM:SS.cc     │ Lane / Role  │
│                          ├──────────────┤
│                          │ NTP Clock    │
└──────────────────────────┴──────────────┘
```

- **Main area** (240×170): Stopwatch time + last 3 splits (rolling)
- **Sidebar** (80×170): WiFi, WebSocket, Lane/Role, NTP clock
- **20fps** refresh, dirty-region tracking for flicker-free updates

## Quick Start

### 1. First Boot — Captive Portal
1. Flash firmware to T-Display S3
2. Device creates AP: **SwimWatch-Setup** (password: `swimwatch123`)
3. Connect and go to `http://192.168.4.1`
4. Enter WiFi credentials, server IP (default: `192.168.1.10:80`), lane number

### 2. Normal Operation
After WiFi connects:
1. NTP syncs (5s timeout)
2. WebSocket connects to server
3. Ready — waiting for start command or GPIO0 press

### Operation

| Button | Action |
|--------|--------|
| GPIO0 (BUTTON1) | Toggle start/stop |
| GPIO14 (BUTTON2) | Reset (only when stopped) |
| GPIO2 (external) | Record split (lane) or send start (starter) |

## WebSocket Protocol

Server default: `ws://192.168.1.10:80/ws`

### Received from server
```json
{"type": "start", "timestamp": 1234567890}
{"type": "reset"}
{"type": "event-heat", "event": "1", "heat": "2"}
{"type": "clear"}
{"type": "device_update_role", "mac": "...", "role": "starter"}
{"type": "device_update_lane", "mac": "...", "lane": 3}
```

### Sent to server
```json
{"type": "split", "lane": 3, "elapsed_ms": 34567, "timestamp": 1234567890}
{"type": "start", "event": "1", "heat": "1", "timestamp": 1234567890}
{"type": "ping", "time": 12345}
{"type": "device_register", "mac": "...", "ip": "...", "role": "lane", "lane": 3}
```

## Project Structure

```
stopwatch/
├── platformio.ini              # Build config
├── include/
│   ├── config.h                # All constants, pins, StopwatchConfig struct
│   ├── stopwatch_timer.h       # esp_timer-based precision timer
│   ├── ntp_manager.h           # SNTP sync manager
│   ├── websocket_stopwatch.h   # WebSocket race protocol
│   ├── display_manager.h       # TFT display controller
│   ├── button_manager.h        # ISR button handler
│   └── captive_portal.h        # WiFi config portal
├── src/
│   ├── main.cpp                # App entry + button loop
│   ├── stopwatch_timer.cpp
│   ├── ntp_manager.cpp
│   ├── websocket_stopwatch.cpp
│   ├── display_manager.cpp
│   ├── button_manager.cpp
│   └── captive_portal.cpp
├── lib/
│   └── TFT_eSPI/               # Local display driver
└── docs/                       # Documentation
```

## Build

```bash
# Build
pio run

# Flash
pio run --target upload

# Serial monitor
pio device monitor -b 115200
```

### Debug Build
Add `-DDEBUG_STOPWATCH` to `build_flags` in `platformio.ini` for verbose `DEBUG_LOG()` output.

### Dependencies
- `WebSockets @ ^2.4.1`
- `ArduinoJson @ ^6.21.3`
- Local `TFT_eSPI` (in `lib/`)

## Configuration (NVS)

Stored in ESP32 NVS Preferences (namespace: `"stopwatch"`):

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `ws_server` | String | `192.168.1.10` | WebSocket + NTP server IP |
| `ws_port` | UInt | `80` | Server port |
| `lane` | UInt | `9` | Lane number |
| `role` | String | `lane` | Device role (`lane` or `starter`) |
| `ntp_server` | String | *(empty)* | NTP server override (empty = use ws_server) |

## Resources

- [LilyGO T-Display S3](https://lilygo.cc/products/t-display-s3)
- [T-Display S3 GitHub](https://github.com/Xinyuan-LilyGO/T-Display-S3)
