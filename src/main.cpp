/**
 * LilyGO T-Display S3 Stopwatch - Remote Split Timer
 * 
 * Hardware:
 * - BUTTON1 (GPIO0):  No local action (WebSocket start only)
 * - BUTTON2 (GPIO14): Reset (when stopped)
 * - BUTTON3 (GPIO2):  Create split time (when running)
 * - Display: ST7789V via TFT_eSPI
 * 
 * Functionality:
 * - Stopwatch starts ONLY via WebSocket server command
 * - GPIO2 button creates split times and sends to server
 * void onSplitTime(int lane, const String& time) {
    Serial.printf("Split time received for lane %d: %s\n", lane, time.c_str());
    // Split times are now handled by the onSplit function
    // This function can be used for additional processing if needed
}y shows last 3 split times in rolling fashion
 * - Reset via GPIO14 when stopped
 * - Uses internal ESP32 timer (millis()) for accurate timing
 * 
 * WebSocket Messages:
 * - Receives: {"type":"start","event":1,"heat":2,"timestamp":...} - Start stopwatch
 * - Receives: {"type":"reset","timestamp":...} - Reset stopwatch  
 * - Receives: {"type":"time_sync","server_time":...} - Time sync
 * - Receives: {"type":"pong","client_ping_time":...,"server_time":...} - Ping response
 * - Sends: {"type":"ping","time":...} - Time sync ping
 * - Sends: {"type":"split","lane":X,"timestamp":...} - Split time
 * 
 * Flow:
 * 1. Check if WiFi credentials exist in preferences
 * 2. If yes, try to connect to WiFi
 * 3. If no credentials or connection fails, start captive portal
 * 4. After successful WiFi setup, restart device
 * 5. On restart, proceed with normal stopwatch operation
 * 6. Wait for WebSocket start command to begin timing
 * 7. Use GPIO2 to create split times during operation
 */

#include <Arduino.h>
#include <Preferences.h>
#include "captive_portal.h"
#include "connectivity.h"
#include "display_manager.h"
#include "button_manager.h"
#include "websocket_stopwatch.h"
#include "energy_manager.h"

// Pin definitions for T-Display S3
#define PIN_POWER_ON                 15  // Power control pin - MUST be HIGH for battery operation
#define PIN_BUTTON_1                 0   // GPIO0 - Boot button
#define PIN_BUTTON_2                 14  // GPIO14 - Reset/Stop button  
#define PIN_LCD_BL                   38  // Backlight control

// Global module instances
CaptivePortalManager* captivePortal = nullptr;
ConnectivityManager connectivity;
DisplayManager display;
ButtonManager buttons;
WebSocketStopwatch stopwatch;
EnergyManager energyManager(display);

// Application state
enum AppMode {
    MODE_SETUP,     // Captive portal mode
    MODE_NORMAL     // Normal stopwatch operation
};

AppMode currentMode = MODE_SETUP;
bool systemInitialized = false;

// Split time tracking for display (last 3 splits)
struct SplitTimeDisplay {
    uint8_t splitNumber;
    uint32_t totalTime;
    String formattedTime;
    bool valid;
};

SplitTimeDisplay lastSplits[3] = {{0, 0, "", false}, {0, 0, "", false}, {0, 0, "", false}};

// Configuration loaded from preferences
struct AppConfig {
    String wsServer;
    uint16_t wsPort;
    uint8_t laneNumber;
    bool useSSL;
} config;

// Forward declarations
void loadConfiguration();
void setupMode();
void normalMode();
void initializeNormalOperation();
void handleButtonEvents();
void updateDisplay();
void checkConnections();
void clearSplitDisplay();

// Normal mode timing variables
unsigned long lastDisplayUpdate = 0;
unsigned long lastStatusUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 100;    // Update display every 100ms
const unsigned long STATUS_UPDATE_INTERVAL = 1000;    // Update status every second

// Callback functions for stopwatch events
void onStopwatchStateChanged(StopwatchState newState);
void onLapAdded(uint8_t lapNumber, uint32_t lapTime, uint32_t totalTime);
void onConnectionChanged(bool connected);
void onTimeSync(bool synced);
void onEventHeatChanged(const String& event, const String& heat);
void onSplitTimeReceived(uint8_t lane, const String& time);
void onDisplayClear();

void setup() {
    // (POWER ON)IO15 must be set to HIGH before starting, otherwise the screen will not display when using battery
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);
    
    Serial.begin(115200);
    Serial.println("\n=== T-Display S3 Stopwatch Starting ===");
    
    // Initialize display first for user feedback
    if (!display.init()) {
        Serial.println("FATAL: Display initialization failed!");
        while (true) delay(1000);
    }
    
    // Initialize energy management system (test mode enabled)
    if (!energyManager.init(false)) {
        Serial.println("ERROR: Energy manager initialization failed!");
    }
    
    // Check if we have stored WiFi credentials
    if (CaptivePortalManager::hasStoredCredentials()) {
        Serial.println("Found stored WiFi credentials, attempting connection...");
        display.showSplashScreen();
        display.showStartupMessage("Connecting to WiFi...");
        
        // Try to connect with stored credentials
        if (CaptivePortalManager::connectWithStoredCredentials()) {
            Serial.println("WiFi connected with stored credentials!");
            currentMode = MODE_NORMAL;
            loadConfiguration();
            initializeNormalOperation();
        } else {
            Serial.println("Failed to connect with stored credentials, starting captive portal...");
            currentMode = MODE_SETUP;
            setupMode();
        }
    } else {
        Serial.println("No stored WiFi credentials found, starting captive portal...");
        currentMode = MODE_SETUP;
        setupMode();
    }
    
    Serial.println("=== Setup Complete ===");
}

void loop() {
    switch (currentMode) {
        case MODE_SETUP:
            if (captivePortal) {
                captivePortal->loop();
                
                // Check if configuration is complete (device will restart automatically)
                if (captivePortal->isConfigComplete()) {
                    Serial.println("Configuration complete, restarting...");
                    delay(1000);
                    ESP.restart();
                }
            }
            break;
            
        case MODE_NORMAL:
            normalMode();
            break;
    }
}

void loadConfiguration() {
    Serial.println("Loading configuration from preferences...");
    
    Preferences prefs;
    prefs.begin("stopwatch", true);
    
    config.wsServer = prefs.getString("ws_server", "scherm.azckamp.nl");
    config.wsPort = prefs.getUInt("ws_port", 443);
    config.laneNumber = prefs.getUInt("lane", 9);
    config.useSSL = (config.wsPort == 443);
    
    prefs.end();
    
    Serial.printf("Config - Server: %s:%d, Lane: %d, SSL: %s\n", 
                  config.wsServer.c_str(), config.wsPort, config.laneNumber,
                  config.useSSL ? "yes" : "no");
}

void setupMode() {
    Serial.println("Starting captive portal setup mode...");
    
    // Show setup screen
    display.showSplashScreen();
    display.showStartupMessage("Setup Mode");
    display.showConfigPortalInfo("T-Display-S3-Setup", "stopwatch123");
    
    // Create and start captive portal
    captivePortal = new CaptivePortalManager();
    if (!captivePortal->begin()) {
        Serial.println("FATAL: Failed to start captive portal!");
        display.showStartupMessage("Setup Failed!");
        while (true) delay(1000);
    }
    
    Serial.println("Captive portal started successfully");
    Serial.println("Connect to WiFi: T-Display-S3-Setup (Password: stopwatch123)");
}

void initializeNormalOperation() {
    Serial.println("Initializing normal stopwatch operation...");
    
    // Initialize button manager
    if (!buttons.init()) {
        Serial.println("ERROR: Button initialization failed!");
        display.showGeneralStatus("Button init failed!", COLOR_ERROR);
        delay(3000);
    }
    
    // Setup display
    display.clearScreen();
    display.drawBorders();
    display.updateLaneInfo(config.laneNumber);
    int rssi = WiFi.RSSI();
    display.updateWiFiStatus("Connected", true, rssi);
    
    // Show battery status (you can implement actual battery reading later)
    display.updateBatteryDisplay(3.7f, 85);  // Example values - replace with actual battery reading
    
    // Setup stopwatch callbacks
    stopwatch.onStateChanged = onStopwatchStateChanged;
    stopwatch.onLapAdded = onLapAdded;
    stopwatch.onConnectionChanged = onConnectionChanged;
    stopwatch.onTimeSync = onTimeSync;
    stopwatch.onEventHeatChanged = onEventHeatChanged;
    stopwatch.onSplitTimeReceived = onSplitTimeReceived;
    stopwatch.onDisplayClear = onDisplayClear;
    

    
    // Initialize WebSocket connection
    display.showStartupMessage("Connecting to server...");
    stopwatch.setServerConfig(config.wsServer, config.wsPort, "/ws", config.useSSL);
    stopwatch.setLaneNumber(config.laneNumber);
    
    if (stopwatch.connect()) {
        Serial.println("WebSocket connection initiated");
        display.updateWebSocketStatus("Connecting...", false);
    } else {
        Serial.println("Failed to initiate WebSocket connection");
        display.updateWebSocketStatus("Failed", false);
    }
    
    // Clear startup message and show initial stopwatch display
    display.clearStartupMessage();
    display.updateStopwatchDisplay(0, false);
    
    systemInitialized = true;
    Serial.println("Normal operation initialized successfully");
}

void normalMode() {
    unsigned long now = millis();
    
    // Handle hardware buttons (highest priority)
    handleButtonEvents();
    
    // Process WebSocket communication (high priority)
    stopwatch.loop();
    
    // Update display at 10Hz (every 100ms)
    if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        updateDisplay();
        lastDisplayUpdate = now;
    }
    
    // Update status information periodically
    if (now - lastStatusUpdate >= STATUS_UPDATE_INTERVAL) {
        checkConnections();
        lastStatusUpdate = now;
    }
    
    // Check for sleep timeout (only when stopwatch is stopped and sleep is enabled)
    if (energyManager.isSleepEnabled() && 
        stopwatch.getState() == STOPWATCH_STOPPED && 
        energyManager.checkSleepTimeout()) {
        Serial.println("Sleep timeout reached, entering LIGHT sleep (GPIO2 wake)...");
        energyManager.enterLightSleep();
    }
    
    // Small delay to prevent excessive CPU usage
    delay(10);
}

void handleButtonEvents() {
    if (!systemInitialized) return;
    
    ButtonEvent event = buttons.getButtonEvent();
    
    switch (event) {
        case BUTTON_START_LAP_PRESSED:
            // GPIO0 - Not used for local control (only WebSocket start)
            energyManager.updateActivityTimer(); // Reset sleep timer on button press
            Serial.println("GPIO0 pressed - No local action (WebSocket start only)");
            break;
            
        case BUTTON_START_2_PRESSED:
            // GPIO2 - Create split time when running
            energyManager.updateActivityTimer(); // Reset sleep timer on button press
            if (stopwatch.getState() == STOPWATCH_RUNNING) {
                stopwatch.addLap();
                Serial.println("Split time created via GPIO2 button");
            } else {
                Serial.println("GPIO2 pressed - Stopwatch not running, no split time created");
            }
            break;
            
        case BUTTON_STOP_PRESSED:
            // GPIO14 - Reset functionality (when stopped)
            energyManager.updateActivityTimer(); // Reset sleep timer on button press
            if (stopwatch.getState() == STOPWATCH_STOPPED) {
                stopwatch.reset();
                Serial.println("Stopwatch reset via GPIO14 button");
            } else {
                Serial.println("GPIO14 pressed - Stopwatch running, no reset action");
            }
            break;
            
        case BUTTON_NONE:
        default:
            // No button event
            break;
    }
}

void updateDisplay() {
    if (!systemInitialized) return;
    
    // Update main stopwatch time display
    uint32_t elapsedTime = stopwatch.getElapsedTime();
    bool isRunning = (stopwatch.getState() == STOPWATCH_RUNNING);
    display.updateStopwatchDisplay(elapsedTime, isRunning);
    
    // Show status when not running
    if (!isRunning && elapsedTime == 0) {
        // Only show this when truly stopped/reset (not just paused)
        static unsigned long lastStatusToggle = 0;
        static bool showStatus = true;
        
        if (millis() - lastStatusToggle > 2000) { // Toggle every 2 seconds
            lastStatusToggle = millis();
            showStatus = !showStatus;
            
            if (showStatus) {
                display.showStartupMessage("Ready - Waiting for start...");
            } else {
                display.clearStartupMessage();
            }
        }
    } else {
        display.clearStartupMessage();
    }
}

void checkConnections() {
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi connection lost");
        display.updateWiFiStatus("Disconnected", false);
        // Could implement reconnection logic here
    } else {
        int rssi = WiFi.RSSI();
        display.updateWiFiStatus("Connected", true, rssi);
    }
    
    // Check WebSocket connection
    if (!stopwatch.isConnected()) {
        display.updateWebSocketStatus("Disconnected", false);
    } else {
        int ping = stopwatch.getPingMs();
        display.updateWebSocketStatus("Connected", true, ping);
    }
    
    // Update battery status (placeholder - implement actual battery reading)
    // For T-Display S3, you can read battery voltage via ADC
    static uint8_t fakeBatteryLevel = 85;
    display.updateBatteryDisplay(3.7f, fakeBatteryLevel);
}

// Callback functions
void onStopwatchStateChanged(StopwatchState newState) {
    Serial.printf("Stopwatch state changed to: %d\n", newState);
    
    switch (newState) {
        case STOPWATCH_STOPPED:
            Serial.println("Stopwatch stopped");
            // Clear split display when stopped/reset
            clearSplitDisplay();
            break;
        case STOPWATCH_RUNNING:
            Serial.println("Stopwatch started");
            break;
        case STOPWATCH_PAUSED:
            Serial.println("Stopwatch paused");
            break;
    }
}

void onLapAdded(uint8_t lapNumber, uint32_t lapTime, uint32_t totalTime) {
    Serial.printf("Split %d added - Total time: %s\n", 
                  lapNumber, 
                  stopwatch.formatTime(totalTime).c_str());
    
    // Shift existing splits up to make room for new one
    lastSplits[0] = lastSplits[1];
    lastSplits[1] = lastSplits[2];
    
    // Add new split to the end
    lastSplits[2] = {lapNumber, totalTime, stopwatch.formatTime(totalTime), true};
    
    // Update display with last 3 splits
    for (int i = 0; i < 3; i++) {
        if (lastSplits[i].valid) {
            String displayText = "Split - " + String(lastSplits[i].splitNumber) + ": " + lastSplits[i].formattedTime;
            display.updateLapTime(i + 1, displayText);
        } else {
            display.updateLapTime(i + 1, "");
        }
    }
}

void onConnectionChanged(bool connected) {
    if (connected) {
        Serial.println("WebSocket connected successfully");
        int ping = stopwatch.getPingMs();
        display.updateWebSocketStatus("Connected", true, ping);
    } else {
        Serial.println("WebSocket disconnected");
        display.updateWebSocketStatus("Disconnected", false);
    }
}

void onTimeSync(bool synced) {
    if (synced) {
        Serial.println("Time synchronized with server");
    } else {
        Serial.println("Time synchronization lost");
    }
}

void onEventHeatChanged(const String& event, const String& heat) {
    Serial.printf("Event/Heat changed: %s / %s\n", event.c_str(), heat.c_str());
    // Event/heat info could be shown in one of the lap areas or as a temporary message
}

void onSplitTimeReceived(uint8_t lane, const String& time) {
    Serial.printf("Split time received for lane %d: %s\n", lane, time.c_str());
    // Split times are now handled by the onSplit function which shows them properly
    // This function is for receiving individual split times from other lanes
}

void onDisplayClear() {
    Serial.println("Display clear requested");
    display.clearLapTimes();
    clearSplitDisplay();
}

void clearSplitDisplay() {
    // Clear the split times tracking
    for (int i = 0; i < 3; i++) {
        lastSplits[i] = {0, 0, "", false};
        display.updateLapTime(i + 1, "");
    }
    Serial.println("Split display cleared");
}
