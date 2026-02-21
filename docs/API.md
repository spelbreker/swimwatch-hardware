# API Reference - SwimWatch

## Overview

SwimWatch is a high-precision swim meet split timer running on the LilyGO T-Display S3. This document covers the public API of every module in the system.

**Modules:**

| Module | Header | Purpose |
|--------|--------|---------|
| Config | `config.h` | Pin defs, constants, `StopwatchConfig` struct, `DEBUG_LOG` |
| StopwatchTimer | `stopwatch_timer.h` | `esp_timer`-based elapsed timing (1us resolution) |
| NTPManager | `ntp_manager.h` | SNTP sync (60s), wall-clock timestamps |
| WebSocketStopwatch | `websocket_stopwatch.h` | Race protocol (start/split/stop/event-heat) |
| DisplayManager | `display_manager.h` | TFT rendering, dirty regions, sidebar |
| ButtonManager | `button_manager.h` | ISR + debounce for GPIO0/GPIO14/GPIO2 |
| CaptivePortalManager | `captive_portal.h` | AP-mode config, NVS Preferences save/load |

---

## Configuration (`config.h`)

### Debug Macro

```cpp
// Enabled via -DDEBUG_STOPWATCH in platformio.ini build_flags
DEBUG_LOG(fmt, ...)   // printf-style, outputs "[DBG] ..." to Serial
```

### Hardware Pin Definitions

```cpp
constexpr uint8_t PIN_BUTTON_START_STOP = 0;    // GPIO0  - onboard BUTTON1
constexpr uint8_t PIN_BUTTON_RESET      = 14;   // GPIO14 - onboard BUTTON2
constexpr uint8_t PIN_BUTTON_SPLIT      = 2;    // GPIO2  - external split trigger
constexpr uint8_t PIN_POWER_ON          = 15;   // Must be HIGH for battery operation
constexpr uint8_t PIN_BATTERY_ADC       = 4;    // GPIO4  - battery voltage (1:2 divider)
```

Display pins are managed by TFT_eSPI via `User_Setup.h`:
`TFT_MOSI=19, TFT_SCLK=18, TFT_CS=5, TFT_DC=16, TFT_RST=23, TFT_BL=38`

### Battery Constants

```cpp
constexpr float   BATTERY_MAX_VOLTAGE = 4.2f;   // Fully charged LiPo
constexpr float   BATTERY_MIN_VOLTAGE = 3.0f;   // Empty LiPo
constexpr uint8_t BATTERY_SAMPLES     = 16;      // ADC averaging
```

### Timing Constants

```cpp
constexpr uint32_t DISPLAY_UPDATE_INTERVAL_MS   = 50;     // 50ms = 20fps
constexpr uint32_t STATUS_UPDATE_INTERVAL_MS    = 1000;   // Status refresh 1Hz
constexpr uint32_t BUTTON_DEBOUNCE_MS           = 200;    // Software debounce
constexpr uint32_t NTP_SYNC_INTERVAL_MS         = 60000;  // NTP re-sync 60s
constexpr uint32_t NTP_INITIAL_SYNC_TIMEOUT_MS  = 5000;   // Max wait for first sync
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS      = 10000;  // WiFi connect timeout
```

### Network Defaults

```cpp
constexpr const char* DEFAULT_SERVER_IP   = "192.168.8.10";
constexpr uint16_t    DEFAULT_SERVER_PORT = 80;
constexpr const char* DEFAULT_WS_PATH    = "/ws";
```

### Captive Portal Defaults

```cpp
constexpr const char* AP_SSID     = "SwimWatch-Setup";
constexpr const char* AP_PASSWORD = "swimwatch123";
```

### StopwatchConfig Struct

Loaded from NVS Preferences, configured via captive portal.

```cpp
struct StopwatchConfig {
    String   wifiSSID;
    String   wifiPassword;
    String   serverIP    = "192.168.8.10";
    uint16_t serverPort  = 80;
    String   ntpServer;                        // Empty = use serverIP
    uint8_t  laneNumber  = 9;
    String   role        = "lane";             // "lane" or "starter"
    bool     useSSL      = false;

    const char* getEffectiveNTPServer() const; // Falls back to serverIP
};
```

---

## StopwatchTimer

Precision elapsed timer based on `esp_timer_get_time()` (64-bit hardware counter, 1us resolution). Independent of NTP -- not affected by time sync adjustments.

```cpp
class StopwatchTimer {
public:
    void     start();
    void     startWithOffset(int64_t offsetUs);  // Start backdated by offset (network compensation)
    void     stop();
    void     reset();
    void     addSplit(uint8_t lane = 0);         // Record a split time
    uint32_t getElapsedMs() const;               // Elapsed in milliseconds
    uint64_t getElapsedUs() const;               // Elapsed in microseconds (internal use)
    bool     isRunning() const;
    const std::vector<SplitTime>& getSplits() const;
    
    // Static formatter
    static void formatMs(uint32_t ms, bool isRunning, char* buffer, size_t bufferSize);
};

// Split time record
struct SplitTime {
    uint32_t elapsedMs;    // Elapsed ms since start (from hardware timer)
    time_t   wallClock;    // Absolute UTC timestamp (from NTP-synced RTC)
    uint8_t  lane;         // Lane number (0 = local button)
};
```

---

## NTPManager

SNTP sync in smooth mode (no jumps) via FreeRTOS background task.

```cpp
class NTPManager {
public:
    void    begin(const char* ntpServer);
    bool    isSynced() const;
    bool    waitForSync(uint32_t timeoutMs);   // Blocking - startup only
    time_t  getEpochTime() const;
    void    getFormattedTime(char* buffer, size_t bufferSize) const; // "HH:MM:SS"
};
```

**Notes:**
- Sync interval: 60s via SNTP (configured by `NTP_SYNC_INTERVAL_MS`)
- LAN accuracy: +/-1-2ms per sync
- Does **not** affect `esp_timer_get_time()` -- elapsed stopwatch timing is independent

---

## WebSocketStopwatch

Manages the WebSocket connection and race protocol. Wraps a `StopwatchTimer` for elapsed timing and uses NTP-synced `time()` for wall-clock timestamps.

### Configuration

```cpp
void setServerConfig(const String& host, uint16_t port,
                     const String& path = "/ws", bool ssl = false);
void setLaneNumber(uint8_t lane);
void setDeviceRole(const String& role);   // "lane" or "starter"
```

### Connection

```cpp
bool connect();
void disconnect();
bool isConnected();
void loop();             // Must be called every iteration of loop()
```

### Stopwatch Control

```cpp
void start();
void stop();
void reset();
void addLap();
void sendStart(const String& event, const String& heat);  // Starter role only
```

### State Queries

```cpp
StopwatchState getState();       // STOPWATCH_STOPPED / RUNNING / PAUSED
uint32_t       getElapsedTime(); // Elapsed ms
uint8_t        getLapCount();
String         getCurrentEvent();
String         getCurrentHeat();
int            getPingMs();
const SplitTimeInfo* getSplitTimes();  // Array[MAX_LANES]
```

### Display Helpers

```cpp
void   clearSplitTimes();
void   clearDisplay();
String formatTime(uint32_t milliseconds);  // "MM:SS.cc"
```

### Remote Control (via Server)

```cpp
void handleRemoteStart(int64_t timestampSec, int64_t timestampUsec);
void handleRemoteReset();
```

**Network Delay Compensation:**
- `handleRemoteStart` receives NTP timestamps from the starter device
- Calculates network delay by comparing starter's timestamp to local NTP time
- Calls `timer.startWithOffset(delayUs)` to backdate the start point
- Result: All devices show synchronized elapsed time within ±2-4ms

### Callbacks (set by `main.cpp`)

```cpp
void (*onStateChanged)(StopwatchState newState);
void (*onLapAdded)(uint8_t lapNumber, uint32_t lapTime, uint32_t totalTime);
void (*onConnectionChanged)(bool connected);
void (*onEventHeatChanged)(const String& event, const String& heat);
void (*onSplitTimeReceived)(uint8_t lane, const String& time);
void (*onDisplayClear)();
void (*onDeviceConfigChanged)(const String& role, uint8_t lane);
```

### StopwatchState Enumeration

```cpp
enum StopwatchState {
    STOPWATCH_STOPPED,
    STOPWATCH_RUNNING,
    STOPWATCH_PAUSED
};
```

### WebSocket Message Format

#### Received from Server

```json
{ "type": "start", "event": "100m Free", "heat": "3", "timestamp_sec": 1234567890, "timestamp_usec": 123456 }
{ "type": "reset" }
{ "type": "event-heat", "event": "100m Free", "heat": "3" }
{ "type": "clear" }
{ "type": "device_update_role", "role": "starter" }
{ "type": "device_update_lane", "lane": 5 }
```

**Notes:**
- `start` message includes microsecond-precision NTP timestamp for network delay compensation
- `timestamp_sec`: Unix epoch seconds (from `gettimeofday()`)
- `timestamp_usec`: Microseconds component (0-999999)

#### Sent to Server

```json
{ "type": "device_register", "mac": "AA:BB:CC:DD:EE:FF", "role": "lane", "lane": 9 }
{ "type": "split", "lane": 9, "time-ms": 31250, "time": "00:31.25", "timestamp": 1234567890 }
{ "type": "pong" }
```

---

## DisplayManager

Two-panel TFT display (320x170 landscape) with dirty-region tracking. Swimming pool blue (#0092b8) sidebar theme.

### Layout Constants

```
Left Panel (0-240px)          Right Panel (240-320px, 80px wide)
+---------------------+       +--------------+
|  Stopwatch (80px)   |       | WiFi (40px)  |
+---------------------+       +--------------+
|  Split 1 (30px)     |       | WS (40px)    |
|  Split 2 (30px)     |       +--------------+
|  Split 3 (30px)     |       | Lane (45px)  |
|                     |       +--------------+
|                     |       | Battery(22px)|
|                     |       +--------------+
|                     |       | NTP (23px)   |
+---------------------+       +--------------+
```

```cpp
#define DISPLAY_WIDTH  320
#define DISPLAY_HEIGHT 170
#define MAIN_AREA_WIDTH       240
#define STATUS_AREA_WIDTH     80
#define STATUS_AREA_X         240

// Sidebar area Y positions and heights
#define AREA_WIFI_STATUS_Y       0     // 40px
#define AREA_WEBSOCKET_STATUS_Y  40    // 40px
#define AREA_LANE_INFO_Y         80    // 45px
#define AREA_BATTERY_Y           125   // 22px
#define AREA_NTP_CLOCK_Y         147   // 23px
```

### Initialization

```cpp
DisplayManager();
bool init();
void setRotation(uint8_t rotation);     // 0-3
void setBrightness(uint8_t brightness); // TODO: PWM on GPIO38
```

### Screen Management

```cpp
void clearScreen();
void showSplashScreen();
void forceRefresh();
bool needsUpdate();
```

### Stopwatch Display

```cpp
void updateStopwatchDisplay(uint32_t elapsedMs, bool isRunning = false);
void showStartupMessage(const String& message);
void clearStartupMessage();
void setEventHeat(const String& event, const String& heat);
```

**Time Format:**
- Running: `MM:SS.d` (1-digit deciseconds)
- Stopped: `MM:SS.cc` (2-digit centiseconds)

### Split Times

```cpp
void updateLapTime(uint8_t lapNumber, const String& time);  // lapNumber 1-3
void clearLapTimes();
```

### Status Sidebar

```cpp
void updateWiFiStatus(const String& status, bool isConnected = false, int rssi = 0);
void updateWebSocketStatus(const String& status, bool isConnected = false, int pingMs = -1);
void updateLaneInfo(uint8_t laneNumber);
void updateRoleInfo(const String& role, const String& event, const String& heat, uint8_t laneNumber);
void updateNtpClock(const String& timeString, bool isSynced = true);
void updateBatteryDisplay(uint8_t percentage);
```

### Utility

```cpp
void showGeneralStatus(const String& message, uint16_t color = COLOR_STATUS);
void showConfigPortalInfo(const String& apName, const String& apPassword);
String formatStopwatchTime(uint32_t milliseconds, bool isRunning = true);
void drawBorders();
void clearStatusAreas();
void sendTFTCommand(uint8_t cmd);  // Raw TFT command (sleep/off)
```

### Color Definitions (RGB565)

```cpp
#define COLOR_BACKGROUND   TFT_BLACK
#define COLOR_TIME_DISPLAY TFT_GREEN
#define COLOR_LAP_INFO     TFT_YELLOW
#define COLOR_STATUS       TFT_CYAN
#define COLOR_ERROR        TFT_RED
#define COLOR_WARNING      TFT_ORANGE
#define COLOR_SIDEBAR_BG   0x049D    // Swimming pool blue (#0092b8)
```

---

## ButtonManager

ISR-driven button handler with 200ms software debounce.

### ButtonEvent Enumeration

```cpp
enum ButtonEvent {
    BUTTON_NONE,
    BUTTON_START_STOP,   // GPIO0  - toggle start/stop
    BUTTON_RESET,        // GPIO14 - reset (only when stopped)
    BUTTON_LAP_PRESSED   // GPIO2  - split / starter send
};
```

### API

```cpp
class ButtonManager {
public:
    ButtonManager();
    bool init();                    // Configure GPIOs, attach ISRs
    ButtonEvent getButtonEvent();   // Returns next pending event
    void clearEvents();             // Discard all pending events
};
```

**Button Hardware:**
- GPIO0/GPIO14: Onboard buttons, active LOW, internal pullup
- GPIO2: External split trigger, active LOW

---

## CaptivePortalManager

WiFi + NTP server configuration via captive portal AP (`SwimWatch-Setup` / `swimwatch123`). Settings stored in ESP32 NVS Preferences (namespace `"stopwatch"`).

### API

```cpp
class CaptivePortalManager {
public:
    CaptivePortalManager();
    ~CaptivePortalManager();

    bool begin();            // Start AP + DNS + web server
    void loop();             // Handle clients (call in loop)
    void stop();             // Shut down AP
    bool isConfigComplete() const;

    // Retrieve user-entered values
    String getConfiguredSSID() const;
    String getConfiguredPassword() const;
    String getConfiguredWsServer() const;
    String getConfiguredWsPort() const;
    String getConfiguredLane() const;
    String getConfiguredRole() const;

    void saveConfiguration();

    // Static helpers
    static bool hasStoredCredentials();
    static bool connectWithStoredCredentials();
};
```

### Portal Fields

| Field | Default | Purpose |
|-------|---------|---------|
| WiFi SSID | *(stored)* | Network name |
| WiFi Password | *(stored)* | Network password |
| Server IP | `192.168.8.10` | WebSocket server |
| Server Port | `80` | WebSocket port |
| NTP Server | *(empty = server IP)* | Dedicated NTP server |
| Lane Number | `9` | Lane assignment |
| Role | `lane` | `"lane"` or `"starter"` |

---

## Build Configuration

### PlatformIO

```ini
[env:lilygo-t-display-s3]
platform = espressif32
board    = lilygo-t-display-s3
framework = arduino

lib_deps =
    links2004/WebSockets @ ^2.4.1
    bblanchon/ArduinoJson @ ^6.21.3

; Add -DDEBUG_STOPWATCH for debug output
build_flags = -DDEBUG_STOPWATCH
```

### Memory Usage (typical)

```
RAM:   ~15% (49KB / 328KB)
Flash: ~15% (973KB / 6.5MB)
```

---

## Resources

- [ESP32-S3 Arduino Core](https://docs.espressif.com/projects/arduino-esp32/en/latest/)
- [TFT_eSPI Library](https://github.com/Bodmer/TFT_eSPI)
- [WebSockets Library](https://github.com/Links2004/arduinoWebSockets)
- [ArduinoJson](https://arduinojson.org/)
- [LilyGO T-Display S3](https://lilygo.cc/products/t-display-s3)
