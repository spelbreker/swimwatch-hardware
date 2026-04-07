# AGENTS.md — SwimWatch

## Project Overview

High-precision swim meet split timer for competitive swimming events.
Runs on LilyGO T-Display S3 (ESP32-S3) with a 1.9" TFT display, three hardware buttons,
WiFi + WebSocket for remote race coordination, and NTP for synchronized wall-clock timestamps.

## Tech Stack

- **ESP32-S3** (Arduino framework via PlatformIO)
- **TFT_eSPI** (local lib) — ST7789V 320×170 display driver
- **WebSockets** `^2.4.1` — WebSocket client for race protocol
- **ArduinoJson** `^6.21.3` — JSON serialization for WebSocket messages
- **Build:** `pio run` / `pio run --target upload` / `pio device monitor`

## File Organization

```
include/config.h              → All pins, constants, defaults, StopwatchConfig struct
include/*.h                   → One header per module (docblock required)
src/*.cpp                     → One source per module
src/main.cpp                  → Entry point: setup/loop, mode switching, callbacks
lib/TFT_eSPI/                 → Local display driver (do NOT modify)
docs/                         → API, hardware, timing, display documentation
```

## Architecture

```
main.cpp                      → App entry, mode switching, button event dispatch
├── StopwatchTimer            → Elapsed timing (esp_timer_get_time(), 1µs)
├── NTPManager                → Wall-clock sync via SNTP (60s interval)
├── WebSocketStopwatch        → Race protocol client (start/split/stop/register)
├── DisplayManager            → TFT rendering, dirty-region tracking, 20fps
├── ButtonManager             → ISR handlers + debounce for 3 GPIOs
└── CaptivePortalManager      → WiFi config portal (AP mode web form)
```

### Startup Flow

1. Check NVS for stored WiFi credentials
2. If found → WiFi connect → NTP sync (5s timeout) → WebSocket connect
3. If missing / failed → captive portal (`SwimWatch-Setup` AP)

### Timing Strategy

**Critical — read [`docs/TIMING_EXPLANATION.md`](docs/TIMING_EXPLANATION.md) for full details.**

| Purpose | API | When to use |
|---------|-----|-------------|
| Elapsed timing | `esp_timer_get_time()` | Stopwatch precision (1µs, hardware counter) |
| Wall-clock | `time()` / `getLocalTime()` | Absolute timestamps (NTP-synced) |
| Scheduling | `millis()` | Debounce, display intervals, non-precision tasks |

**Never use `millis()` for precision timing.**

## Core Patterns

### Header File Pattern

Every header must have a `@file` / `@brief` docblock and include guard:

```cpp
/**
 * @file module_name.h
 * @brief One-line purpose description
 *
 * Optional detail about hardware specifics, timing, or constraints.
 */
#pragma once  // or #ifndef/#define/#endif

#include <cstdint>
#include "config.h"

class ModuleNameManager {
public:
    bool init();
    // ... public API with brief doc comments
private:
    // ... internal state
};
```

### ISR Handler Pattern

Interrupts must use static wrappers with software debounce:

```cpp
// In header:
static ModuleManager* _instance;
static void IRAM_ATTR _isrHandler();

// In source:
void IRAM_ATTR ModuleManager::_isrHandler() {
    uint32_t now = millis();
    if (now - _instance->_lastTrigger > BUTTON_DEBOUNCE_MS) {
        _instance->_lastTrigger = now;
        _instance->_flag = true;  // Set volatile flag, process in loop
    }
}
```

### Constants Pattern

All hardware pins and magic numbers go in `include/config.h`:

```cpp
constexpr uint8_t  PIN_BUTTON_START_STOP = 0;
constexpr uint32_t DISPLAY_UPDATE_INTERVAL_MS = 50;
constexpr float    BATTERY_MAX_VOLTAGE = 4.2f;
```

Never hardcode pin numbers or timing constants in source files.

### Display Update Pattern

Use dirty-region tracking — never full-screen redraws:

```cpp
// Mark dirty when state changes
stopwatchAreaDirty = true;

// In update loop (called at 20fps):
if (stopwatchAreaDirty) {
    clearArea(x, y, w, h);
    // redraw only this region
    stopwatchAreaDirty = false;
}
```

### Debug Logging Pattern

```cpp
DEBUG_LOG("Button → split, lane=%d, elapsed=%dms", lane, elapsed);
// Expands to Serial.printf() when -DDEBUG_STOPWATCH is set
// Compiles to nothing in production
```

### WebSocket Message Pattern

```cpp
StaticJsonDocument<256> doc;
doc["type"] = WS_MSG_SPLIT;
doc["lane"] = _laneNumber;
doc["time"] = formattedTime;
doc["timestamp"] = (uint64_t)time(nullptr) * 1000;

String payload;
serializeJson(doc, payload);
_ws.sendTXT(payload);
```

Use the `WS_MSG_*` constants from `websocket_stopwatch.h` — never raw strings.

### Callback Wiring Pattern

Main.cpp wires up module events via function pointers:

```cpp
stopwatch.onStateChanged    = onStopwatchStateChanged;
stopwatch.onLapAdded        = onLapAdded;
stopwatch.onConnectionChanged = onConnectionChanged;
```

### Non-Blocking Loop Pattern

```cpp
void loop() {
    unsigned long now = millis();

    handleButtonEvents();       // Always: highest priority
    stopwatch.loop();           // Always: process WebSocket

    if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL_MS) {
        updateDisplay();
        lastDisplayUpdate = now;
    }
    if (now - lastStatusUpdate >= STATUS_UPDATE_INTERVAL_MS) {
        checkConnections();
        lastStatusUpdate = now;
    }
}
```

No `delay()` or blocking calls in `loop()`.

## Code Style & Conventions

### Naming

- Classes: `PascalCase` with `*Manager` suffix — `ButtonManager`, `DisplayManager`
- Methods: `camelCase` — `getElapsedMs()`, `addSplit()`
- Constants: `UPPER_SNAKE_CASE` — `PIN_BUTTON_SPLIT`, `DISPLAY_UPDATE_INTERVAL_MS`
- Private members: `_camelCase` prefix — `_laneNumber`, `_isRunning`
- Enums: `UPPER_SNAKE_CASE` values — `BUTTON_START_STOP`, `STOPWATCH_RUNNING`

### Includes

```cpp
#include <Arduino.h>          // System headers first
#include <WebSocketsClient.h> // Library headers
#include "config.h"           // Project headers (always include config.h)
#include "stopwatch_timer.h"
```

### Configuration Storage

- Runtime config: `StopwatchConfig` struct (defined in `config.h`)
- Persistent storage: ESP32 NVS `Preferences` (namespace `"stopwatch"`)
- User configuration: captive portal web form → saved to NVS → ESP restart

## Commands

```bash
pio run                      # Build firmware
pio run --target upload      # Flash to device
pio device monitor           # Serial monitor (115200 baud)
```

## General Rules

- Follow existing conventions in sibling files before introducing new patterns
- Do not modify `lib/TFT_eSPI/` — it's a vendored display driver
- Do not change dependencies without explicit approval
- Do not create documentation files unless explicitly requested
- All new constants go in `include/config.h`
- One class per file, one concern per class
- Every public method needs a brief doc comment
- Test hardware-dependent code by building with `pio run` (no unit test framework)
