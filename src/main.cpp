/**
 * @file main.cpp
 * @brief SwimWatch — High-precision swim meet split timer
 *
 * Hardware: LilyGO T-Display S3 (ESP32-S3R8)
 *   - GPIO0:  Start/Stop toggle (onboard BUTTON1)
 *   - GPIO14: Reset (onboard BUTTON2, only when stopped)
 *   - GPIO2:  Split / Start-send (external, active LOW)
 *   - Display: ST7789V 320x170 via TFT_eSPI
 *
 * Startup flow:
 *   1. Check NVS for stored WiFi credentials
 *   2. If found → WiFi connect → NTP sync (5s timeout) → WebSocket connect
 *   3. If missing / failed → captive portal (SwimWatch-Setup AP)
 *
 * Timing:
 *   - Elapsed: esp_timer_get_time() via StopwatchTimer (1µs, NTP-independent)
 *   - Wall-clock: time()/getLocalTime() synced by NTPManager every 60s
 *   - Never millis() for precision timing — only for scheduling
 */

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"
#include "ntp_manager.h"
#include "captive_portal.h"
#include "display_manager.h"
#include "button_manager.h"
#include "websocket_stopwatch.h"

// ── Global module instances ────────────────────────────────────
CaptivePortalManager* captivePortal = nullptr;
DisplayManager  display;
ButtonManager   buttons;
WebSocketStopwatch stopwatch;
NTPManager      ntpManager;

// Application state
enum AppMode {
    MODE_SETUP,     // Captive portal mode
    MODE_NORMAL     // Normal stopwatch operation
};

AppMode currentMode = MODE_SETUP;
bool systemInitialized = false;

// Split time tracking for display (last 3 splits)
struct SplitTimeDisplay {
    uint8_t  splitNumber;
    uint32_t totalTime;        // Total elapsed ms
    String   formattedTime;    // Formatted total time
    uint32_t lapTime;          // This lap interval ms
    String   formattedLapTime; // Formatted lap interval
    bool     valid;
};

SplitTimeDisplay lastSplits[3] = {
    {0, 0, "", 0, "", false},
    {0, 0, "", 0, "", false},
    {0, 0, "", 0, "", false}
};

// ── Configuration loaded from NVS Preferences ─────────────────
StopwatchConfig config;

// ── Forward declarations ───────────────────────────────────────
void loadConfiguration();
void setupMode();
void normalMode();
void initializeNormalOperation();
void handleButtonEvents();
void updateDisplay();
void checkConnections();
void clearSplitDisplay();

// Scheduling (millis-based, non-precision)
unsigned long lastDisplayUpdate = 0;
unsigned long lastStatusUpdate  = 0;

// ── Stopwatch event callbacks ──────────────────────────────────
void onStopwatchStateChanged(StopwatchState newState);
void onLapAdded(uint8_t lapNumber, uint32_t lapTime, uint32_t totalTime);
void onConnectionChanged(bool connected);
void onEventHeatChanged(const String& event, const String& heat);
void onSplitTimeReceived(uint8_t lane, const String& time);
void onDisplayClear();
void onDeviceConfigChanged(const String& role, uint8_t lane);

void setup() {
    // IO15 must be HIGH before starting — otherwise display won't work on battery
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);
    
    Serial.begin(115200);
    Serial.println("=== SwimWatch Starting ===");
    
    // Initialize display first for user feedback
    if (!display.init()) {
        Serial.println("FATAL: Display initialization failed!");
        while (true) delay(1000);
    }
    
    // Check if we have stored WiFi credentials
    if (CaptivePortalManager::hasStoredCredentials()) {
        DEBUG_LOG("Found stored WiFi credentials");
        display.showSplashScreen();
        display.showStartupMessage("Connecting to WiFi...");
        
        if (CaptivePortalManager::connectWithStoredCredentials()) {
            Serial.println("WiFi connected!");
            currentMode = MODE_NORMAL;
            loadConfiguration();

            // Start NTP sync (non-blocking background task)
            display.showStartupMessage("Syncing time (NTP)...");
            ntpManager.begin(config.getEffectiveNTPServer());
            ntpManager.waitForSync(NTP_INITIAL_SYNC_TIMEOUT_MS);
            if (ntpManager.isSynced()) {
                Serial.println("NTP synced");
            } else {
                Serial.println("NTP sync timeout — continuing without sync");
            }

            initializeNormalOperation();
        } else {
            Serial.println("WiFi connect failed, starting captive portal...");
            currentMode = MODE_SETUP;
            setupMode();
        }
    } else {
        Serial.println("No WiFi credentials, starting captive portal...");
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
    Preferences prefs;
    prefs.begin("stopwatch", true);  // read-only
    
    config.serverIP   = prefs.getString("ws_server", DEFAULT_SERVER_IP);
    config.serverPort = prefs.getUInt("ws_port", DEFAULT_SERVER_PORT);
    config.laneNumber = prefs.getUInt("lane", 9);
    config.useSSL     = (config.serverPort == 443);
    config.role       = prefs.getString("role", "lane");
    config.ntpServer  = prefs.getString("ntp_server", "");
    
    prefs.end();
    
    Serial.printf("Config — Server: %s:%d, Role: %s, Lane: %d, NTP: %s\n",
                  config.serverIP.c_str(), config.serverPort, config.role.c_str(),
                  config.laneNumber, config.getEffectiveNTPServer());
}

void setupMode() {
    display.showSplashScreen();
    display.showStartupMessage("Setup Mode");
    display.showConfigPortalInfo(AP_SSID, AP_PASSWORD);
    
    captivePortal = new CaptivePortalManager();
    if (!captivePortal->begin()) {
        Serial.println("FATAL: Failed to start captive portal!");
        display.showStartupMessage("Setup Failed!");
        while (true) delay(1000);
    }
    
    Serial.printf("Captive portal started — SSID: %s, Pass: %s\n", AP_SSID, AP_PASSWORD);
}

void initializeNormalOperation() {
    // Initialize button manager (GPIO2 split trigger)
    if (!buttons.init()) {
        Serial.println("ERROR: Button initialization failed!");
        display.showGeneralStatus("Button init failed!", COLOR_ERROR);
        delay(3000);
    }
    
    // Setup display layout
    display.clearScreen();
    display.drawBorders();
    if (config.role == "starter") {
        display.setEventHeat("1", "1");
        display.updateRoleInfo(config.role, String(""), String(""), config.laneNumber);
    } else {
        display.updateLaneInfo(config.laneNumber);
    }
    display.updateWiFiStatus("Connected", true, WiFi.RSSI());
    
    // Wire up stopwatch event callbacks
    stopwatch.onStateChanged       = onStopwatchStateChanged;
    stopwatch.onLapAdded           = onLapAdded;
    stopwatch.onConnectionChanged  = onConnectionChanged;
    stopwatch.onEventHeatChanged   = onEventHeatChanged;
    stopwatch.onSplitTimeReceived  = onSplitTimeReceived;
    stopwatch.onDisplayClear       = onDisplayClear;
    stopwatch.onDeviceConfigChanged = onDeviceConfigChanged;
    
    // Connect to WebSocket server
    display.showStartupMessage("Connecting to server...");
    stopwatch.setServerConfig(config.serverIP, config.serverPort, DEFAULT_WS_PATH, config.useSSL);
    stopwatch.setLaneNumber(config.laneNumber);
    stopwatch.setDeviceRole(config.role);
    
    if (stopwatch.connect()) {
        DEBUG_LOG("WebSocket connection initiated");
        display.updateWebSocketStatus("Connecting...", false);
    } else {
        Serial.println("WebSocket connection failed");
        display.updateWebSocketStatus("Failed", false);
    }
    
    display.clearStartupMessage();
    display.updateStopwatchDisplay(0, false);
    
    systemInitialized = true;
    Serial.println("Normal operation ready");
}

void normalMode() {
    unsigned long now = millis();
    
    // Handle hardware buttons (highest priority — no blocking)
    handleButtonEvents();
    
    // Process WebSocket communication
    stopwatch.loop();
    
    // Update display at 20fps (every 50ms per config)
    if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL_MS) {
        updateDisplay();
        lastDisplayUpdate = now;
    }
    
    // Update status info (WiFi, WS, NTP) at 1Hz
    if (now - lastStatusUpdate >= STATUS_UPDATE_INTERVAL_MS) {
        checkConnections();
        lastStatusUpdate = now;
    }
}

void handleButtonEvents() {
    if (!systemInitialized) return;
    
    ButtonEvent event = buttons.getButtonEvent();
    if (event == BUTTON_NONE) return;
    
    switch (event) {
        case BUTTON_START_STOP:
            // GPIO0: Toggle start/stop
            if (stopwatch.getState() == STOPWATCH_RUNNING) {
                stopwatch.stop();
                DEBUG_LOG("Button → stop");
            } else {
                stopwatch.start();
                DEBUG_LOG("Button → start");
            }
            break;

        case BUTTON_RESET:
            // GPIO14: Split when running, Reset when stopped
            if (stopwatch.getState() == STOPWATCH_RUNNING) {
                stopwatch.addLap();
                DEBUG_LOG("Button → split (BUTTON2 while running)");
            } else {
                stopwatch.reset();
                clearSplitDisplay();
                DEBUG_LOG("Button → reset");
            }
            break;

        case BUTTON_LAP_PRESSED:
            // GPIO2: Split (lane) or send start (starter)
            if (config.role == "starter") {
                String ev = stopwatch.getCurrentEvent();
                String ht = stopwatch.getCurrentHeat();
                if (ev.isEmpty()) ev = "1";
                if (ht.isEmpty()) ht = "1";
                stopwatch.sendStart(ev, ht);
                DEBUG_LOG("Starter button → start sent");
            } else {
                if (stopwatch.getState() == STOPWATCH_RUNNING) {
                    stopwatch.addLap();
                    DEBUG_LOG("Split recorded via button");
                } else {
                    DEBUG_LOG("Button pressed — stopwatch not running");
                }
            }
            break;

        default:
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
    // WiFi status
    if (WiFi.status() != WL_CONNECTED) {
        display.updateWiFiStatus("Disconnected", false);
    } else {
        display.updateWiFiStatus("Connected", true, WiFi.RSSI());
    }
    
    // WebSocket status
    if (stopwatch.isConnected()) {
        display.updateWebSocketStatus("Connected", true, stopwatch.getPingMs());
    } else {
        display.updateWebSocketStatus("Disconnected", false);
    }

    // NTP clock in sidebar
    if (ntpManager.isSynced()) {
        char timeBuf[9];
        ntpManager.getFormattedTime(timeBuf, sizeof(timeBuf));
        display.updateNtpClock(String(timeBuf), true);
    } else {
        display.updateNtpClock("--:--:--", false);
    }

    // Battery percentage (GPIO4 ADC with 1:2 voltage divider)
    uint32_t rawSum = 0;
    for (int i = 0; i < BATTERY_SAMPLES; i++) {
        rawSum += analogRead(PIN_BATTERY_ADC);
    }
    float voltage = (rawSum / (float)BATTERY_SAMPLES) * 2.0f * 3.3f / 4095.0f;
    uint8_t battPct = constrain(
        (int)((voltage - BATTERY_MIN_VOLTAGE) / (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE) * 100.0f),
        0, 100);
    display.updateBatteryDisplay(battPct);
}

// ── Stopwatch event callbacks ──────────────────────────────────

void onStopwatchStateChanged(StopwatchState newState) {
    if (newState == STOPWATCH_STOPPED) {
        clearSplitDisplay();
    }
    DEBUG_LOG("Stopwatch state: %d", newState);
}

void onLapAdded(uint8_t lapNumber, uint32_t lapTime, uint32_t totalTime) {
    String totalFormatted = stopwatch.formatTime(totalTime);
    String lapFormatted   = stopwatch.formatTime(lapTime);
    DEBUG_LOG("Split %d: total=%s lap=%s", lapNumber, totalFormatted.c_str(), lapFormatted.c_str());
    
    // Rolling display — newest split always at the top (row 1)
    lastSplits[2] = lastSplits[1];
    lastSplits[1] = lastSplits[0];
    lastSplits[0] = {lapNumber, totalTime, totalFormatted, lapTime, lapFormatted, true};
    
    for (int i = 0; i < 3; i++) {
        if (lastSplits[i].valid) {
            // One line: "#1  01:23.45  +00:25.30"
            String line = "#" + String(lastSplits[i].splitNumber)
                        + "  " + lastSplits[i].formattedTime
                        + "  +" + lastSplits[i].formattedLapTime;
            display.updateLapTime(i + 1, line);
        } else {
            display.updateLapTime(i + 1, "");
        }
    }
}

void onConnectionChanged(bool connected) {
    DEBUG_LOG("WebSocket %s", connected ? "connected" : "disconnected");
    display.updateWebSocketStatus(connected ? "Connected" : "Disconnected",
                                  connected, connected ? stopwatch.getPingMs() : 0);
}

void onEventHeatChanged(const String& event, const String& heat) {
    DEBUG_LOG("Event/Heat: %s/%s", event.c_str(), heat.c_str());
    if (config.role == "starter") {
        display.setEventHeat(event, heat);
    }
}

void onSplitTimeReceived(uint8_t lane, const String& time) {
    DEBUG_LOG("Lane %d split: %s", lane, time.c_str());
}

void onDisplayClear() {
    display.clearLapTimes();
    clearSplitDisplay();
    DEBUG_LOG("Display cleared");
}

void onDeviceConfigChanged(const String& role, uint8_t lane) {
    config.role = role;
    config.laneNumber = lane;
    
    // Persist to NVS
    Preferences prefs;
    prefs.begin("stopwatch", false);
    prefs.putString("role", role);
    prefs.putUInt("lane", lane);
    prefs.end();
    
    DEBUG_LOG("Config saved — Role: %s, Lane: %d", role.c_str(), lane);
    
    if (role == "starter") {
        display.updateRoleInfo(role, stopwatch.getCurrentEvent(), stopwatch.getCurrentHeat(), lane);
    } else {
        display.updateLaneInfo(lane);
    }
}

void clearSplitDisplay() {
    for (int i = 0; i < 3; i++) {
        lastSplits[i] = {0, 0, "", 0, "", false};
        display.updateLapTime(i + 1, "");
    }
}
