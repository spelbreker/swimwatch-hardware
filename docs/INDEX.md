# Documentation Index — SwimWatch

## Quick Navigation

| Document | Audience | Contents |
|----------|----------|----------|
| [README.md](../README.md) | Everyone | Project overview, quick start, features |
| [API.md](API.md) | Developers | Full API reference for all modules |
| [DISPLAY_MANAGER.md](DISPLAY_MANAGER.md) | Developers | Display layout, colours, rendering |
| [HARDWARE.md](HARDWARE.md) | Hardware / Advanced | Pin assignments, wiring, power |
| [DEVELOPER.md](DEVELOPER.md) | Contributors | Architecture, build, code style |

---

## For First-Time Users

1. Start with **[README.md](../README.md)** for overview and setup
2. Connect hardware per **[HARDWARE.md](HARDWARE.md)**
3. Build and upload: `pio run --target upload`

## For Developers

1. Read **[DEVELOPER.md](DEVELOPER.md)** for architecture and startup flow
2. Use **[API.md](API.md)** as reference during development
3. See **[DISPLAY_MANAGER.md](DISPLAY_MANAGER.md)** for layout coordinates
4. Check **[HARDWARE.md](HARDWARE.md)** for pin constraints

## For Contributors

1. Follow code style in **[DEVELOPER.md](DEVELOPER.md)**
2. Update relevant docs with code changes
3. Test on actual T-Display S3 hardware
4. Submit a pull request

---

## Module Overview

| Module | Header | Source | Purpose |
|--------|--------|--------|---------|
| Config | `config.h` | -- | Pins, constants, `StopwatchConfig` |
| StopwatchTimer | `stopwatch_timer.h` | `stopwatch_timer.cpp` | esp_timer elapsed timing |
| NTPManager | `ntp_manager.h` | `ntp_manager.cpp` | SNTP wall-clock sync |
| WebSocketStopwatch | `websocket_stopwatch.h` | `websocket_stopwatch.cpp` | Race protocol |
| DisplayManager | `display_manager.h` | `display_manager.cpp` | TFT display rendering |
| ButtonManager | `button_manager.h` | `button_manager.cpp` | ISR button handling |
| CaptivePortalManager | `captive_portal.h` | `captive_portal.cpp` | WiFi config portal |
| Main | -- | `main.cpp` | App entry, mode switching |
