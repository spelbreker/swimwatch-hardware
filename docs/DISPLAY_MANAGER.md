# Display Manager Documentation — SwimWatch

## Overview

The Display Manager renders the SwimWatch interface on the LilyGO T-Display S3's 1.9" ST7789V IPS LCD (320x170, 8-bit parallel). It uses a two-panel layout with dirty-region tracking to minimise flicker and CPU usage.

**Refresh rate:** 50ms (20fps) for the stopwatch area, 1s for status sidebar.

---

## Hardware

| Property | Value |
|----------|-------|
| Display | 1.9" ST7789V IPS LCD |
| Resolution | 320x170 (landscape) |
| Interface | 8-bit parallel via TFT_eSPI |
| Colour depth | 16-bit RGB565 |
| Backlight | GPIO38 (PWM capable, not yet implemented) |

---

## Layout

```
320px
+----------- 240px -----------+------ 80px ------+
|                              |   WiFi Status    | 40px
|   Stopwatch Time Display     |  (bars + RSSI)   |
|        (240 x 80px)         |                   |
|                              +------------------+
+------------------------------+  WebSocket Status | 40px
|                              |   (WS + ping)    |
|     Split Times Area         +------------------+
|  Split 1: MM:SS.cc  (30px)  |   Lane / Role    | 45px
|  Split 2: MM:SS.cc  (30px)  |                   |
|  Split 3: MM:SS.cc  (30px)  +------------------+
|                              |   Battery %      | 22px
|                              +------------------+
|                              |   NTP Clock      | 23px
+------------------------------+------------------+
170px
```

### Coordinates Reference

```cpp
// Main area (left panel)
#define MAIN_AREA_X           0
#define MAIN_AREA_WIDTH       240
#define AREA_STOPWATCH_Y      0      // height 80
#define AREA_LAP1_Y           80     // height 30
#define AREA_LAP2_Y           110    // height 30
#define AREA_LAP3_Y           140    // height 30

// Status area (right panel)
#define STATUS_AREA_X         240
#define STATUS_AREA_WIDTH     80
#define AREA_WIFI_STATUS_Y    0      // height 40
#define AREA_WEBSOCKET_STATUS_Y 40   // height 40
#define AREA_LANE_INFO_Y      80     // height 45
#define AREA_BATTERY_Y        125    // height 22
#define AREA_NTP_CLOCK_Y      147    // height 23
```

---

## Dirty-Region Tracking

Each display area has its own dirty flag and cached string. An area is only redrawn when:
1. Its content has changed (new string differs from cached), or
2. Its dirty flag is set (e.g. after `forceRefresh()` or `clearScreen()`).

```cpp
// Dirty flags
bool stopwatchAreaDirty;
bool wifiAreaDirty;
bool websocketAreaDirty;
bool laneAreaDirty;
bool batteryAreaDirty;
bool ntpClockAreaDirty;
bool lapAreaDirty;

// Cached strings
String lastTimeString;
String lastWiFiStatus;
String lastWebSocketStatus;
String lastLaneInfo;
String lastBatteryStr;
String lastNtpClock;
String lastLap1, lastLap2, lastLap3;
```

---

## Colour Scheme

### Primary Palette (RGB565)

| Constant | Value | Colour | Usage |
|----------|-------|--------|-------|
| `COLOR_BACKGROUND` | `TFT_BLACK` | Black | Main area background |
| `COLOR_TIME_DISPLAY` | `TFT_GREEN` | Green | Running timer |
| `COLOR_WARNING` | `TFT_ORANGE` | Orange | Stopped timer |
| `COLOR_LAP_INFO` | `TFT_YELLOW` | Yellow | Split times |
| `COLOR_STATUS` | `TFT_CYAN` | Cyan | General status |
| `COLOR_ERROR` | `TFT_RED` | Red | Errors, low battery |
| `COLOR_SIDEBAR_BG` | `0x049D` | Pool blue | Sidebar background |

### WiFi Signal Bars

| Signal | Bars | Colour |
|--------|------|--------|
| > -50 dBm (excellent) | 4 | Green |
| -50 to -60 dBm (good) | 3 | Green |
| -60 to -70 dBm (fair) | 2 | Yellow |
| -70 to -80 dBm (poor) | 1 | Red |
| < -80 dBm | 0 | -- |

---

## Fonts

| Area | Font | Size |
|------|------|------|
| Stopwatch time | Font 6 | Large |
| Split times | Font 2 | Medium |
| Status text / sidebar labels | Font 1 | Small |
| Lane number | Font 2 | Medium |

---

## API Reference

### Initialisation

```cpp
DisplayManager();          // Constructor, marks all areas dirty
bool init();               // Init TFT, set rotation to landscape, clear screen
void setRotation(uint8_t); // 0-3 (default 1 = landscape)
void setBrightness(uint8_t); // TODO: PWM on GPIO38
```

### Screen Management

```cpp
void clearScreen();        // Fill black, redraw sidebar, reset all caches
void showSplashScreen();   // "SwimWatch" + "T-Display S3" + "Initializing..."
void forceRefresh();       // Set all dirty flags
bool needsUpdate();        // True if any dirty flag is set
```

### Stopwatch Display (left panel, 240x80)

```cpp
void updateStopwatchDisplay(uint32_t elapsedMs, bool isRunning = false);
```
- Running: green `MM:SS.d` (1-digit deciseconds)
- Stopped: orange `MM:SS.cc` (2-digit centiseconds)

```cpp
void showStartupMessage(const String& message);
void clearStartupMessage();
void setEventHeat(const String& event, const String& heat);
```

### Split Times (left panel, 3 x 30px rows)

```cpp
void updateLapTime(uint8_t lapNumber, const String& time); // 1-3
void clearLapTimes();
```

### WiFi Status (sidebar, 80x40)

```cpp
void updateWiFiStatus(const String& status, bool isConnected = false, int rssi = 0);
```
Draws 4-bar signal strength indicator + "WiFi" label + RSSI value.

### WebSocket Status (sidebar, 80x40)

```cpp
void updateWebSocketStatus(const String& status, bool isConnected = false, int pingMs = -1);
```
Shows "WS" + ping time (ms) when connected, or "Disconnected" in red.

### Lane / Role Info (sidebar, 80x45)

```cpp
void updateLaneInfo(uint8_t laneNumber);
void updateRoleInfo(const String& role, const String& event, const String& heat, uint8_t laneNumber);
```

### Battery Display (sidebar, 80x22)

```cpp
void updateBatteryDisplay(uint8_t percentage);
```
Shows "Bat" label + percentage. Red when <= 20%.

Battery is read from GPIO4 ADC with 1:2 voltage divider, 16-sample averaging, mapped from 3.0V-4.2V to 0-100%.

### NTP Clock (sidebar, 80x23)

```cpp
void updateNtpClock(const String& timeString, bool isSynced = true);
```
Shows "NTP" label + `HH:MM:SS`. Yellow when not synced.

### Utility

```cpp
void showGeneralStatus(const String& message, uint16_t color = COLOR_STATUS);
void showConfigPortalInfo(const String& apName, const String& apPassword);
String formatStopwatchTime(uint32_t milliseconds, bool isRunning = true);
void drawBorders();
void clearStatusAreas();
void sendTFTCommand(uint8_t cmd); // Raw TFT command (e.g. sleep)
```

---

## Update Strategy

| Area | Frequency | Trigger |
|------|-----------|---------|
| Stopwatch | 20 Hz (50ms) | `DISPLAY_UPDATE_INTERVAL_MS` in main loop |
| WiFi / WS / Battery / NTP | 1 Hz (1s) | `STATUS_UPDATE_INTERVAL_MS` in `checkConnections()` |
| Split times | Event-driven | When new split is recorded |
| Lane / Role | Event-driven | On WebSocket config message |

---

## Usage Example

```cpp
DisplayManager display;

void setup() {
    display.init();
    display.showSplashScreen();
    display.updateLaneInfo(9);
}

void loop() {
    static uint32_t lastDisplay = 0;
    static uint32_t lastStatus = 0;
    uint32_t now = millis();

    if (now - lastDisplay >= DISPLAY_UPDATE_INTERVAL_MS) {
        display.updateStopwatchDisplay(timer.getElapsedMs(), timer.isRunning());
        lastDisplay = now;
    }

    if (now - lastStatus >= STATUS_UPDATE_INTERVAL_MS) {
        display.updateWiFiStatus("Connected", true, WiFi.RSSI());
        display.updateBatteryDisplay(batteryPercent);
        if (ntpManager.isSynced()) {
            char buf[9];
            ntpManager.getFormattedTime(buf, sizeof(buf));
            display.updateNtpClock(String(buf), true);
        }
        lastStatus = now;
    }
}
```

---

## Troubleshooting

| Problem | Cause | Fix |
|---------|-------|-----|
| Flicker | Full-screen redraws | Ensure dirty flags work; only changed areas redraw |
| Garbled text | Wrong font/coordinates | Check layout constants match header |
| No backlight | TFT_BL pin wrong | Must be GPIO38 (not GPIO4) in `User_Setup.h` |
| Slow updates | Too-frequent full refresh | Use `needsUpdate()` guard, respect intervals |

---

## Related Docs

- [API Reference](API.md) -- Full module API
- [Hardware](HARDWARE.md) -- Pin assignments, power, wiring
- [Developer Guide](DEVELOPER.md) -- Architecture, build, contributing
