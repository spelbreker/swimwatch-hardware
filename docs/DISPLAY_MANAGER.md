# Display Manager Documentation

## Overview

The Display Manager is the core component responsible for rendering and updating the visual interface of the LilyGO T-Display S3 swimming stopwatch. It implements a clean, two-panel layout optimized for swimming competition timing with efficient performance and a swimming pool themed design.

## Architecture

### Display Hardware Specifications

- **Device:** LilyGO T-Display S3 with ST7789V 1.9" IPS TFT
- **Resolution:** 320×170 pixels (landscape orientation)
- **Interface:** 8-bit parallel interface via TFT_eSPI library
- **Color Depth:** 16-bit RGB565 color space
- **Refresh Rate:** Up to 10Hz for smooth timer updates

### Two-Panel Layout Design

The display is divided into two main areas to maximize readability and information density:

```
┌─────────────────────────────────────────┬─────────────────────┐
│                                         │     WiFi Status     │
│          Stopwatch Time Display         │    (Signal Bars)    │
│             (240x80 area)               │      + RSSI dBm     │
│                                         ├─────────────────────┤
├─────────────────────────────────────────┤   WebSocket Status  │
│           Split Times Area              │      WS + Ping      │
│   Split - 1: xx:xx:xx (if available)    │                     │
│   Split - 2: xx:xx:xx (if available)    ├─────────────────────┤
│   Split - 3: xx:xx:xx (if available)    │    Lane Number      │
│             (240x90 area)               │      Lane X         │
│                                         ├─────────────────────┤
│                                         │   Battery Status    │
│                                         │    Battery XX%      │
└─────────────────────────────────────────┴─────────────────────┘
```

## Layout Specifications

### Main Area (Left Panel - 240×170px)

The main area occupies 75% of the screen width and contains the primary timing information.

#### Stopwatch Display Area (240×80px)
- **Position:** (0, 0) to (240, 80)
- **Purpose:** Primary elapsed time display
- **Format:** 
  - Running: `MM:SS:M` (single digit milliseconds, 100ms precision)
  - Stopped: `MM:SS:MM` (centiseconds precision for final times)
- **Font:** Large (Font 6) for maximum visibility
- **Colors:** 
  - Green (`TFT_GREEN`) when running
  - Orange (`TFT_ORANGE`) when stopped/paused

#### Split Times Area (240×90px)
- **Position:** (0, 80) to (240, 170)
- **Purpose:** Display last 3 split times in rolling fashion
- **Layout:** Three rows of 30px height each
  - Row 1: (0, 80) to (240, 110)
  - Row 2: (0, 110) to (240, 140)  
  - Row 3: (0, 140) to (240, 170)
- **Format:** `Split - X: HH:MM:SS` where X is the actual split number
- **Font:** Medium (Font 2)
- **Color:** Yellow (`TFT_YELLOW`) for high visibility
- **Behavior:** Only shows actual recorded splits, no static placeholder text

### Status Area (Right Panel - 80×170px)

The status area occupies 25% of the screen width and displays system information with a swimming pool themed background.

#### WiFi Status Section (80×40px)
- **Position:** (240, 0) to (320, 40)
- **Content:** Signal strength visualization + connection status
- **Visual Elements:**
  - 4-bar signal strength indicator with color coding
  - "WiFi" text label
  - RSSI value in dBm
- **Signal Quality Mapping:**
  - **Excellent (>-50 dBm):** 4 green bars
  - **Good (-50 to -60 dBm):** 3 green bars
  - **Fair (-60 to -70 dBm):** 2 yellow bars  
  - **Poor (-70 to -80 dBm):** 1 red bar
  - **No signal (<-80 dBm):** 0 bars

#### WebSocket Status Section (80×40px)
- **Position:** (240, 40) to (320, 80)
- **Content:** Server connection status and latency
- **Format:**
  - Connected: `WS\nXXms` (ping time)
  - Connected (no ping): `WS\nOK`
  - Disconnected: `WS\nDisconnected`
- **Colors:**
  - White when connected
  - Red when disconnected or error

#### Lane Information Section (80×45px)
- **Position:** (240, 80) to (320, 125)
- **Content:** Current lane assignment
- **Format:** `Lane\nX` where X is the lane number
- **Font:** Medium (Font 2)
- **Color:** White on swimming pool background

#### Battery Status Section (80×45px)
- **Position:** (240, 125) to (320, 170)
- **Content:** Battery percentage and voltage
- **Format:** `Battery\nXX%`
- **Font:** Medium (Font 2) for visibility
- **Colors:**
  - White for normal levels (>20%)
  - Red for low battery (<20%)

## Color Scheme

### Primary Color Palette

The interface uses a swimming-themed color scheme optimized for competitive environments:

```cpp
// Core interface colors (RGB565 format)
#define COLOR_BACKGROUND     TFT_BLACK      // High contrast background
#define COLOR_TIME_DISPLAY   TFT_GREEN      // Active timer color
#define COLOR_TIME_STOPPED   TFT_ORANGE     // Stopped timer color
#define COLOR_LAP_INFO       TFT_YELLOW     // Split time visibility
#define COLOR_STATUS         TFT_CYAN       // General status text
#define COLOR_ERROR          TFT_RED        // Error indications
#define COLOR_WARNING        TFT_ORANGE     // Warning states

// Swimming pool themed elements
#define COLOR_SIDEBAR_BG     0x06FF         // Swimming pool blue-green

// WiFi signal strength indicators
#define COLOR_WIFI_BAR_STRONG TFT_GREEN     // Excellent signal
#define COLOR_WIFI_BAR_GOOD   TFT_YELLOW    // Fair signal  
#define COLOR_WIFI_BAR_WEAK   TFT_RED       // Poor signal
```

### Color Usage Guidelines

- **High Contrast:** Black background ensures maximum readability
- **State Indication:** Colors communicate system status at a glance
- **Swimming Theme:** Blue-green sidebar evokes pool environment
- **Accessibility:** Color choices tested for visibility in various lighting

## Technical Implementation

### Performance Optimization

#### Dirty Region System
The display manager implements an efficient dirty region tracking system to minimize unnecessary screen updates:

```cpp
// Dirty flags for each display area
bool stopwatchAreaDirty;    // Main timer display
bool wifiAreaDirty;         // WiFi status section
bool websocketAreaDirty;    // WebSocket status section  
bool laneAreaDirty;         // Lane information
bool batteryAreaDirty;      // Battery status
bool lapAreaDirty;          // Split times area
```

Benefits:
- Reduces screen flicker
- Improves battery life
- Maintains smooth 10Hz timer updates
- Prevents unnecessary CPU usage

#### String Caching
Display strings are cached to prevent redundant drawing operations:

```cpp
// Cached display strings for change detection
String lastTimeString;      // Last displayed time
String lastWiFiStatus;      // Last WiFi status text
String lastWebSocketStatus; // Last WebSocket status
String lastLaneInfo;        // Last lane information
String lastBatteryString;   // Last battery status
String lastLap1, lastLap2, lastLap3; // Split times
```

### Update Frequency Strategy

Different display areas update at optimal frequencies based on their information type:

- **Stopwatch Display:** 10Hz (100ms intervals) for smooth timing
- **Status Areas:** 1Hz (1000ms intervals) for connection monitoring
- **Split Times:** Event-driven updates when new splits are recorded
- **Battery Status:** Every few seconds for power monitoring

### Memory Management

#### Efficient String Operations
- Minimize dynamic string allocation during updates
- Use cached comparisons to avoid unnecessary redraws
- Implement smart area clearing to reduce memory fragmentation

#### Display Buffer Optimization
- Clear only specific areas rather than full screen refreshes
- Batch related updates to minimize TFT communication overhead
- Use appropriate font loading strategies based on display area

## API Reference

### Core Display Functions

#### Initialization and Setup

```cpp
// Initialize display hardware and set up layout
bool init();

// Set display orientation (0-3, landscape/portrait)
void setRotation(uint8_t rotation);

// Control screen brightness (future implementation)
void setBrightness(uint8_t brightness);

// Clear entire screen and reset to default state
void clearScreen();

// Show startup splash screen with branding
void showSplashScreen();
```

#### Main Timer Display

```cpp
// Update the primary stopwatch display
void updateStopwatchDisplay(uint32_t elapsedMs, bool isRunning = false);

// Show startup/status messages in main area
void showStartupMessage(const String& message);

// Clear startup messages
void clearStartupMessage();

// Legacy compatibility - show "00:00:00"
void showTimeZero();
```

#### Split Time Management

```cpp
// Update individual split time display (1-3)
void updateLapTime(uint8_t lapNumber, const String& time);

// Clear all split time displays
void clearLapTimes();

// Legacy array-based lap display (backward compatibility)
void updateLapDisplay(const uint8_t* laps, uint8_t lapCount, uint8_t maxDisplay = 5);
```

#### Status Area Updates

```cpp
// Update WiFi status with signal strength visualization
void updateWiFiStatus(const String& status, bool isConnected = false, int rssi = 0);

// Update WebSocket connection status with ping time
void updateWebSocketStatus(const String& status, bool isConnected = false, int pingMs = -1);

// Update lane number display
void updateLaneInfo(uint8_t laneNumber);

// Update battery status with percentage
void updateBatteryDisplay(float voltage, uint8_t percentage);
```

#### Layout and Utility Functions

```cpp
// Draw optional border lines between areas
void drawBorders();

// Clear all status areas and reset sidebar
void clearStatusAreas();

// Force complete refresh of all areas
void forceRefresh();

// Check if any area needs updating
bool needsUpdate();
```

### Helper Functions

#### Time Formatting

```cpp
// Format time for stopwatch display with running/stopped precision
String formatStopwatchTime(uint32_t milliseconds, bool isRunning = true);

// General time formatting with centisecond option
String formatTimeDisplay(uint32_t milliseconds, bool showCentiseconds = true);
```

#### Area Management

```cpp
// Clear specific rectangular area
void clearArea(int16_t x, int16_t y, int16_t w, int16_t h);

// Draw swimming pool themed sidebar background
void drawSidebarBackground();

// Draw WiFi signal strength bars with RSSI-based coloring
void drawWiFiStrengthBars(int rssi, int x, int y, int width, int height);
```

## Usage Examples

### Basic Initialization

```cpp
DisplayManager display;

void setup() {
    // Initialize display system
    if (!display.init()) {
        Serial.println("Display initialization failed!");
        return;
    }
    
    // Show splash screen
    display.showSplashScreen();
    
    // Setup initial layout
    display.updateLaneInfo(9);                    // Set lane number
    display.updateWiFiStatus("Connected", true, -55); // Show WiFi status
    display.updateStopwatchDisplay(0, false);     // Show 00:00:00
}
```

### Timer Operation

```cpp
void loop() {
    static uint32_t lastUpdate = 0;
    uint32_t now = millis();
    
    // Update timer display every 100ms
    if (now - lastUpdate >= 100) {
        uint32_t elapsed = stopwatch.getElapsedTime();
        bool running = stopwatch.isRunning();
        
        display.updateStopwatchDisplay(elapsed, running);
        lastUpdate = now;
    }
    
    // Update status areas every second
    static uint32_t lastStatusUpdate = 0;
    if (now - lastStatusUpdate >= 1000) {
        updateStatusAreas();
        lastStatusUpdate = now;
    }
}
```

### Split Time Recording

```cpp
void onSplitRecorded(uint8_t splitNumber, const String& timeString) {
    // Update display with new split time
    // Split times are shown in rolling fashion (last 3)
    
    if (splitNumber <= 3) {
        display.updateLapTime(splitNumber, "Split - " + String(splitNumber) + ": " + timeString);
    } else {
        // Shift older splits up and add new one
        // This is handled automatically by the main application logic
    }
}
```

### Status Updates

```cpp
void updateStatusAreas() {
    // WiFi status with signal strength
    if (WiFi.status() == WL_CONNECTED) {
        int rssi = WiFi.RSSI();
        display.updateWiFiStatus("Connected", true, rssi);
    } else {
        display.updateWiFiStatus("Disconnected", false);
    }
    
    // WebSocket status with ping time
    if (webSocket.isConnected()) {
        int ping = webSocket.getPingMs();
        display.updateWebSocketStatus("Connected", true, ping);
    } else {
        display.updateWebSocketStatus("Disconnected", false);
    }
    
    // Battery status (implement actual ADC reading)
    float voltage = getBatteryVoltage();  // Implement this function
    uint8_t percentage = calculateBatteryPercentage(voltage);
    display.updateBatteryDisplay(voltage, percentage);
}
```

## Configuration Constants

### Layout Dimensions

```cpp
// Display hardware constants
#define DISPLAY_WIDTH  320
#define DISPLAY_HEIGHT 170

// Main area (left side - 75% width)
#define MAIN_AREA_X      0
#define MAIN_AREA_WIDTH  240

// Status area (right side - 25% width) 
#define STATUS_AREA_X      240
#define STATUS_AREA_WIDTH  80

// Individual area heights and positions
#define AREA_STOPWATCH_Y       0
#define AREA_STOPWATCH_HEIGHT  80

#define AREA_LAP1_Y        80
#define AREA_LAP2_Y        110
#define AREA_LAP3_Y        140
#define AREA_LAP_HEIGHT    30

#define AREA_WIFI_STATUS_Y         0
#define AREA_WIFI_STATUS_HEIGHT    40
#define AREA_WEBSOCKET_STATUS_Y    40
#define AREA_WEBSOCKET_STATUS_HEIGHT 40
#define AREA_LANE_INFO_Y           80
#define AREA_LANE_INFO_HEIGHT      45
#define AREA_BATTERY_STATUS_Y      125
#define AREA_BATTERY_STATUS_HEIGHT 45
```

### Font Configuration

```cpp
// Font assignments for different display areas
#define FONT_STOPWATCH 6    // Large font for main timer (maximum visibility)
#define FONT_STATUS    1    // Small font for status information
#define FONT_LAP       2    // Medium font for split times
#define FONT_LANE      2    // Medium font for lane number
```

## Integration Guidelines

### Button Manager Integration

The display responds to hardware button events and provides visual feedback:

```cpp
void onButtonPressed(ButtonType button) {
    switch(button) {
        case BUTTON_START:
            // Visual feedback could be added here
            break;
        case BUTTON_SPLIT:
            // Split time will be shown via callback
            break;
        case BUTTON_RESET:
            display.clearLapTimes();
            display.updateStopwatchDisplay(0, false);
            break;
    }
}
```

### WebSocket Integration

Real-time server communication status is visualized in the sidebar:

```cpp
void onWebSocketEvent(webSocketEvent_t type, uint8_t* payload, size_t length) {
    switch(type) {
        case WStype_CONNECTED:
            display.updateWebSocketStatus("Connected", true);
            break;
        case WStype_DISCONNECTED:
            display.updateWebSocketStatus("Disconnected", false);
            break;
        case WStype_PONG:
            // Update with ping time when pong received
            int pingMs = calculatePingTime();
            display.updateWebSocketStatus("Connected", true, pingMs);
            break;
    }
}
```

### Connectivity Integration

WiFi connection quality is visualized with colored signal bars:

```cpp
void monitorWiFiConnection() {
    if (WiFi.status() == WL_CONNECTED) {
        int rssi = WiFi.RSSI();
        display.updateWiFiStatus("Connected", true, rssi);
        
        // RSSI interpretation:
        // > -50 dBm: Excellent (4 green bars)
        // -50 to -60: Good (3 green bars)  
        // -60 to -70: Fair (2 yellow bars)
        // -70 to -80: Poor (1 red bar)
        // < -80 dBm: No signal (0 bars)
    } else {
        display.updateWiFiStatus("Disconnected", false);
    }
}
```

## Performance Guidelines

### Update Frequency Recommendations

- **Critical Timing:** Update stopwatch display every 100ms for smooth operation
- **Status Information:** Update connection status every 1-2 seconds
- **Battery Monitoring:** Update battery status every 5-10 seconds
- **Split Times:** Update immediately when new splits are recorded

### Memory Optimization

- Use string caching to prevent unnecessary allocations
- Clear only changed areas rather than full screen refreshes
- Minimize string operations during high-frequency updates
- Monitor memory usage during extended operation

### Power Efficiency

- Implement brightness control for outdoor/indoor use
- Use efficient drawing operations to reduce CPU usage
- Consider sleep modes for display during inactivity
- Optimize update frequencies based on information criticality

## Troubleshooting

### Common Issues

#### Display Flicker
- **Cause:** Excessive full-screen refreshes
- **Solution:** Ensure dirty flag system is working correctly
- **Check:** Verify only changed areas are being redrawn

#### Missing or Garbled Text
- **Cause:** Incorrect font selection or area coordinates
- **Solution:** Verify layout constants and font assignments
- **Check:** Ensure text fits within defined area boundaries

#### Performance Issues
- **Cause:** Inefficient string operations or update frequency
- **Solution:** Profile update calls and optimize caching
- **Check:** Monitor string allocation patterns

#### Color Problems
- **Cause:** Incorrect RGB565 color definitions
- **Solution:** Verify color constants match expected values
- **Check:** Test colors on actual hardware under different lighting

### Debugging Functions

```cpp
// Debug helper to visualize display areas
void debugShowAreas() {
    tft.drawRect(MAIN_AREA_X, 0, MAIN_AREA_WIDTH, DISPLAY_HEIGHT, TFT_RED);
    tft.drawRect(STATUS_AREA_X, 0, STATUS_AREA_WIDTH, DISPLAY_HEIGHT, TFT_BLUE);
}

// Print layout information to serial
void printLayoutInfo() {
    Serial.println("=== Display Layout Information ===");
    Serial.printf("Main Area: %dx%d at (%d,%d)\n", 
                  MAIN_AREA_WIDTH, DISPLAY_HEIGHT, MAIN_AREA_X, 0);
    Serial.printf("Status Area: %dx%d at (%d,%d)\n",
                  STATUS_AREA_WIDTH, DISPLAY_HEIGHT, STATUS_AREA_X, 0);
}
```

## Future Enhancements

### Planned Features

- **PWM Brightness Control:** Dynamic screen brightness adjustment
- **Actual Battery ADC Reading:** Replace placeholder with real voltage monitoring
- **Touch Support:** Lane number adjustment via capacitive touch
- **Theme Customization:** User-selectable color schemes
- **Animation Effects:** Subtle visual feedback for state changes

### Technical Improvements

- **Power Saving Modes:** Display sleep during inactivity
- **Rotation Support:** Multiple orientation options
- **Error Recovery:** Automatic display reinitialization on communication errors
- **Configuration Persistence:** Save user preferences to flash memory
- **Performance Profiling:** Built-in memory and timing analysis

## Related Documentation

- **[Hardware Specifications](HARDWARE.md)** - Physical device details and pin assignments
- **[API Reference](API.md)** - Complete function documentation with examples
- **[Developer Guide](DEVELOPER.md)** - Setup instructions and development workflow
- **[Main README](../README.md)** - Project overview and quick start guide

---

**Document Version:** 1.0  
**Last Updated:** July 2025  
**Compatible Hardware:** LilyGO T-Display S3  
**Library Dependencies:** TFT_eSPI v2.4.0+
