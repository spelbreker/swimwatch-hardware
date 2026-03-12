/**
 * @file config.h
 * @brief Central configuration for SwimWatch — LilyGO T-Display S3
 *
 * All hardware pin definitions, timing constants, network defaults,
 * and the application configuration struct live here. Every module
 * imports from this single header instead of defining its own constants.
 *
 * Hardware: LilyGO T-Display S3 (ESP32-S3R8)
 * Display:  1.9" 170x320 ST7789V IPS LCD, 8-bit parallel
 * Buttons:  GPIO0 (Start/Stop), GPIO14 (Reset), GPIO2 (Split, active LOW)
 */
#pragma once

#include <cstdint>
#include <Arduino.h>

// ============================================================
// Debug configuration
// Add -DDEBUG_STOPWATCH to build_flags in platformio.ini to enable
// ============================================================
#ifdef DEBUG_STOPWATCH
    #define DEBUG_LOG(fmt, ...) Serial.printf("[DBG] " fmt "\n", ##__VA_ARGS__)
#else
    #define DEBUG_LOG(fmt, ...) ((void)0)
#endif

// ============================================================
// Hardware pin definitions — LilyGO T-Display S3
// ============================================================
constexpr uint8_t PIN_BUTTON_START_STOP = 0;    // GPIO0  — onboard BUTTON1
constexpr uint8_t PIN_BUTTON_RESET      = 14;   // GPIO14 — onboard BUTTON2
constexpr uint8_t PIN_BUTTON_SPLIT      = 2;    // GPIO2  — external split trigger
constexpr uint8_t PIN_POWER_ON          = 15;   // Must be HIGH for battery operation
constexpr uint8_t PIN_BATTERY_ADC       = 4;    // GPIO4  — battery voltage (1:2 divider)

// Display pins are managed by TFT_eSPI via User_Setup.h:
// TFT_MOSI=19, TFT_SCLK=18, TFT_CS=5, TFT_DC=16, TFT_RST=23, TFT_BL=38

// ============================================================
// Battery constants
// ============================================================
constexpr float BATTERY_MAX_VOLTAGE     = 4.2f;  // Fully charged LiPo
constexpr float BATTERY_MIN_VOLTAGE     = 3.0f;  // Empty LiPo
constexpr uint8_t BATTERY_SAMPLES       = 16;    // ADC averaging

// ============================================================
// Timing constants
// ============================================================
constexpr uint32_t DISPLAY_UPDATE_INTERVAL_MS   = 50;     // 50ms = 20fps
constexpr uint32_t STATUS_UPDATE_INTERVAL_MS    = 1000;   // Status refresh 1Hz
constexpr uint32_t BUTTON_DEBOUNCE_MS           = 200;    // Software debounce
constexpr uint32_t NTP_SYNC_INTERVAL_MS         = 60000;  // NTP re-sync every 60s
constexpr uint32_t NTP_INITIAL_SYNC_TIMEOUT_MS  = 5000;   // Max wait for first sync
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS      = 10000;  // WiFi connection timeout

// ============================================================
// Network defaults
// Same IP serves as both WebSocket server and NTP server
// ============================================================
constexpr const char* DEFAULT_SERVER_IP   = "192.168.8.10";
constexpr uint16_t    DEFAULT_SERVER_PORT = 80;
constexpr const char* DEFAULT_WS_PATH    = "/ws";

// ============================================================
// Captive portal defaults
// ============================================================
constexpr const char* AP_SSID     = "SwimWatch-Setup";
constexpr const char* AP_PASSWORD = "swimwatch123";

// ============================================================
// Application configuration struct
// Loaded from NVS Preferences, configured via captive portal
// ============================================================
struct StopwatchConfig {
    String wifiSSID;
    String wifiPassword;
    String serverIP    = "192.168.8.10";   // WebSocket + NTP server
    uint16_t serverPort = 80;
    String ntpServer;                       // Empty = use serverIP
    uint8_t laneNumber  = 9;
    String role         = "lane";           // "lane" or "starter"
    bool useSSL         = false;

    /** Returns the effective NTP server (falls back to serverIP) */
    const char* getEffectiveNTPServer() const {
        return ntpServer.isEmpty() ? serverIP.c_str() : ntpServer.c_str();
    }
};
