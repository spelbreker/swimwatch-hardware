#include "display_manager.h"

DisplayManager::DisplayManager() 
    : stopwatchAreaDirty(true)
    , wifiAreaDirty(true) 
    , websocketAreaDirty(true)
    , laneAreaDirty(true)
    , batteryAreaDirty(true)
    , lapAreaDirty(true)
    , timeFont(6)
    , statusFont(1)
    , lapFont(2) {
}

bool DisplayManager::init() {
    Serial.println("Initializing display...");
    
    tft.init();
    tft.setRotation(1); // Landscape orientation
    
    // Set default colors and fonts
    clearScreen();
    
    // Draw the layout borders
    drawBorders();
    
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
    tft.fillScreen(COLOR_BACKGROUND);
    
    // Clear all cached strings
    lastTimeString = "";
    lastWiFiStatus = "";
    lastWebSocketStatus = "";
    lastLaneInfo = "";
    lastBatteryString = "";
    lastLap1 = "";
    lastLap2 = "";
    lastLap3 = "";
    lastStartupMessage = "";
    
    // Mark all areas as dirty
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
    
    tft.drawString("T-Display S3", DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2 - 20);
    tft.setTextFont(2);
    tft.setTextColor(COLOR_STATUS, COLOR_BACKGROUND);
    tft.drawString("Stopwatch System", DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2 + 10);
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
    // Use WiFi status area for general status messages
    String statusText = message;
    
    if (statusText != lastWiFiStatus || wifiAreaDirty) {
        clearArea(STATUS_AREA_X, AREA_WIFI_STATUS_Y, STATUS_AREA_WIDTH, AREA_WIFI_STATUS_HEIGHT);
        
        tft.setTextFont(statusFont);
        tft.setTextColor(color, COLOR_BACKGROUND);
        tft.setTextDatum(MC_DATUM);
        
        int centerX = STATUS_AREA_X + (STATUS_AREA_WIDTH / 2);
        int centerY = AREA_WIFI_STATUS_Y + (AREA_WIFI_STATUS_HEIGHT / 2);
        tft.drawString(statusText, centerX, centerY);
        
        lastWiFiStatus = statusText;
        wifiAreaDirty = false;
    }
}

void DisplayManager::clearStatus() {
    if (!lastWiFiStatus.isEmpty() || wifiAreaDirty) {
        clearArea(STATUS_AREA_X, AREA_WIFI_STATUS_Y, STATUS_AREA_WIDTH, AREA_WIFI_STATUS_HEIGHT);
        lastWiFiStatus = "";
        wifiAreaDirty = false;
    }
}

void DisplayManager::updateLapDisplay(const uint8_t* laps, uint8_t lapCount, uint8_t maxDisplay) {
    // This function displays laps in the dedicated lap areas
    clearLapTimes(); // Clear all lap areas first
    
    // Display up to 3 laps in the dedicated areas
    for (uint8_t i = 0; i < lapCount && i < 3; i++) {
        String lapTime = String(laps[i] / 60) + ":" + 
                        String((laps[i] % 60) / 10) + String((laps[i] % 60) % 10);
        updateLapTime(i + 1, lapTime);
    }
    
    lapAreaDirty = false;
}

void DisplayManager::clearLaps() {
    clearLapTimes();
}

void DisplayManager::showConnectionInfo(const String& ssid, const IPAddress& ip) {
    // Show connection info in the WiFi status area
    String connectionText = "Connected\\n" + ssid;
    updateWiFiStatus(connectionText, true);
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

// New layout-specific functions
void DisplayManager::drawBorders() {
    // Draw vertical line separating main area from status area
    tft.drawFastVLine(STATUS_AREA_X - 1, 0, DISPLAY_HEIGHT, COLOR_STATUS);
    
    // Draw horizontal lines in status area
    tft.drawFastHLine(STATUS_AREA_X, AREA_WIFI_STATUS_HEIGHT, STATUS_AREA_WIDTH, COLOR_STATUS);
    tft.drawFastHLine(STATUS_AREA_X, AREA_WIFI_STATUS_HEIGHT + AREA_WEBSOCKET_STATUS_HEIGHT, STATUS_AREA_WIDTH, COLOR_STATUS);
    tft.drawFastHLine(STATUS_AREA_X, AREA_WIFI_STATUS_HEIGHT + AREA_WEBSOCKET_STATUS_HEIGHT + AREA_LANE_INFO_HEIGHT, STATUS_AREA_WIDTH, COLOR_STATUS);
    
    // Draw horizontal lines in main area for laps
    tft.drawFastHLine(MAIN_AREA_X, AREA_STOPWATCH_HEIGHT, MAIN_AREA_WIDTH, COLOR_STATUS);
    tft.drawFastHLine(MAIN_AREA_X, AREA_LAP1_Y + AREA_LAP1_HEIGHT, MAIN_AREA_WIDTH, COLOR_STATUS);
    tft.drawFastHLine(MAIN_AREA_X, AREA_LAP2_Y + AREA_LAP2_HEIGHT, MAIN_AREA_WIDTH, COLOR_STATUS);
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
}

void DisplayManager::showStartupMessage(const String& message) {
    if (message != lastStartupMessage || stopwatchAreaDirty) {
        // Clear stopwatch area
        clearArea(MAIN_AREA_X, AREA_STOPWATCH_Y, MAIN_AREA_WIDTH, AREA_STOPWATCH_HEIGHT);
        
        tft.setTextFont(4);
        tft.setTextColor(COLOR_STATUS, COLOR_BACKGROUND);
        tft.setTextDatum(MC_DATUM);
        
        int centerX = MAIN_AREA_X + (MAIN_AREA_WIDTH / 2);
        int centerY = AREA_STOPWATCH_Y + (AREA_STOPWATCH_HEIGHT / 2);
        tft.drawString(message, centerX, centerY);
        
        lastStartupMessage = message;
        stopwatchAreaDirty = false;
    }
}

void DisplayManager::clearStartupMessage() {
    lastStartupMessage = "";
    stopwatchAreaDirty = true;
}

void DisplayManager::updateLapTime(uint8_t lapNumber, const String& time) {
    String lapText = "Lap " + String(lapNumber) + ": " + time;
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
    
    if (lapText != *lastLap || lapAreaDirty) {
        // Clear lap area
        clearArea(MAIN_AREA_X, yPos, MAIN_AREA_WIDTH, AREA_LAP1_HEIGHT);
        
        tft.setTextFont(lapFont);
        tft.setTextColor(COLOR_LAP_INFO, COLOR_BACKGROUND);
        tft.setTextDatum(ML_DATUM);
        tft.drawString(lapText, MAIN_AREA_X + 5, yPos + (AREA_LAP1_HEIGHT / 2));
        
        *lastLap = lapText;
    }
    lapAreaDirty = false;
}

void DisplayManager::clearLapTimes() {
    clearArea(MAIN_AREA_X, AREA_LAP1_Y, MAIN_AREA_WIDTH, AREA_LAP1_HEIGHT + AREA_LAP2_HEIGHT + AREA_LAP3_HEIGHT);
    lastLap1 = "";
    lastLap2 = "";
    lastLap3 = "";
    lapAreaDirty = false;
}

void DisplayManager::updateWiFiStatus(const String& status, bool isConnected, int rssi) {
    String wifiText;
    if (isConnected && rssi != 0) {
        wifiText = "WIFI\n" + String(rssi) + " dBm";
    } else {
        wifiText = "WIFI\n" + status;
    }
    
    if (wifiText != lastWiFiStatus || wifiAreaDirty) {
        clearArea(STATUS_AREA_X, AREA_WIFI_STATUS_Y, STATUS_AREA_WIDTH, AREA_WIFI_STATUS_HEIGHT);
        
        tft.setTextFont(statusFont);
        tft.setTextColor(isConnected ? COLOR_TIME_DISPLAY : COLOR_ERROR, COLOR_BACKGROUND);
        tft.setTextDatum(MC_DATUM);
        
        int centerX = STATUS_AREA_X + (STATUS_AREA_WIDTH / 2);
        int centerY = AREA_WIFI_STATUS_Y + (AREA_WIFI_STATUS_HEIGHT / 2);
        tft.drawString(wifiText, centerX, centerY);
        
        lastWiFiStatus = wifiText;
        wifiAreaDirty = false;
    }
}

void DisplayManager::updateWebSocketStatus(const String& status, bool isConnected, int pingMs) {
    String wsText;
    if (isConnected && pingMs >= 0) {
        wsText = "WS\n" + String(pingMs) + "ms";
    } else if (isConnected) {
        wsText = "WS\nConnected";
    } else {
        wsText = "WS\n" + status;
    }
    
    if (wsText != lastWebSocketStatus || websocketAreaDirty) {
        clearArea(STATUS_AREA_X, AREA_WEBSOCKET_STATUS_Y, STATUS_AREA_WIDTH, AREA_WEBSOCKET_STATUS_HEIGHT);
        
        tft.setTextFont(statusFont);
        tft.setTextColor(isConnected ? COLOR_TIME_DISPLAY : COLOR_WARNING, COLOR_BACKGROUND);
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
        clearArea(STATUS_AREA_X, AREA_LANE_INFO_Y, STATUS_AREA_WIDTH, AREA_LANE_INFO_HEIGHT);
        
        tft.setTextFont(statusFont + 1);
        tft.setTextColor(COLOR_STATUS, COLOR_BACKGROUND);
        tft.setTextDatum(MC_DATUM);
        
        int centerX = STATUS_AREA_X + (STATUS_AREA_WIDTH / 2);
        int centerY = AREA_LANE_INFO_Y + (AREA_LANE_INFO_HEIGHT / 2);
        tft.drawString(laneText, centerX, centerY);
        
        lastLaneInfo = laneText;
        laneAreaDirty = false;
    }
}

void DisplayManager::updateBatteryDisplay(float voltage, uint8_t percentage) {
    String batteryText = "Battery\n" + String(percentage) + "%";
    
    if (batteryText != lastBatteryString || batteryAreaDirty) {
        clearArea(STATUS_AREA_X, AREA_BATTERY_STATUS_Y, STATUS_AREA_WIDTH, AREA_BATTERY_STATUS_HEIGHT);
        
        tft.setTextFont(statusFont);
        uint16_t color = (percentage > 20) ? COLOR_STATUS : COLOR_ERROR;
        tft.setTextColor(color, COLOR_BACKGROUND);
        tft.setTextDatum(MC_DATUM);
        
        int centerX = STATUS_AREA_X + (STATUS_AREA_WIDTH / 2);
        int centerY = AREA_BATTERY_STATUS_Y + (AREA_BATTERY_STATUS_HEIGHT / 2);
        tft.drawString(batteryText, centerX, centerY);
        
        lastBatteryString = batteryText;
        batteryAreaDirty = false;
    }
}

void DisplayManager::clearStatusAreas() {
    clearArea(STATUS_AREA_X, 0, STATUS_AREA_WIDTH, DISPLAY_HEIGHT);
    lastWiFiStatus = "";
    lastWebSocketStatus = "";
    lastLaneInfo = "";
    lastBatteryString = "";
    wifiAreaDirty = true;
    websocketAreaDirty = true;
    laneAreaDirty = true;
    batteryAreaDirty = true;
}

// Legacy compatibility functions
void DisplayManager::showTimeZero() {
    updateStopwatchDisplay(0, false);
}

void DisplayManager::showBatteryStatus(float voltage, uint8_t percentage) {
    updateBatteryDisplay(voltage, percentage);
}

void DisplayManager::showWiFiStatus(const String& message, bool isError) {
    updateWiFiStatus(message, !isError);
}

void DisplayManager::showWebSocketStatus(const String& message, bool isConnected) {
    updateWebSocketStatus(message, isConnected);
}
