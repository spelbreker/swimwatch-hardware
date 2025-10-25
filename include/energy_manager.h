/**
 * Energy Manager for T-Display S3 Stopwatch
 * 
 * Handles power management, sleep functionality, and activity tracking
 * for battery-powered operation with automatic sleep/wake capabilities.
 */

#pragma once

#include <Arduino.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include "display_manager.h"

class EnergyManager {
public:
    // Sleep timeout configurations (in milliseconds)
    static constexpr uint32_t SLEEP_TIMEOUT_TEST = 20 * 1000;      // 20 seconds for testing
    static constexpr uint32_t SLEEP_TIMEOUT_NORMAL = 20 * 60 * 1000; // 20 minutes for production
    
    // Wake-up pin definitions
    static constexpr gpio_num_t WAKEUP_PIN_BUTTON1 = GPIO_NUM_0;   // Onboard BUTTON1 
    static constexpr gpio_num_t WAKEUP_PIN_BUTTON2 = GPIO_NUM_14;  // Onboard BUTTON2
    static constexpr gpio_num_t WAKEUP_PIN_EXTERNAL = GPIO_NUM_2;  // External button
    
    // Constructor
    EnergyManager(DisplayManager& displayMgr);
    
    // Initialization and configuration
    bool init(bool testMode = false);
    
    // Activity tracking
    void updateActivityTimer();
    bool checkSleepTimeout() const;
    void setSleepEnabled(bool enabled);
    bool isSleepEnabled() const;
    
    // Sleep management
    void enterLightSleep();
    
    // Power management
    void setLowPowerMode(bool enabled);
    void disableUnusedPeripherals();
    void enableLightSleep();
    
    // Status and monitoring
    unsigned long getTimeSinceLastActivity() const;
    unsigned long getTimeUntilSleep() const;
    bool shouldShowSleepWarning() const;
    
    // Battery management (if available)
    float getBatteryVoltage() const;
    uint8_t getBatteryPercentage() const;
    bool isLowBattery() const;

private:
    DisplayManager& display;
    
    // Configuration
    uint32_t sleepTimeoutMs;
    bool sleepEnabled;
    bool testMode;
    bool lowPowerMode;
    
    // Activity tracking
    unsigned long lastActivityTime;
    
    // Sleep warning threshold (show warning in last minute)
    static constexpr uint32_t SLEEP_WARNING_THRESHOLD = 60 * 1000; // 60 seconds
    
    // Battery monitoring (if ADC is available)
    static constexpr int BATTERY_ADC_PIN = 4; // GPIO4 for battery voltage
    static constexpr float BATTERY_MIN_VOLTAGE = 3.0f;  // More realistic minimum for LiPo
    static constexpr float BATTERY_MAX_VOLTAGE = 4.2f;
    
    // Helper methods
    // Deep sleep helpers removed; only light sleep path retained
    uint32_t getCurrentSleepTimeout() const;
};