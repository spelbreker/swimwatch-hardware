/**
 * @file display_manager.h
 * @brief TFT display manager for SwimWatch — LilyGO T-Display S3
 *
 * Two-panel layout on 320x170 ST7789V:
 * - Left (240px): Stopwatch time + split times (last 3, rolling)
 * - Right (80px): WiFi / WebSocket / Lane / NTP clock (swimming pool theme)
 *
 * Updates use dirty-region tracking to minimize flicker.
 * Refresh rate: 50ms (20fps) driven by main loop.
 */
#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <stdint.h>
#include <TFT_eSPI.h>
#include <SPI.h>

// ===========================================
// Hardware Configuration for T-Display S3
// ===========================================

// Display Constants
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 170

// ===========================================
// Color Definitions (RGB565 Format)
// ===========================================

// Primary interface colors
#define COLOR_BACKGROUND TFT_BLACK
#define COLOR_TIME_DISPLAY TFT_GREEN
#define COLOR_LAP_INFO TFT_YELLOW
#define COLOR_STATUS TFT_CYAN
#define COLOR_ERROR TFT_RED
#define COLOR_WARNING TFT_ORANGE

// Swimming pool themed sidebar
#define COLOR_SIDEBAR_BG 0x049D  // Swimming pool blue color (#0092b8)

// WiFi signal strength indicators
#define COLOR_WIFI_BAR TFT_WHITE
#define COLOR_WIFI_BAR_WEAK TFT_RED
#define COLOR_WIFI_BAR_GOOD TFT_YELLOW
#define COLOR_WIFI_BAR_STRONG TFT_GREEN

// ===============================================
// Layout Configuration - Two Panel Design
// ===============================================

// Display areas based on the new layout design
// Left side (main area): 0-240px width (75% of screen)
// Right side (status): 240-320px width (25% of screen)

// Main area dimensions (left side - stopwatch and splits)
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

// Status area dimensions (right side - system information)
#define STATUS_AREA_WIDTH 80
#define STATUS_AREA_X 240

#define AREA_WIFI_STATUS_Y 0
#define AREA_WIFI_STATUS_HEIGHT 40
#define AREA_WEBSOCKET_STATUS_Y 40
#define AREA_WEBSOCKET_STATUS_HEIGHT 40
#define AREA_LANE_INFO_Y 80
#define AREA_LANE_INFO_HEIGHT 45
#define AREA_BATTERY_Y 125
#define AREA_BATTERY_HEIGHT 22
#define AREA_NTP_CLOCK_Y 147
#define AREA_NTP_CLOCK_HEIGHT 23

/**
 * @class DisplayManager
 * @brief Manages the TFT display for SwimWatch
 *
 * Layout (320x170, landscape):
 * ┌──────────────────────────┬──────────────┐
 * │   Stopwatch Display      │ WiFi Status  │
 * │     MM:SS.d / MM:SS.cc   │  (bars+RSSI) │
 * ├──────────────────────────┼──────────────┤
 * │   Split Times            │ WebSocket    │
 * │   Split 1: MM:SS.cc     │  (WS+ping)   │
 * │   Split 2: MM:SS.cc     ├──────────────┤
 * │   Split 3: MM:SS.cc     │ Lane / Role  │
 * │                          ├──────────────┤
 * │                          │ Battery %    │
 * │                          ├──────────────┤
 * │                          │ NTP Clock    │
 * └──────────────────────────┴──────────────┘
 */
class DisplayManager {
private:
    // ===================================
    // Internal State and Configuration
    // ===================================
    
    TFT_eSPI tft;
    
    // Display state tracking for efficient updates
    String lastTimeString;
    String lastWiFiStatus;
    String lastWebSocketStatus;
    String lastLaneInfo;
    String lastNtpClock;
    String lastBatteryStr;
    String lastLap1;
    String lastLap2;
    String lastLap3;
    String lastStartupMessage;
    String lastEventHeat;
    
    // Dirty flags for selective area updates
    bool stopwatchAreaDirty;
    bool wifiAreaDirty;
    bool websocketAreaDirty;
    bool laneAreaDirty;
    bool ntpClockAreaDirty;
    bool batteryAreaDirty;
    bool lapAreaDirty;
    
    // Font configuration for different display areas
    uint8_t timeFont;      // Large font for main timer
    uint8_t statusFont;    // Small font for status text  
    uint8_t lapFont;       // Medium font for lap times
    
    // ===================================
    // Internal Helper Methods
    // ===================================
    
    void clearArea(int16_t x, int16_t y, int16_t w, int16_t h);
    void drawSidebarBackground();
    void drawWiFiStrengthBars(int rssi, int x, int y, int width, int height);
    String formatTimeDisplay(uint32_t milliseconds, bool showCentiseconds = true);
    
public:
    // Send raw command to TFT (for sleep/off)
    void sendTFTCommand(uint8_t cmd);
    // ===================================
    // Constructor and Initialization
    // ===================================
    
    DisplayManager();
    
    // ===================================
    // Hardware and Display Setup
    // ===================================
    
    // Initialization
    bool init();
    void setRotation(uint8_t rotation);
    void setBrightness(uint8_t brightness);
    
    // ===================================
    // Screen and Layout Management
    // ===================================
    
    // Screen management
    void clearScreen();
    void showSplashScreen();
    
    // ===================================
    // Primary Stopwatch Display
    // ===================================
    
    // Main stopwatch display
    void updateStopwatchDisplay(uint32_t elapsedMs, bool isRunning = false);
    void showStartupMessage(const String& message);
    void clearStartupMessage();
    
    // Show Event/Heat under the stopwatch time (left side)
    void setEventHeat(const String& event, const String& heat);
    
    // ===================================
    // Split Time Management
    // ===================================
    
    // Lap times display (left side)
    void updateLapTime(uint8_t lapNumber, const String& time);
    void clearLapTimes();
    
    // ===================================
    // Status Information Display  
    // ===================================
    
    // Status displays (right side)
    void updateWiFiStatus(const String& status, bool isConnected = false, int rssi = 0);
    void updateWebSocketStatus(const String& status, bool isConnected = false, int pingMs = -1);
    void updateLaneInfo(uint8_t laneNumber);
    void updateRoleInfo(const String& role, const String& event, const String& heat, uint8_t laneNumber);
    void updateNtpClock(const String& timeString, bool isSynced = true);
    void updateBatteryDisplay(uint8_t percentage);
    
    // ===================================
    // Layout and Utility Functions
    // ===================================
    
    // Layout helper
    void drawBorders();
    void clearStatusAreas();
    
    // ===================================
    // Utility Functions
    // ===================================
    
    // Status display (used by main.cpp for error messages)
    void showGeneralStatus(const String& message, uint16_t color = COLOR_STATUS);
    
    // Connection setup (used by main.cpp for captive portal)
    void showConfigPortalInfo(const String& apName, const String& apPassword);
    
    // Time formatting (part of core API)
    String formatStopwatchTime(uint32_t milliseconds, bool isRunning = true);
    
    // ===================================
    // System State Management
    // ===================================
    
    // Force refresh
    void forceRefresh();
    bool needsUpdate();
};

#endif // DISPLAY_MANAGER_H
