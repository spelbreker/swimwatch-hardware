
#include <stdint.h>
#include "display_manager.h"

// ...existing code...
// Implementation after constructor and other methods
void DisplayManager::sendTFTCommand(uint8_t cmd) {
    tft.writecommand(cmd);
}
/**
 * @file display_manager.cpp
 * @brief Display Manager for LilyGO T-Display S3 Swimming Stopwatch
 * 
 * This file implements the display management system for a swimming stopwatch
 * using the LilyGO T-Display S3 with ST7789V 1.9" TFT display.
 * 
 * Display Layout (320x170 pixels, landscape):
 * ┌─────────────────────────────────────────┬─────────────────────┐
 * │                                         │     WiFi Status     │
 * │          Stopwatch Time Display         │    (Strength Bars)  │
 * │             (240x80 area)               ├─────────────────────┤
 * │                                         │   WebSocket Status  │
 * ├─────────────────────────────────────────┤      (WS + Ping)    │
 * │           Split Times Area              ├─────────────────────┤
 * │   Split - 1: xx:xx:xx (if available)    │    Lane Number      │
 * │   Split - 2: xx:xx:xx (if available)    │      Lane X         │
 * │   Split - 3: xx:xx:xx (if available)    ├─────────────────────┤
 * │             (240x90 area)               │   Battery Status    │
 * │                                         │    Battery XX%      │
 * └─────────────────────────────────────────┴─────────────────────┘
 * 
 * Features:
 * - Main area (0-240px): Stopwatch time and split times
 * - Sidebar (240-320px): Status information with swimming pool background
 * - WiFi strength visualization with colored bars
 * - Real-time WebSocket connection monitoring
 * - Lane number display
 * - Battery percentage monitoring
 * - Clean, swimming-themed interface
 * 
 * @author Swimming Timer System
 * @date 2025
 */

#include "display_manager.h"

// ================================
// Initialization and Basic Setup
// ================================

DisplayManager::DisplayManager() 
    : stopwatchAreaDirty(true)
    , wifiAreaDirty(true) 
    , websocketAreaDirty(true)
    , laneAreaDirty(true)
    , batteryAreaDirty(true)
    , lapAreaDirty(true)
    , timeFont(6)      // Large font for stopwatch time
    , statusFont(1)    // Small font for status text
    , lapFont(2) {     // Medium font for lap times
}

bool DisplayManager::init() {
    Serial.println("Initializing display...");
    
    tft.init();
    tft.setRotation(1); // Landscape orientation
    
    // Set default colors and fonts
    clearScreen();
    
    // Make sure lap areas start completely clean
    clearLapTimes();
    
    // Force all status areas to be marked as dirty so they will be drawn
    forceRefresh();
    
    Serial.println("Display initialized successfully");
    return true;
}

void DisplayManager::setRotation(uint8_t rotation) {
    tft.setRotation(rotation);
    forceRefresh();
}

void DisplayManager::setBrightness(uint8_t brightness) {
    // Note: TFT_eSPI doesn't have built-in brightness control
    // This would need to be implemented via PWM on the backlight pin
    // For LilyGO T-Display S3, backlight is on GPIO4
    // TODO: Implement PWM brightness control
}

void DisplayManager::clearScreen() {
    // Fill main area with black background
    tft.fillScreen(COLOR_BACKGROUND);
    
    // Draw swimming pool colored sidebar background
    drawSidebarBackground();
    
    // Reset all cached display strings
    lastTimeString = "";
    lastWiFiStatus = "";
    lastWebSocketStatus = "";
    lastLaneInfo = "";
    lastBatteryString = "";
    lastLap1 = "";
    lastLap2 = "";
    lastLap3 = "";
    lastStartupMessage = "";
    lastEventHeat = "";
    
    // Mark all display areas as needing refresh
    stopwatchAreaDirty = true;
    wifiAreaDirty = true;
    websocketAreaDirty = true;
    laneAreaDirty = true;
    batteryAreaDirty = true;
    lapAreaDirty = true;
}

void DisplayManager::showSplashScreen() {
    clearScreen();
    
    tft.setTextFont(4);
    tft.setTextColor(COLOR_TIME_DISPLAY, COLOR_BACKGROUND);
    tft.setTextDatum(MC_DATUM);
    
    tft.drawString("SwimWatch", DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2 - 20);
    tft.setTextFont(2);
    tft.setTextColor(COLOR_STATUS, COLOR_BACKGROUND);
    tft.drawString("T-Display S3", DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2 + 10);
    tft.drawString("Initializing...", DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2 + 30);
    
    delay(2000);
}

void DisplayManager::clearArea(int16_t x, int16_t y, int16_t w, int16_t h) {
    tft.fillRect(x, y, w, h, COLOR_BACKGROUND);
}

String DisplayManager::formatTimeDisplay(uint32_t milliseconds, bool showCentiseconds) {
    uint16_t minutes = milliseconds / 60000;
    uint8_t seconds = (milliseconds / 1000) % 60;
    
    char buffer[16];
    
    if (showCentiseconds) {
        uint8_t centiseconds = (milliseconds % 1000) / 10;
        sprintf(buffer, "%02d:%02d:%02d", minutes, seconds, centiseconds);
    } else {
        uint8_t millisec = (milliseconds % 1000) / 100; // Single digit milliseconds
        sprintf(buffer, "%02d:%02d:%d", minutes, seconds, millisec);
    }
    
    return String(buffer);
}

String DisplayManager::formatStopwatchTime(uint32_t milliseconds, bool isRunning) {
    uint16_t minutes = milliseconds / 60000;
    uint8_t seconds = (milliseconds / 1000) % 60;
    
    char buffer[16];
    
    if (isRunning) {
        // When running: show 1 digit for milliseconds
        uint8_t millisec = (milliseconds % 1000) / 100;
        sprintf(buffer, "%02d:%02d:%01d", minutes, seconds, millisec);
    } else {
        // When stopped: show 2 digits for milliseconds  
        uint8_t centiseconds = (milliseconds % 1000) / 10;
        sprintf(buffer, "%02d:%02d:%02d", minutes, seconds, centiseconds);
    }
    
    return String(buffer);
}

void DisplayManager::showGeneralStatus(const String& message, uint16_t color) {
    // Show status messages in the main area, below the stopwatch display
    // This ensures they don't interfere with the right sidebar
    
    // Clear the area below stopwatch for status message
    clearArea(MAIN_AREA_X, AREA_LAP1_Y, MAIN_AREA_WIDTH, AREA_LAP1_HEIGHT);
    
    tft.setTextFont(statusFont + 1);
    tft.setTextColor(color, COLOR_BACKGROUND);
    tft.setTextDatum(MC_DATUM);
    
    int centerX = MAIN_AREA_X + (MAIN_AREA_WIDTH / 2);
    int centerY = AREA_LAP1_Y + (AREA_LAP1_HEIGHT / 2);
    tft.drawString(message, centerX, centerY);
}

void DisplayManager::showConfigPortalInfo(const String& apName, const String& apPassword) {
    clearScreen();
    
    tft.setTextFont(3);
    tft.setTextColor(COLOR_WARNING, COLOR_BACKGROUND);
    tft.setTextDatum(MC_DATUM);
    
    tft.drawString("Configuration Mode", DISPLAY_WIDTH/2, 30);
    
    tft.setTextFont(2);
    tft.setTextColor(COLOR_STATUS, COLOR_BACKGROUND);
    tft.drawString("Connect to WiFi:", DISPLAY_WIDTH/2, 60);
    tft.drawString(apName, DISPLAY_WIDTH/2, 80);
    tft.drawString("Password: " + apPassword, DISPLAY_WIDTH/2, 100);
    tft.drawString("Then go to 192.168.4.1", DISPLAY_WIDTH/2, 130);
}

// ===========================
// Utility and Helper Functions
// ===========================

void DisplayManager::forceRefresh() {
    stopwatchAreaDirty = true;
    wifiAreaDirty = true;
    websocketAreaDirty = true;
    laneAreaDirty = true;
    batteryAreaDirty = true;
    lapAreaDirty = true;
}

bool DisplayManager::needsUpdate() {
    return stopwatchAreaDirty || wifiAreaDirty || websocketAreaDirty || 
           laneAreaDirty || batteryAreaDirty || lapAreaDirty;
}

// ===============================
// Core Display Update Functions  
// ===============================
void DisplayManager::drawBorders() {
    // Optional: Draw vertical line separating main area from status area
    // Commented out for cleaner look - sidebar background provides visual separation
    // tft.drawFastVLine(STATUS_AREA_X - 1, 0, DISPLAY_HEIGHT, COLOR_STATUS);
}

void DisplayManager::drawSidebarBackground() {
    // Fill the right sidebar with swimming pool blue-green color
    tft.fillRect(STATUS_AREA_X, 0, STATUS_AREA_WIDTH, DISPLAY_HEIGHT, COLOR_SIDEBAR_BG);
}

void DisplayManager::drawWiFiStrengthBars(int rssi, int x, int y, int width, int height) {
    /**
     * Draw WiFi signal strength as colored bars
     * RSSI Signal Quality Reference:
     * > -50 dBm: Excellent (4 bars, green)
     * -50 to -60 dBm: Good (3 bars, green)  
     * -60 to -70 dBm: Fair (2 bars, yellow)
     * -70 to -80 dBm: Poor (1 bar, red)
     * < -80 dBm: No signal (0 bars)
     */
    
    int barWidth = width / 4 - 2;
    int barSpacing = 2;
    
    // Clear the drawing area
    tft.fillRect(x, y, width, height, COLOR_SIDEBAR_BG);
    
    // Determine signal strength level (0-4 bars)
    int signalLevel = 0;
    if (rssi > -50) signalLevel = 4;      // Excellent
    else if (rssi > -60) signalLevel = 3; // Good  
    else if (rssi > -70) signalLevel = 2; // Fair
    else if (rssi > -80) signalLevel = 1; // Poor
    else signalLevel = 0;                 // No signal
    
    // Draw bars with increasing height from left to right
    for (int i = 0; i < 4; i++) {
        int barHeight = (height * (i + 1)) / 4;
        int barX = x + i * (barWidth + barSpacing);
        int barY = y + height - barHeight;
        
        uint16_t barColor;
        if (i < signalLevel) {
            // Active bars colored by signal quality
            if (signalLevel >= 3) barColor = COLOR_WIFI_BAR_STRONG;      // Green
            else if (signalLevel >= 2) barColor = COLOR_WIFI_BAR_GOOD;   // Yellow  
            else barColor = COLOR_WIFI_BAR_WEAK;                         // Red
        } else {
            // Inactive bars in dim color
            barColor = COLOR_STATUS;
        }
        
        tft.fillRect(barX, barY, barWidth, barHeight, barColor);
    }
}

void DisplayManager::updateStopwatchDisplay(uint32_t elapsedMs, bool isRunning) {
    String timeString = formatStopwatchTime(elapsedMs, isRunning);
    
    if (timeString != lastTimeString || stopwatchAreaDirty) {
        // Clear stopwatch area
        clearArea(MAIN_AREA_X, AREA_STOPWATCH_Y, MAIN_AREA_WIDTH, AREA_STOPWATCH_HEIGHT);
        
        tft.setTextFont(timeFont);
        tft.setTextColor(isRunning ? COLOR_TIME_DISPLAY : COLOR_WARNING, COLOR_BACKGROUND);
        tft.setTextDatum(MC_DATUM);
        
        // Center the time in the stopwatch area
        int centerX = MAIN_AREA_X + (MAIN_AREA_WIDTH / 2);
        int centerY = AREA_STOPWATCH_Y + (AREA_STOPWATCH_HEIGHT / 2);
        tft.drawString(timeString, centerX, centerY);
        
        lastTimeString = timeString;
        stopwatchAreaDirty = false;
    }
    
    // Draw Event/Heat at bottom of screen (always visible when set)
    if (!lastEventHeat.isEmpty()) {
        tft.setTextFont(4);
        tft.setTextColor(COLOR_LAP_INFO, COLOR_BACKGROUND);
        tft.setTextDatum(MC_DATUM);
        int centerX = MAIN_AREA_X + (MAIN_AREA_WIDTH / 2);
        int bottomY = DISPLAY_HEIGHT - 10; // 10px from bottom
        tft.fillRect(MAIN_AREA_X, bottomY - 15, MAIN_AREA_WIDTH, 25, COLOR_BACKGROUND);
        tft.drawString(lastEventHeat, centerX, bottomY);
    }
}

void DisplayManager::showStartupMessage(const String& message) {
    if (message != lastStartupMessage || stopwatchAreaDirty) {
        // Clear the stopwatch area
        tft.fillRect(MAIN_AREA_X, AREA_STOPWATCH_Y, MAIN_AREA_WIDTH, AREA_STOPWATCH_HEIGHT, COLOR_BACKGROUND);
        
        tft.setTextFont(2);
        tft.setTextColor(COLOR_STATUS, COLOR_BACKGROUND);
        tft.setTextDatum(MC_DATUM);
        
        // Center the message in the stopwatch area
        int centerX = MAIN_AREA_X + (MAIN_AREA_WIDTH / 2);
        int centerY = AREA_STOPWATCH_Y + (AREA_STOPWATCH_HEIGHT / 2);
        
        // Split long messages into multiple lines for better readability
        if (message.length() > 20) {
            int spaceIndex = message.indexOf(' ', 10);
            if (spaceIndex > 0 && spaceIndex < message.length() - 5) {
                String line1 = message.substring(0, spaceIndex);
                String line2 = message.substring(spaceIndex + 1);
                tft.drawString(line1, centerX, centerY - 10);
                tft.drawString(line2, centerX, centerY + 10);
            } else {
                tft.drawString(message, centerX, centerY);
            }
        } else {
            tft.drawString(message, centerX, centerY);
        }
        
        lastStartupMessage = message;
        stopwatchAreaDirty = false;
    }
}

void DisplayManager::clearStartupMessage() {
    if (!lastStartupMessage.isEmpty()) {
        // Actually clear the stopwatch area
        tft.fillRect(MAIN_AREA_X, AREA_STOPWATCH_Y, MAIN_AREA_WIDTH, AREA_STOPWATCH_HEIGHT, COLOR_BACKGROUND);
        lastStartupMessage = "";
        stopwatchAreaDirty = false;
    }
}

void DisplayManager::setEventHeat(const String& event, const String& heat) {
    String combined = String("Event:") + event + " Heat:" + heat;
    if (combined != lastEventHeat) {
        lastEventHeat = combined;
        // Trigger redraw of stopwatch area to include updated event/heat
        stopwatchAreaDirty = true;
    }
}

// ===========================
// Split Time and Lap Management
// ===========================

void DisplayManager::updateLapTime(uint8_t lapNumber, const String& time) {
    String* lastLap = nullptr;
    int yPos = 0;
    
    switch (lapNumber) {
        case 1:
            lastLap = &lastLap1;
            yPos = AREA_LAP1_Y;
            break;
        case 2:
            lastLap = &lastLap2;
            yPos = AREA_LAP2_Y;
            break;
        case 3:
            lastLap = &lastLap3;
            yPos = AREA_LAP3_Y;
            break;
        default:
            return; // Only support 3 laps
    }
    
    if (time != *lastLap || lapAreaDirty) {
        // Clear the lap area (avoid bottom 30px reserved for event/heat)
        int maxHeight = (yPos + AREA_LAP1_HEIGHT < DISPLAY_HEIGHT - 30) ? AREA_LAP1_HEIGHT : DISPLAY_HEIGHT - 30 - yPos;
        if (maxHeight > 0) {
            tft.fillRect(MAIN_AREA_X, yPos, MAIN_AREA_WIDTH, maxHeight, COLOR_BACKGROUND);
        
            // Draw text if we have a valid time
            if (!time.isEmpty()) {
                tft.setTextFont(lapFont);
                tft.setTextColor(COLOR_LAP_INFO, COLOR_BACKGROUND);
                tft.setTextDatum(ML_DATUM);
                tft.drawString(time, MAIN_AREA_X + 5, yPos + (maxHeight / 2));
            }
        }
        
        *lastLap = time;
    }
    lapAreaDirty = false;
}

void DisplayManager::clearLapTimes() {
    // Clear all lap areas (but not bottom 30px reserved for event/heat)
    int clearHeight = (DISPLAY_HEIGHT - 30) - AREA_LAP1_Y;
    if (clearHeight > 0) {
        tft.fillRect(MAIN_AREA_X, AREA_LAP1_Y, MAIN_AREA_WIDTH, clearHeight, COLOR_BACKGROUND);
    }
    lastLap1 = "";
    lastLap2 = "";
    lastLap3 = "";
    lapAreaDirty = false;
}

// ===========================
// Status Area Updates
// ===========================

// =============================
// Status Display Functions
// =============================

void DisplayManager::updateWiFiStatus(const String& status, bool isConnected, int rssi) {
    String wifiText = isConnected ? "WiFi" : status;
    String currentStatus = wifiText + (isConnected && rssi != 0 ? String(rssi) : "");
    
    if (currentStatus != lastWiFiStatus || wifiAreaDirty) {
        // Clear the WiFi status area with sidebar background
        tft.fillRect(STATUS_AREA_X, AREA_WIFI_STATUS_Y, STATUS_AREA_WIDTH, AREA_WIFI_STATUS_HEIGHT, COLOR_SIDEBAR_BG);
        
        if (isConnected && rssi != 0) {
            // Draw WiFi strength bars
            int barAreaHeight = 15;
            int barY = AREA_WIFI_STATUS_Y + 5;
            int barWidth = STATUS_AREA_WIDTH - 10;
            drawWiFiStrengthBars(rssi, STATUS_AREA_X + 5, barY, barWidth, barAreaHeight);
            
            // Show "WiFi" text below bars
            tft.setTextFont(1);
            tft.setTextColor(TFT_WHITE, COLOR_SIDEBAR_BG);
            tft.setTextDatum(MC_DATUM);
            int centerX = STATUS_AREA_X + (STATUS_AREA_WIDTH / 2);
            tft.drawString("WiFi", centerX, AREA_WIFI_STATUS_Y + 28);
            
            // Show RSSI value
            tft.drawString(String(rssi) + "dBm", centerX, AREA_WIFI_STATUS_Y + 38);
        } else {
            // Show disconnected status
            tft.setTextFont(1);
            tft.setTextColor(COLOR_ERROR, COLOR_SIDEBAR_BG);
            tft.setTextDatum(MC_DATUM);
            int centerX = STATUS_AREA_X + (STATUS_AREA_WIDTH / 2);
            int centerY = AREA_WIFI_STATUS_Y + (AREA_WIFI_STATUS_HEIGHT / 2);
            tft.drawString(wifiText, centerX, centerY);
        }
        
        lastWiFiStatus = currentStatus;
        wifiAreaDirty = false;
    }
}

void DisplayManager::updateWebSocketStatus(const String& status, bool isConnected, int pingMs) {
    String wsText;
    if (isConnected && pingMs >= 0) {
        wsText = "WS\n" + String(pingMs) + "ms";
    } else if (isConnected) {
        wsText = "WS\nOK";
    } else {
        wsText = "WS\n" + status;
    }
    
    if (wsText != lastWebSocketStatus || websocketAreaDirty) {
        // Clear with sidebar background
        tft.fillRect(STATUS_AREA_X, AREA_WEBSOCKET_STATUS_Y, STATUS_AREA_WIDTH, AREA_WEBSOCKET_STATUS_HEIGHT, COLOR_SIDEBAR_BG);
        
        tft.setTextFont(1);
        tft.setTextColor(isConnected ? TFT_WHITE : COLOR_ERROR, COLOR_SIDEBAR_BG);
        tft.setTextDatum(MC_DATUM);
        
        int centerX = STATUS_AREA_X + (STATUS_AREA_WIDTH / 2);
        int centerY = AREA_WEBSOCKET_STATUS_Y + (AREA_WEBSOCKET_STATUS_HEIGHT / 2);
        tft.drawString(wsText, centerX, centerY);
        
        lastWebSocketStatus = wsText;
        websocketAreaDirty = false;
    }
}

void DisplayManager::updateLaneInfo(uint8_t laneNumber) {
    String laneText = "Lane\n" + String(laneNumber);
    
    if (laneText != lastLaneInfo || laneAreaDirty) {
        // Clear with sidebar background  
        tft.fillRect(STATUS_AREA_X, AREA_LANE_INFO_Y, STATUS_AREA_WIDTH, AREA_LANE_INFO_HEIGHT, COLOR_SIDEBAR_BG);
        
        tft.setTextFont(2);
        tft.setTextColor(TFT_WHITE, COLOR_SIDEBAR_BG);
        tft.setTextDatum(MC_DATUM);
        
        int centerX = STATUS_AREA_X + (STATUS_AREA_WIDTH / 2);
        int centerY = AREA_LANE_INFO_Y + (AREA_LANE_INFO_HEIGHT / 2);
        tft.drawString(laneText, centerX, centerY);
        
        lastLaneInfo = laneText;
        laneAreaDirty = false;
    }
}

void DisplayManager::updateRoleInfo(const String& role, const String& event, const String& heat, uint8_t laneNumber) {
    String text;
    if (role == "starter") {
        text = String("Starter");
    } else {
        text = String("Lane\n") + String(laneNumber);
    }
    if (text != lastLaneInfo || laneAreaDirty) {
        tft.fillRect(STATUS_AREA_X, AREA_LANE_INFO_Y, STATUS_AREA_WIDTH, AREA_LANE_INFO_HEIGHT, COLOR_SIDEBAR_BG);
        tft.setTextFont(2);
        tft.setTextColor(TFT_WHITE, COLOR_SIDEBAR_BG);
        tft.setTextDatum(MC_DATUM);
        int centerX = STATUS_AREA_X + (STATUS_AREA_WIDTH / 2);
        int centerY = AREA_LANE_INFO_Y + (AREA_LANE_INFO_HEIGHT / 2);
        tft.drawString(text, centerX, centerY);
        lastLaneInfo = text;
        laneAreaDirty = false;
    }
}

void DisplayManager::updateBatteryDisplay(float voltage, uint8_t percentage) {
    String batteryText = "Battery\n" + String(percentage) + "%";
    
    if (batteryText != lastBatteryString || batteryAreaDirty) {
        // Clear with sidebar background
        tft.fillRect(STATUS_AREA_X, AREA_BATTERY_STATUS_Y, STATUS_AREA_WIDTH, AREA_BATTERY_STATUS_HEIGHT, COLOR_SIDEBAR_BG);
        
        tft.setTextFont(2);  // Use larger font for better visibility
        uint16_t color = (percentage > 20) ? TFT_WHITE : COLOR_ERROR;
        tft.setTextColor(color, COLOR_SIDEBAR_BG);
        tft.setTextDatum(MC_DATUM);
        
        int centerX = STATUS_AREA_X + (STATUS_AREA_WIDTH / 2);
        int centerY = AREA_BATTERY_STATUS_Y + (AREA_BATTERY_STATUS_HEIGHT / 2);
        tft.drawString(batteryText, centerX, centerY);
        
        lastBatteryString = batteryText;
        batteryAreaDirty = false;
    }
}

void DisplayManager::clearStatusAreas() {
    // Fill entire sidebar with swimming pool background
    tft.fillRect(STATUS_AREA_X, 0, STATUS_AREA_WIDTH, DISPLAY_HEIGHT, COLOR_SIDEBAR_BG);
    
    lastWiFiStatus = "";
    lastWebSocketStatus = "";
    lastLaneInfo = "";
    lastBatteryString = "";
    wifiAreaDirty = true;
    websocketAreaDirty = true;
    laneAreaDirty = true;
    batteryAreaDirty = true;
}

// ===============================
// Utility Functions
// ===============================

// showGeneralStatus and showConfigPortalInfo are kept as they're still used in main.cpp
