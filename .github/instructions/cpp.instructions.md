---
applyTo: "**/*.{cpp,h,ino}"
---

# C++ / Arduino Coding Standards — SwimWatch

## Timing APIs

```cpp
// Precision elapsed timing (stopwatch):
int64_t now = esp_timer_get_time();  // microseconds, 64-bit

// Wall-clock timestamps (NTP-synced):
time_t ts = time(nullptr);

// Scheduling and debounce ONLY:
unsigned long now = millis();
```

Never use `millis()` or `micros()` for stopwatch/split timing.

## Constants

All pin numbers, intervals, and magic numbers in `include/config.h` as `constexpr`:

```cpp
// GOOD
constexpr uint8_t PIN_BUTTON_SPLIT = 2;
constexpr uint32_t DISPLAY_UPDATE_INTERVAL_MS = 50;

// BAD — hardcoded in source
pinMode(2, INPUT_PULLDOWN);
if (now - last > 50) { ... }
```

## ISR Handlers

- Mark with `static IRAM_ATTR`
- Use `volatile` flags — no heavy logic in ISR
- Software debounce using `millis()` delta check
- Static instance pointer pattern for `attachInterrupt`

```cpp
void IRAM_ATTR MyManager::_isrHandler() {
    uint32_t now = millis();
    if (now - _instance->_lastTrigger > BUTTON_DEBOUNCE_MS) {
        _instance->_lastTrigger = now;
        _instance->_flag = true;
    }
}
```

## Display Updates

Use dirty-region tracking. Never call full-screen `fillScreen()` in the update loop.

```cpp
if (wifiAreaDirty) {
    clearArea(STATUS_AREA_X, AREA_WIFI_STATUS_Y, STATUS_AREA_WIDTH, AREA_WIFI_STATUS_HEIGHT);
    // redraw region
    wifiAreaDirty = false;
}
```

## Non-Blocking Loop

No `delay()` or blocking calls in `loop()`. Schedule with `millis()` intervals:

```cpp
if (millis() - lastUpdate >= INTERVAL_MS) {
    doWork();
    lastUpdate = millis();
}
```

## Debug Output

Use `DEBUG_LOG()` macro — never raw `Serial.printf()` for debug messages:

```cpp
DEBUG_LOG("Split %d: elapsed=%dms", splitNum, elapsed);
```

## Header Structure

Every `.h` file needs:

```cpp
/**
 * @file filename.h
 * @brief One-line description
 */
#pragma once

#include "config.h"
```

## Naming

- Classes: `PascalCase` + `Manager` suffix
- Methods: `camelCase`
- Constants: `UPPER_SNAKE_CASE`
- Private members: `_camelCase`
- Enums: `UPPER_SNAKE_CASE` values

## WebSocket Messages

Use `WS_MSG_*` constants — never raw type strings:

```cpp
doc["type"] = WS_MSG_SPLIT;  // not "split"
```

## JSON

Use `StaticJsonDocument` with appropriate size. Prefer stack allocation over heap.
