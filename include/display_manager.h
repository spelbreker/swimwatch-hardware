

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

/**
 * @file display_manager.h
 * @brief Display Manager for LilyGO T-Display S3 Swimming Stopwatch
 * 
 * Provides a clean, two-panel interface for swimming competition timing:
 * - Left panel: Stopwatch time and split times (240px wide)
 * - Right panel: Status information with swimming pool theme (80px wide)
 * 
 * Features:
 * - Efficient dirty-region updates to minimize flicker
 * - WiFi signal strength visualization with colored bars
 * - Real-time WebSocket connection monitoring
 * - Battery status with low-power warnings
 * - Clean split time display (last 3 splits, rolling)
 * - Swimming pool themed color scheme
 * 
 * @author Swimming Timer System
 * @date 2025
 */

#include <stdint.h>
#include <TFT_eSPI.h>
#include <SPI.h>

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
#define AREA_BATTERY_STATUS_Y 125
#define AREA_BATTERY_STATUS_HEIGHT 45

/**
 * @class DisplayManager
 * @brief Manages the TFT display for the swimming stopwatch application
 * 
 * This class provides a comprehensive display management system for the
 * LilyGO T-Display S3 swimming stopwatch. It implements a two-panel layout
 * with efficient dirty-region updates and swimming-themed visual design.
 * 
 * Layout Structure:
 * ┌─────────────────────────┬──────────────┐
 * │    Stopwatch Display    │ WiFi Status  │
 * │      (240x80px)         │   (80x40px)  │
 * ├─────────────────────────┼──────────────┤
 * │    Split Times Area     │ WebSocket    │
 * │   Split 1: xx:xx:xx     │   (80x40px)  │
 * │   Split 2: xx:xx:xx     ├──────────────┤
 * │   Split 3: xx:xx:xx     │ Lane Number  │
 * │      (240x90px)         │   (80x45px)  │
 * │                         ├──────────────┤
 * │                         │ Battery      │
 * │                         │   (80x45px)  │
 * └─────────────────────────┴──────────────┘
 * 
 * Key Features:
 * - Dirty-region tracking for efficient updates
 * - Swimming pool themed color scheme  
 * - WiFi signal strength visualization
 * - Real-time connection status monitoring
 * - Clean split time management (rolling display)
 * - Battery monitoring with low-power alerts
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
    String lastBatteryString;
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
    void updateBatteryDisplay(float voltage, uint8_t percentage);
    
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
