# API Reference - T-Display S3 Stopwatch

## 📋 Overview

This document provides a comprehensive API reference for all modules in the T-Display S3 Stopwatch system. Each module is designed with a clean interface and clear separation of concerns.

## 🖥️ DisplayManager Class

### Constructor
```cpp
DisplayManager::DisplayManager()
```
Initializes display manager with default settings and marks all areas as dirty for initial refresh.

### Initialization
```cpp
bool DisplayManager::init()
```
**Returns**: `true` if display initialized successfully  
**Description**: Initializes TFT display, sets rotation, and draws initial layout borders.

### Screen Management
```cpp
void DisplayManager::clearScreen()
void DisplayManager::setRotation(uint8_t rotation)
void DisplayManager::setBrightness(uint8_t brightness)  // TODO: Implement PWM
void DisplayManager::forceRefresh()
bool DisplayManager::needsUpdate()
```

**Parameters**:
- `rotation`: 0-3 (0=portrait, 1=landscape, 2=inverted portrait, 3=inverted landscape)
- `brightness`: 0-255 (brightness level, requires PWM implementation)

### Stopwatch Display
```cpp
void DisplayManager::updateStopwatchDisplay(uint32_t elapsedMs, bool isRunning)
void DisplayManager::showStartupMessage(const String& message)
void DisplayManager::clearStartupMessage()
void DisplayManager::showTimeZero()  // Legacy compatibility
```

**Parameters**:
- `elapsedMs`: Elapsed time in milliseconds
- `isRunning`: Display color changes based on running state
- `message`: Startup message to display in main area

**Time Format**:
- Running: `mm:ss:m` (1 digit milliseconds)
- Stopped: `mm:ss:mm` (2 digit milliseconds)

### Lap Time Management
```cpp
void DisplayManager::updateLapTime(uint8_t lapNumber, const String& time)
void DisplayManager::clearLapTimes()
void DisplayManager::updateLapDisplay(const uint8_t* laps, uint8_t lapCount, uint8_t maxDisplay)
void DisplayManager::clearLaps()  // Legacy compatibility
```

**Parameters**:
- `lapNumber`: 1-3 (only 3 lap display areas available)
- `time`: Formatted time string (e.g., "01:23:45")
- `laps`: Array of lap times in seconds
- `lapCount`: Number of laps in array

### Status Display
```cpp
void DisplayManager::updateWiFiStatus(const String& status, bool isConnected, int rssi = 0)
void DisplayManager::updateWebSocketStatus(const String& status, bool isConnected, int pingMs = -1)
void DisplayManager::updateLaneInfo(uint8_t laneNumber)
void DisplayManager::updateBatteryDisplay(float voltage, uint8_t percentage)
```

**Parameters**:
- `status`: Status text to display
- `isConnected`: Affects display color (green=connected, red=error, orange=warning)
- `rssi`: WiFi signal strength in dBm (optional)
- `pingMs`: WebSocket ping time in milliseconds (optional)
- `laneNumber`: Lane number for this device
- `voltage`: Battery voltage in volts
- `percentage`: Battery percentage (0-100)

### Legacy Compatibility
```cpp
void DisplayManager::showGeneralStatus(const String& message, uint16_t color)
void DisplayManager::clearStatus()
void DisplayManager::showConnectionInfo(const String& ssid, const IPAddress& ip)
void DisplayManager::showConfigPortalInfo(const String& apName, const String& apPassword)
void DisplayManager::showBatteryStatus(float voltage, uint8_t percentage)
void DisplayManager::showWiFiStatus(const String& message, bool isError)
void DisplayManager::showWebSocketStatus(const String& message, bool isConnected)
```

### Display Areas and Constants
```cpp
// Display dimensions
static const int DISPLAY_WIDTH = 320;
static const int DISPLAY_HEIGHT = 170;

// Area definitions
static const int MAIN_AREA_WIDTH = 240;
static const int STATUS_AREA_WIDTH = 80;
static const int STATUS_AREA_X = 240;

// Area heights
static const int AREA_STOPWATCH_HEIGHT = 80;
static const int AREA_LAP_HEIGHT = 30;
static const int AREA_WIFI_STATUS_HEIGHT = 40;
static const int AREA_WEBSOCKET_STATUS_HEIGHT = 40;
static const int AREA_LANE_INFO_HEIGHT = 45;
static const int AREA_BATTERY_STATUS_HEIGHT = 45;

// Colors
static const uint16_t COLOR_BACKGROUND = 0x0000;        // Black
static const uint16_t COLOR_TIME_DISPLAY = 0x07E0;     // Green
static const uint16_t COLOR_WARNING = 0xFFE0;          // Yellow
static const uint16_t COLOR_ERROR = 0xF800;            // Red
static const uint16_t COLOR_STATUS = 0x07FF;           // Cyan
static const uint16_t COLOR_LAP_INFO = 0xFFE0;         // Yellow
```

## 🔘 ButtonManager Class

### Constructor
```cpp
ButtonManager::ButtonManager()
```
Initializes button manager with default configuration.

### Initialization
```cpp
bool ButtonManager::init()
```
**Returns**: `true` if initialization successful  
**Description**: Sets up GPIO pins and attaches hardware interrupts.

### Button Events
```cpp
ButtonEvent ButtonManager::getButtonEvent()
bool ButtonManager::isButtonPressed(uint8_t buttonPin)
```

**Returns**:
- `getButtonEvent()`: ButtonEvent enum or BUTTON_NONE if no event
- `isButtonPressed()`: Current button state (true if pressed)

### ButtonEvent Enumeration
```cpp
enum ButtonEvent {
    BUTTON_NONE = 0,
    BUTTON1_PRESSED,    // GPIO0 - Start/Lap
    BUTTON2_PRESSED,    // GPIO14 - Stop/Reset
    BUTTON3_PRESSED     // GPIO2 - Split Timer
};
```

### Button Pin Constants
```cpp
static const uint8_t BUTTON1_PIN = 0;   // Start/Lap (internal pullup)
static const uint8_t BUTTON2_PIN = 14;  // Stop/Reset (internal pullup)
static const uint8_t BUTTON3_PIN = 2;   // Split (external pulldown required)
```

### Debounce Configuration
```cpp
static const uint32_t DEBOUNCE_DELAY = 50;  // milliseconds
```

## 🌐 ConnectivityManager Class

### Constructor
```cpp
ConnectivityManager::ConnectivityManager()
```

### Initialization
```cpp
bool ConnectivityManager::initWiFi(DisplayManager& display)
```

**Parameters**:
- `display`: Reference to DisplayManager for status updates

**Returns**: `true` if initialization successful

### WiFi Management
```cpp
bool ConnectivityManager::isWiFiConnected()
int ConnectivityManager::getWiFiRSSI()
String ConnectivityManager::getWiFiSSID()
IPAddress ConnectivityManager::getLocalIP()
```

### Configuration Management
```cpp
bool ConnectivityManager::saveConfig(const String& wsServer, int wsPort, const String& lane)
bool ConnectivityManager::loadConfig(String& wsServer, int& wsPort, String& lane)
void ConnectivityManager::clearConfig()
```

**Parameters**:
- `wsServer`: WebSocket server address
- `wsPort`: WebSocket port number
- `lane`: Lane identifier string

### Configuration Portal
```cpp
void ConnectivityManager::startConfigPortal()
bool ConnectivityManager::isConfigPortalActive()
```

**Description**: Starts WiFiManager configuration portal on `192.168.4.1`

## ⏱️ WebSocketStopwatch Class

### Constructor
```cpp
WebSocketStopwatch::WebSocketStopwatch()
```

### Initialization
```cpp
bool WebSocketStopwatch::init(const String& serverUrl, int port, const String& path, const String& lane, DisplayManager& display)
```

**Parameters**:
- `serverUrl`: WebSocket server address
- `port`: Server port (443 for SSL, 80 for non-SSL)
- `path`: WebSocket path (e.g., "/ws")
- `lane`: Lane identifier
- `display`: Reference to DisplayManager

### Connection Management
```cpp
void WebSocketStopwatch::connect()
void WebSocketStopwatch::disconnect()
bool WebSocketStopwatch::isConnected()
void WebSocketStopwatch::handleEvents()
```

**Description**: 
- `handleEvents()` must be called regularly in main loop for message processing

### Stopwatch Control
```cpp
void WebSocketStopwatch::start()
void WebSocketStopwatch::start(uint32_t serverTimestamp)
void WebSocketStopwatch::stop()
void WebSocketStopwatch::reset()
void WebSocketStopwatch::addLap()
void WebSocketStopwatch::addSplit()
```

**Parameters**:
- `serverTimestamp`: Server-provided start time for synchronization

### State Management
```cpp
bool WebSocketStopwatch::isRunning()
bool WebSocketStopwatch::isStopped()
uint32_t WebSocketStopwatch::getElapsedTime()
uint8_t WebSocketStopwatch::getLapCount()
uint8_t WebSocketStopwatch::getSplitCount()
```

### Message Handling
```cpp
void WebSocketStopwatch::sendSplitTime(uint32_t splitTime)
void WebSocketStopwatch::sendMessage(const String& message)
```

**Parameters**:
- `splitTime`: Split time in milliseconds
- `message`: JSON formatted message string

### Lag Compensation
```cpp
int WebSocketStopwatch::getPingTime()
bool WebSocketStopwatch::isLagCompensationActive()
```

**Returns**:
- `getPingTime()`: Current ping time in milliseconds
- `isLagCompensationActive()`: Whether compensation is being applied

### WebSocket Message Format

#### Received Messages
```json
{
  "type": "start",
  "timestamp": 1234567890
}

{
  "type": "reset"
}

{
  "type": "time_sync",
  "server_time": 1234567890
}
```

#### Sent Messages
```json
{
  "type": "split",
  "lane": "9",
  "time-ms": 1234567890,
  "time": "MM:SS:CC"
}

{
  "type": "lap",
  "lane": "9", 
  "lap_number": 1,
  "time-ms": 1234567890,
  "time": "MM:SS:CC"
}
```

## 🔧 Configuration Structures

### WiFiManager Custom Parameters
```cpp
// WebSocket server configuration
WiFiManagerParameter wsServer("server", "WebSocket Server", "scherm.azckamp.nl", 40);
WiFiManagerParameter wsPort("port", "WebSocket Port", "443", 6);
WiFiManagerParameter wsLane("lane", "Lane Number", "9", 4);
```

### LittleFS Configuration File
```json
{
  "ws_server": "scherm.azckamp.nl",
  "ws_port": 443,
  "lane": "9"
}
```

## 🎨 Color Definitions

### TFT Colors (16-bit RGB565)
```cpp
#define COLOR_BACKGROUND    0x0000  // Black
#define COLOR_TIME_DISPLAY  0x07E0  // Green (running/connected)
#define COLOR_WARNING       0xFFE0  // Yellow (stopped/warning) 
#define COLOR_ERROR         0xF800  // Red (error/disconnected)
#define COLOR_STATUS        0x07FF  // Cyan (normal status)
#define COLOR_LAP_INFO      0xFFE0  // Yellow (lap information)
```

### Color Usage Guidelines
- **Green**: Active states (running timer, connected status)
- **Red**: Error states (disconnected, failed operations)
- **Yellow**: Warning states (stopped timer, timeouts)
- **Cyan**: Normal informational status
- **White**: Text on colored backgrounds

## 📊 Performance Specifications

### Update Frequencies
```cpp
static const uint32_t DISPLAY_UPDATE_INTERVAL = 100;    // 100ms
static const uint32_t STATUS_UPDATE_INTERVAL = 1000;    // 1 second
static const uint32_t WEBSOCKET_PING_INTERVAL = 10000;  // 10 seconds
static const uint32_t NTP_SYNC_INTERVAL = 3600000;      // 1 hour
```

### Memory Usage
```cpp
// Approximate memory usage per module
DisplayManager:     ~2KB RAM (cached strings, display buffer)
ButtonManager:      ~1KB RAM (state tracking, debounce)
ConnectivityManager: ~3KB RAM (WiFi stack)
WebSocketStopwatch: ~6KB RAM (WebSocket client, JSON parsing)
```

### Timing Accuracy
- **Display Updates**: ±10ms
- **Button Response**: <10ms (interrupt-driven)
- **WebSocket Ping**: Measured and compensated
- **Internal Timer**: Uses millis() with ±1ms accuracy

## 🛠️ Build Configuration

### PlatformIO Dependencies
```ini
lib_deps = 
    links2004/WebSockets @ ^2.4.1
    bblanchon/ArduinoJson @ ^6.21.3
```

### Compiler Flags
```ini
build_flags = 
    -DCORE_DEBUG_LEVEL=3
    -DWIFI_MANAGER_DEBUG
    -DWEBSOCKETS_DEBUG
```

## 🔍 Error Codes

### Return Values
```cpp
// Success codes
#define SUCCESS                 0
#define INIT_OK                 true
#define CONNECTION_OK           true

// Error codes  
#define ERROR_DISPLAY_INIT     -1
#define ERROR_WIFI_CONNECT     -2
#define ERROR_WEBSOCKET_FAIL   -3
#define ERROR_CONFIG_LOAD      -4
#define ERROR_CONFIG_SAVE      -5
```

### Debug Levels
```cpp
#define DEBUG_LEVEL_NONE    0
#define DEBUG_LEVEL_ERROR   1
#define DEBUG_LEVEL_WARN    2
#define DEBUG_LEVEL_INFO    3
#define DEBUG_LEVEL_DEBUG   4
#define DEBUG_LEVEL_VERBOSE 5
```

## 📚 Usage Examples

### Basic Initialization
```cpp
DisplayManager display;
ButtonManager buttons;
ConnectivityManager connectivity;
WebSocketStopwatch stopwatch;

void setup() {
    Serial.begin(115200);
    
    // Initialize display first
    if (!display.init()) {
        Serial.println("Display init failed!");
        return;
    }
    
    // Initialize buttons
    if (!buttons.init()) {
        Serial.println("Button init failed!");
        return;
    }
    
    // Initialize WiFi with display updates
    if (!connectivity.initWiFi(display)) {
        Serial.println("WiFi init failed!");
        return;
    }
    
    // Initialize WebSocket
    String server, lane;
    int port;
    connectivity.loadConfig(server, port, lane);
    stopwatch.init(server, port, "/ws", lane, display);
}
```

### Main Loop Pattern
```cpp
void loop() {
    static uint32_t lastDisplayUpdate = 0;
    static uint32_t lastStatusUpdate = 0;
    
    uint32_t now = millis();
    
    // Handle WebSocket events
    stopwatch.handleEvents();
    
    // Check for button events
    ButtonEvent event = buttons.getButtonEvent();
    if (event != BUTTON_NONE) {
        handleButtonEvent(event);
    }
    
    // Update display (100ms interval)
    if (now - lastDisplayUpdate >= 100) {
        display.updateStopwatchDisplay(stopwatch.getElapsedTime(), stopwatch.isRunning());
        lastDisplayUpdate = now;
    }
    
    // Update status (1s interval)
    if (now - lastStatusUpdate >= 1000) {
        updateStatusDisplay();
        lastStatusUpdate = now;
    }
    
    delay(10); // Small delay to prevent watchdog timeout
}
```

---

## 📖 Additional Resources

- [ESP32-S3 Arduino Core API](https://docs.espressif.com/projects/arduino-esp32/en/latest/)
- [TFT_eSPI Library Documentation](https://github.com/Bodmer/TFT_eSPI)
- [WebSockets Library Documentation](https://github.com/Links2004/arduinoWebSockets)
- [ArduinoJson Documentation](https://arduinojson.org/)
- [WiFiManager Documentation](https://github.com/tzapu/WiFiManager)
