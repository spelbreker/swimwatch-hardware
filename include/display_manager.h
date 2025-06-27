#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <TFT_eSPI.h>
#include <SPI.h>

// Display Constants
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 170

// Colors
#define COLOR_BACKGROUND TFT_BLACK
#define COLOR_TIME_DISPLAY TFT_GREEN
#define COLOR_LAP_INFO TFT_YELLOW
#define COLOR_STATUS TFT_CYAN
#define COLOR_ERROR TFT_RED
#define COLOR_WARNING TFT_ORANGE

// Display areas based on the new layout design
// Left side (main area): 0-240px width
// Right side (status): 240-320px width (80px wide)

// Left side areas
#define MAIN_AREA_WIDTH 240
#define MAIN_AREA_X 0

#define AREA_STOPWATCH_Y 0
#define AREA_STOPWATCH_HEIGHT 80
#define AREA_LAP1_Y 80
#define AREA_LAP1_HEIGHT 30
#define AREA_LAP2_Y 110  
#define AREA_LAP2_HEIGHT 30
#define AREA_LAP3_Y 140
#define AREA_LAP3_HEIGHT 30

// Right side areas  
#define STATUS_AREA_WIDTH 80
#define STATUS_AREA_X 240

#define AREA_WIFI_STATUS_Y 0
#define AREA_WIFI_STATUS_HEIGHT 40
#define AREA_WEBSOCKET_STATUS_Y 40
#define AREA_WEBSOCKET_STATUS_HEIGHT 40
#define AREA_LANE_INFO_Y 80
#define AREA_LANE_INFO_HEIGHT 45
#define AREA_BATTERY_STATUS_Y 125
#define AREA_BATTERY_STATUS_HEIGHT 45

class DisplayManager {
private:
    TFT_eSPI tft;
    
    // Display state tracking
    String lastTimeString;
    String lastWiFiStatus;
    String lastWebSocketStatus;
    String lastLaneInfo;
    String lastBatteryString;
    String lastLap1;
    String lastLap2;
    String lastLap3;
    String lastStartupMessage;
    
    bool stopwatchAreaDirty;
    bool wifiAreaDirty;
    bool websocketAreaDirty;
    bool laneAreaDirty;
    bool batteryAreaDirty;
    bool lapAreaDirty;
    
    // Display settings
    uint8_t timeFont;
    uint8_t statusFont;
    uint8_t lapFont;
    
    // Helper methods
    void clearArea(int16_t x, int16_t y, int16_t w, int16_t h);
    String formatTimeDisplay(uint32_t milliseconds, bool showCentiseconds = true);
    
public:
    DisplayManager();
    
    // Initialization
    bool init();
    void setRotation(uint8_t rotation);
    void setBrightness(uint8_t brightness);
    
    // Screen management
    void clearScreen();
    void showSplashScreen();
    
    // Main stopwatch display
    void updateStopwatchDisplay(uint32_t elapsedMs, bool isRunning = false);
    void showStartupMessage(const String& message);
    void clearStartupMessage();
    
    // Lap times display (left side)
    void updateLapTime(uint8_t lapNumber, const String& time);
    void clearLapTimes();
    
    // Status displays (right side)
    void updateWiFiStatus(const String& status, bool isConnected = false, int rssi = 0);
    void updateWebSocketStatus(const String& status, bool isConnected = false, int pingMs = -1);
    void updateLaneInfo(uint8_t laneNumber);
    void updateBatteryDisplay(float voltage, uint8_t percentage);
    
    // Layout helper
    void drawBorders();
    void clearStatusAreas();
    
    // Status display (backward compatibility)
    void showWiFiStatus(const String& message, bool isError = false);
    void showWebSocketStatus(const String& message, bool isConnected = false);
    void showGeneralStatus(const String& message, uint16_t color = COLOR_STATUS);
    void clearStatus();
    
    // Legacy support
    void showTimeZero();
    String formatStopwatchTime(uint32_t milliseconds, bool isRunning = true);
    void showBatteryStatus(float voltage, uint8_t percentage);
    
    // Lap display
    void updateLapDisplay(const uint8_t* laps, uint8_t lapCount, uint8_t maxDisplay = 5);
    void clearLaps();
    
    // Connection info
    void showConnectionInfo(const String& ssid, const IPAddress& ip);
    void showConfigPortalInfo(const String& apName, const String& apPassword);
    
    // Force refresh
    void forceRefresh();
    bool needsUpdate();
};

#endif // DISPLAY_MANAGER_H
