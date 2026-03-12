# Copilot Instructions — SwimWatch

## Project Overview

- **Project Name:** SwimWatch
- **Purpose:** High-precision swim meet split timer with remote competition support
- **Platform:** LilyGO T-Display S3 (ESP32-S3) + PlatformIO (Arduino framework)
- **Server:** WebSocket + NTP server at configurable IP (default `192.168.1.10:80`)

## Hardware

- **MCU:** ESP32-S3R8 Dual-core LX7, 16MB Flash, 8MB PSRAM
- **Display:** 1.9" 170×320 ST7789V IPS LCD, 8-bit parallel interface
- **Working Power Supply:** 3.3V
- **Onboard Buttons:**
  - **BUTTON1 (GPIO0):** Start/Stop
  - **BUTTON2 (GPIO14):** Reset
- **External Input:**
  - **GPIO2:** External split trigger (active LOW)
- **Display Pins** (managed by TFT_eSPI via `User_Setup.h`):
  - TFT_MOSI=GPIO19, TFT_SCLK=GPIO18, TFT_CS=GPIO5, TFT_DC=GPIO16, TFT_BL=GPIO4

## Architecture

```
main.cpp                    — App entry, mode switching, button loop
├── StopwatchTimer          — Elapsed timing (esp_timer_get_time(), 1µs resolution)
├── NTPManager              — Time sync via SNTP (60s interval, smooth mode)
├── WebSocketStopwatch      — Remote competition protocol (start/split/stop)
├── DisplayManager          — TFT display (50ms refresh, dirty-region tracking)
├── ButtonManager           — Hardware ISR + debounce for GPIO0/GPIO14/GPIO2
└── CaptivePortalManager    — WiFi + NTP server configuration via web form
```

### Timing Strategy

- **Elapsed timing:** `esp_timer_get_time()` — 1µs resolution, 64-bit hardware counter
  - Independent of NTP/RTC — not affected by time sync adjustments
  - Drift: ~±20ppm (~±0.6ms over 30s, ~±36ms over 30min)
- **Wall-clock timestamps:** `time()` / `getLocalTime()` — NTP-synced RTC
  - Used for absolute timestamps on split/finish records
  - NTP re-syncs every 60 seconds via SNTP (smooth mode, no jumps)
  - LAN accuracy: ±1-2ms per sync, ±1.2ms max drift between syncs
- **Never use `millis()` for timing** — only for debounce, intervals, non-precision tasks
- **NTP server = WebSocket server** — same IP (`192.168.1.10`), configurable in portal

### Configuration

- Stored in ESP32 NVS Preferences (namespace: `"stopwatch"`)
- Configured via captive portal AP (`SwimWatch-Setup` / `swimwatch123`)
- Settings: WiFi SSID/password, server IP/port, NTP server, device role, lane number
- All defaults defined in `include/config.h`

## App Behavior

- **Display:** Elapsed time as `MM:SS.d` (1-digit deciseconds while running)
- **Stopped:** Full precision `MM:SS.cc` (2-digit centiseconds)
- **Refresh:** Every 50ms (20fps) — smooth for 1-digit display
- **Buttons:** Hardware interrupts with 200ms software debounce
  - BUTTON1 (GPIO0): Start/Stop toggle
  - BUTTON2 (GPIO14): Reset (only when stopped)
  - GPIO2: Record split time (only when running)
- **Startup flow:** WiFi → NTP sync (5s timeout) → WebSocket connect → Ready
- **Splash screen:** Shows "SwimWatch" + "T-Display S3" + "Initializing..."

## Modules

| Module | Files | Purpose |
|--------|-------|---------|
| Config | `include/config.h` | Pin defs, defaults, `StopwatchConfig` struct, `DEBUG_LOG` macro |
| Timer | `stopwatch_timer.h/cpp` | `esp_timer`-based elapsed timing, split recording |
| NTP | `ntp_manager.h/cpp` | SNTP sync (60s), wall-clock timestamps |
| WebSocket | `websocket_stopwatch.h/cpp` | Race protocol (start/split/stop), server communication |
| Display | `display_manager.h/cpp` | TFT rendering, dirty regions, sidebar (WiFi/lane/time) |
| Buttons | `button_manager.h/cpp` | ISR handlers, debounce, GPIO0/GPIO14/GPIO2 |
| Portal | `captive_portal.h/cpp` | AP mode config form, Preferences save/load |

## Style Rules

- Use descriptive variable and function names
- Keep functions short and focused on a single task
- Use comments to explain hardware-specific logic and pin assignments
- Group related code into classes — one concern per class
- All pin numbers and constants in `include/config.h` as `constexpr`
- Use `setup()` for initialization, `loop()` for main non-blocking logic
- Debounce button inputs via hardware ISR + software debounce timer
- Update display only when state changes (dirty-region tracking)
- **No blocking code in `loop()`** — use `millis()` intervals for scheduling
- Use `DEBUG_LOG()` macro for debug output — disabled in production via build flag
- Document hardware dependencies in file-level docblocks
- Every `.h` file has a `@file` / `@brief` docblock
- Every public function has a brief doc comment explaining purpose and params

## Build

- **Platform:** PlatformIO, `espressif32`, board `lilygo-t-display-s3`
- **Framework:** Arduino
- **Dependencies:** `WebSockets@^2.4.1`, `ArduinoJson@^6.21.3`, local `TFT_eSPI`
- **Debug build:** Add `-DDEBUG_STOPWATCH` to `build_flags` in `platformio.ini`

## Resources

- https://lilygo.cc/products/t-display-s3
- https://github.com/Xinyuan-LilyGO/T-Display-S3
- https://gist.github.com/buzzkillb/2b6381632f73ee965e1c9329163fdfd3
