# Developer Guide — SwimWatch

## Project Overview

SwimWatch is a high-precision swim meet split timer running on the LilyGO T-Display S3 (ESP32-S3R8). It connects to a WebSocket server for remote race control and uses NTP for wall-clock timestamps.

**Key design decisions:**
- `esp_timer_get_time()` for elapsed timing (1 us, NTP-independent)
- NTP-synced `time()` for wall-clock timestamps
- Never `millis()` for precision timing (only scheduling/debounce)
- Dirty-region display updates (20 fps stopwatch, 1 Hz status)
- All config in `config.h`, stored in NVS Preferences

---

## Architecture

```
main.cpp                        App entry, mode switching, button loop
+-- StopwatchTimer              Elapsed timing (esp_timer, 1 us resolution)
+-- NTPManager                  SNTP sync (60s interval, smooth mode)
+-- WebSocketStopwatch          Race protocol (start/split/stop/event-heat)
+-- DisplayManager              TFT rendering, dirty regions, sidebar
+-- ButtonManager               ISR + debounce for GPIO0/GPIO14/GPIO2
+-- CaptivePortalManager        WiFi + server config via web form
```

### Module Dependencies

```
main.cpp
  +-> DisplayManager       (TFT_eSPI)
  +-> ButtonManager        (config.h)
  +-> NTPManager
  +-> WebSocketStopwatch   (WebSocketsClient, ArduinoJson, StopwatchTimer)
  +-> CaptivePortalManager (WiFi, DNSServer, WebServer, Preferences)
```

---

## Project Structure

```
stopwatch/
+-- platformio.ini              Build configuration
+-- include/
|   +-- config.h                Pins, constants, StopwatchConfig, DEBUG_LOG
|   +-- display_manager.h       Display API + layout constants
|   +-- button_manager.h        Button events + ISR handler
|   +-- stopwatch_timer.h       esp_timer precision timer
|   +-- ntp_manager.h           SNTP time sync
|   +-- websocket_stopwatch.h   WebSocket race protocol
|   +-- captive_portal.h        AP-mode configuration
+-- src/
|   +-- main.cpp                App entry, setup(), loop()
|   +-- display_manager.cpp
|   +-- button_manager.cpp
|   +-- stopwatch_timer.cpp
|   +-- ntp_manager.cpp
|   +-- websocket_stopwatch.cpp
|   +-- captive_portal.cpp
+-- lib/
|   +-- TFT_eSPI/               Local copy with T-Display S3 User_Setup.h
+-- docs/
|   +-- API.md                  Full API reference
|   +-- DISPLAY_MANAGER.md      Display layout and rendering
|   +-- HARDWARE.md             Pin assignments and wiring
|   +-- DEVELOPER.md            This file
|   +-- INDEX.md                Documentation index
+-- examples/
|   +-- simple/main.cpp
|   +-- display_test/main.cpp
|   +-- helloworld.cpp
```

---

## Development Environment

### Requirements

- [PlatformIO](https://platformio.org/) (CLI or VS Code extension)
- ESP32 toolchain (installed automatically by PlatformIO)
- USB-C cable for programming

### Build

```bash
# Build firmware
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor

# Erase NVS (reset all config)
pio run --target erase
```

### Debug Build

Add `-DDEBUG_STOPWATCH` to `build_flags` in `platformio.ini`:

```ini
build_flags = -DDEBUG_STOPWATCH
```

This enables the `DEBUG_LOG()` macro which prints `[DBG] ...` to Serial.

---

## Timing Architecture

### Elapsed Timing (StopwatchTimer)

- Uses `esp_timer_get_time()` -- 64-bit hardware counter, 1 us resolution
- **Not affected by NTP sync** -- completely independent
- Drift: ~+/-20 ppm (~0.6 ms per 30s, ~36 ms per 30 min)

### Wall-Clock Timestamps (NTPManager)

- Uses `time()` / `getLocalTime()` synced by SNTP
- Re-syncs every 60s in smooth mode (no jumps)
- LAN accuracy: +/-1-2 ms per sync
- Used for absolute timestamps on split/finish records

### Scheduling (millis)

- `millis()` is used **only** for:
  - Display update intervals (50 ms, 1000 ms)
  - Button debounce (200 ms)
  - WebSocket reconnect delays
- **Never** for precision timing

---

## Startup Flow

```
1. Serial.begin(115200)
2. Display init + splash screen
3. Check NVS for stored WiFi credentials
4. If found:
   a. WiFi.begin(ssid, password)  -- 10s timeout
   b. NTP sync via SNTP           -- 5s timeout
   c. WebSocket connect
   d. Ready (main loop)
5. If missing / failed:
   a. Start captive portal AP (SwimWatch-Setup)
   b. User configures via 192.168.4.1
   c. Save to NVS, restart
```

---

## Configuration

All configuration is stored in ESP32 NVS Preferences (namespace `"stopwatch"`).

Configurable via captive portal or by erasing NVS and re-entering.

| Setting | NVS Key | Default |
|---------|---------|---------|
| WiFi SSID | `ssid` | -- |
| WiFi Password | `password` | -- |
| Server IP | `serverIP` | `192.168.8.10` |
| Server Port | `port` | `80` |
| NTP Server | `ntpServer` | *(empty = server IP)* |
| Lane Number | `lane` | `9` |
| Role | `role` | `lane` |

The NTP server defaults to the same IP as the WebSocket server, keeping the setup simple for typical LAN deployments.

---

## Code Style

- **Naming:** PascalCase for classes, camelCase for functions/variables, UPPER_CASE for constants
- **Functions:** Short, single-purpose
- **Comments:** Explain hardware-specific logic and pin assignments
- **Classes:** One concern per class
- **Constants:** All pins and magic numbers in `config.h` as `constexpr`
- **No blocking in loop():** Use `millis()` intervals for scheduling
- **Debug output:** Use `DEBUG_LOG()` macro, disabled in production
- **Doc comments:** Every `.h` has `@file` / `@brief`; every public function has a brief comment

---

## Display System

See [DISPLAY_MANAGER.md](DISPLAY_MANAGER.md) for full details.

Key points:
- 320x170 landscape, two panels (240px main + 80px sidebar)
- Dirty-region tracking per area (7 areas)
- String caching prevents redundant redraws
- 20 fps for stopwatch, 1 Hz for status areas
- Swimming pool blue sidebar theme (`COLOR_SIDEBAR_BG = 0x049D`)

---

## Button System

ISR-driven with 200 ms software debounce (`BUTTON_DEBOUNCE_MS`).

| GPIO | Event | Function |
|------|-------|----------|
| 0 | `BUTTON_START_STOP` | Toggle start/stop |
| 14 | `BUTTON_RESET` | Reset (only when stopped) |
| 2 | `BUTTON_LAP_PRESSED` | Record split / send start |

Each button gets a `volatile bool` flag set in the ISR, cleared when `getButtonEvent()` is called.

---

## WebSocket Protocol

The device connects to `ws://<serverIP>:<port>/ws` and:

1. Sends `device_register` with MAC, role, lane
2. Receives `start` / `reset` / `event-heat` / `clear` commands
3. Sends `split` times with elapsed ms + wall-clock timestamp
4. Exchanges `ping` / `pong` for latency measurement

**Network Delay Compensation:**
- Starter device sends `start` messages with microsecond NTP timestamps (`timestamp_sec`, `timestamp_usec`)
- Lane devices calculate network delay by comparing starter's timestamp to their own NTP time
- Lane timers backdate their start point by the delay via `startWithOffset()`
- Result: All devices show synchronized elapsed time within ±2-4ms (NTP accuracy limit)

See [API.md](API.md) for full message format.

---

## Battery Monitoring

Battery voltage is read from GPIO4 via `analogRead()` with 16-sample averaging.

The 1:2 resistive voltage divider on the T-Display S3 means:
`actual_voltage = adc_voltage * 2`

Mapped linearly from 3.0 V (0%) to 4.2 V (100%).

Displayed in the sidebar (22px area) as "Bat XX%", red when <= 20%.

---

## NTP Integration

- Server IP defaults to same as WebSocket server (configurable separately)
- SNTP smooth mode prevents time jumps during races
- `NTPManager::isSynced()` returns true after first successful sync
- Sidebar shows "NTP HH:MM:SS" (white when synced, yellow when not)

---

## Testing

### On-Device Testing

```bash
# Upload and monitor
pio run --target upload; pio device monitor
```

### Useful Debug Checks

- **WiFi:** Serial prints connection status and IP
- **NTP:** `DEBUG_LOG` shows sync callbacks
- **WebSocket:** `DEBUG_LOG` shows connect/disconnect/messages
- **Display:** `forceRefresh()` redraws everything
- **Battery:** `DEBUG_LOG` shows ADC raw and voltage

### Memory Monitoring

```cpp
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
Serial.printf("Min free heap: %d bytes\n", ESP.getMinFreeHeap());
```

---

## Troubleshooting

| Problem | Likely cause | Fix |
|---------|-------------|-----|
| No WiFi | Wrong credentials in NVS | Erase flash: `pio run --target erase` |
| NTP not syncing | Server unreachable | Check NTP server IP, verify with `ntpdate -q <ip>` |
| WebSocket disconnect | Wrong server IP/port | Check captive portal config |
| Display blank | TFT_BL wrong | Must be GPIO38 in User_Setup.h |
| Battery always 0% | GPIO4 conflict | Ensure TFT_BL is NOT on GPIO4 |
| Buttons unresponsive | Debounce too long | Adjust `BUTTON_DEBOUNCE_MS` in config.h |
| Timer drift | Expected | ~20 ppm is normal for ESP32 crystal |

---

## Contributing

1. Fork the repository
2. Create a feature branch
3. Follow the code style above
4. Update docs if architecture changes
5. Test on actual hardware
6. Submit a pull request

---

## Resources

- [ESP32-S3 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- [Arduino ESP32 Core](https://github.com/espressif/arduino-esp32)
- [PlatformIO Docs](https://docs.platformio.org/)
- [TFT_eSPI Library](https://github.com/Bodmer/TFT_eSPI)
- [WebSockets Library](https://github.com/Links2004/arduinoWebSockets)
- [ArduinoJson](https://arduinojson.org/)
- [LilyGO T-Display S3](https://lilygo.cc/products/t-display-s3)
