# Copilot Instructions — SwimWatch

## Project Overview

High-precision swim meet split timer with remote competition support.
LilyGO T-Display S3 (ESP32-S3) + PlatformIO (Arduino framework).

## Important Documentation

**Read and follow the guidelines in these files:**

- [`AGENTS.md`](../AGENTS.md) — Detailed coding guide with code examples and architecture
- [`docs/DEVELOPER.md`](../docs/DEVELOPER.md) — Developer guide, module descriptions
- [`docs/API.md`](../docs/API.md) — WebSocket protocol and message types
- [`docs/HARDWARE.md`](../docs/HARDWARE.md) — Pin assignments, electrical specs
- [`docs/TIMING_EXPLANATION.md`](../docs/TIMING_EXPLANATION.md) — Timer precision details

## Tech Stack

- **PlatformIO** (`espressif32`, Arduino framework) — board `lilygo-t-display-s3`
- **Dependencies:** `WebSockets@^2.4.1`, `ArduinoJson@^6.21.3`, local `TFT_eSPI`
- **MCU:** ESP32-S3R8 (dual-core LX7, 16MB Flash, 8MB PSRAM)
- **Display:** 1.9" 170×320 ST7789V IPS LCD

## File Organization

- **Config:** `include/config.h` — all pins, constants, defaults, `StopwatchConfig` struct
- **Headers:** `include/*.h` — one header per module
- **Sources:** `src/*.cpp` — one source per module
- **Entry point:** `src/main.cpp` — setup/loop, mode switching, callbacks

## Key Rules

- Use `esp_timer_get_time()` for elapsed timing — never `millis()` for precision
- Use `time()` / `getLocalTime()` for wall-clock timestamps (NTP-synced)
- Use `millis()` only for debounce, scheduling intervals, non-precision tasks
- All pin numbers and constants in `include/config.h` as `constexpr`
- No blocking code in `loop()` — use `millis()` intervals for scheduling
- Use `DEBUG_LOG()` macro for debug output — disabled without `-DDEBUG_STOPWATCH`
- One class per module, `*Manager` naming convention
- Every `.h` file needs `@file` / `@brief` docblock
- Update display via dirty-region tracking, not full redraws
- ISR handlers must be `static IRAM_ATTR` with software debounce

## Key Commands

```bash
pio run                      # Build firmware
pio run --target upload      # Flash to device
pio device monitor           # Serial monitor (115200 baud)
```
