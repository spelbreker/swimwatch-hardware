/**
 * Energy Manager Implementation for T-Display S3 Stopwatch
 * 
 * Handles power management, sleep functionality, and activity tracking
 * for battery-powered operation with automatic sleep/wake capabilities.
 */

#include "energy_manager.h"
#include <esp_wifi.h>
#include <esp_bt.h>
#include <WiFi.h>
#include <driver/adc.h>

// Correct backlight pin for T-Display S3 (see LilyGO pin_config.h: PIN_LCD_BL = 38)
#ifndef BACKLIGHT_PIN
#define BACKLIGHT_PIN 38
#endif

EnergyManager::EnergyManager(DisplayManager& displayMgr) 
    : display(displayMgr)
    , sleepTimeoutMs(SLEEP_TIMEOUT_TEST)
    , sleepEnabled(true)
    , testMode(false)
    , lowPowerMode(false)
    , lastActivityTime(0) {
}

bool EnergyManager::init(bool testMode) {
    this->testMode = testMode;
    this->sleepTimeoutMs = testMode ? SLEEP_TIMEOUT_TEST : SLEEP_TIMEOUT_NORMAL;
    Serial.printf("EnergyManager initialized - Test mode: %s, Sleep timeout: %d seconds (light sleep only)\n", 
                  testMode ? "ON" : "OFF", sleepTimeoutMs / 1000);
    updateActivityTimer();
    return true;
}

// Deep sleep configuration functions removed (light sleep path only)

void EnergyManager::updateActivityTimer() {
    lastActivityTime = millis();
    Serial.println("Activity timer updated");
}

bool EnergyManager::checkSleepTimeout() const {
    if (!sleepEnabled) {
        return false;
    }
    
    unsigned long currentTime = millis();
    
    // Handle millis() overflow (occurs approximately every 50 days)
    if (currentTime < lastActivityTime) {
        return false; // Don't sleep during overflow
    }
    
    unsigned long timeSinceActivity = currentTime - lastActivityTime;
    return (timeSinceActivity >= getCurrentSleepTimeout());
}

void EnergyManager::setSleepEnabled(bool enabled) {
    sleepEnabled = enabled;
    Serial.printf("Sleep %s\n", enabled ? "enabled" : "disabled");
}

bool EnergyManager::isSleepEnabled() const {
    return sleepEnabled;
}

void EnergyManager::enterLightSleep() {
    Serial.println("Preparing to enter LIGHT SLEEP (GPIO2 wake)...");

    // Stop network stacks to reduce power
    Serial.println("Disabling WiFi and BT...");
    WiFi.mode(WIFI_OFF);
    btStop();

    // Turn off display/backlight (do not rely on deep sleep hold)
    pinMode(BACKLIGHT_PIN, OUTPUT);
    digitalWrite(BACKLIGHT_PIN, LOW);
    // Put panel into sleep
    display.sendTFTCommand(0x28); // DISPOFF
    // Deep sleep functions removed
    
    // Restore display brightness
    display.setBrightness(255);  // Full brightness
    
    // Clear and reinitialize display
    display.clearScreen();
    
    Serial.println("Display restored successfully");
}

void EnergyManager::setLowPowerMode(bool enabled) {
    lowPowerMode = enabled;
    Serial.printf("Low power mode %s\n", enabled ? "enabled" : "disabled");
}

void EnergyManager::disableUnusedPeripherals() {
    Serial.println("Disabling unused peripherals for deep sleep...");
    
    // Gracefully disconnect WiFi without aggressive deinit
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect(true);
    }
    WiFi.mode(WIFI_OFF);
    
    // Don't call esp_wifi_deinit() as it can cause crashes
    // esp_wifi_stop() is sufficient for deep sleep
    esp_wifi_stop();
    
    // Disable Bluetooth if enabled
    esp_bt_controller_disable();
    
    Serial.println("Peripherals disabled for deep sleep");
}

void EnergyManager::enableLightSleep() {
    Serial.println("Enabling light sleep mode...");
    esp_sleep_enable_gpio_wakeup();
    esp_light_sleep_start();
}

unsigned long EnergyManager::getTimeSinceLastActivity() const {
    unsigned long currentTime = millis();
    
    // Handle millis() overflow
    if (currentTime < lastActivityTime) {
        return 0;
    }
    
    return currentTime - lastActivityTime;
}

unsigned long EnergyManager::getTimeUntilSleep() const {
    if (!sleepEnabled) {
        return UINT32_MAX; // Never sleep
    }
    
    unsigned long timeSinceActivity = getTimeSinceLastActivity();
    uint32_t timeout = getCurrentSleepTimeout();
    
    if (timeSinceActivity >= timeout) {
        return 0; // Should sleep now
    }
    
    return timeout - timeSinceActivity;
}

bool EnergyManager::shouldShowSleepWarning() const {
    if (!sleepEnabled) {
        return false;
    }
    
    unsigned long timeUntilSleep = getTimeUntilSleep();
    return (timeUntilSleep > 0 && timeUntilSleep <= SLEEP_WARNING_THRESHOLD);
}

float EnergyManager::getBatteryVoltage() const {
    // Read battery voltage from ADC (if available)
    // This is a placeholder implementation
    // You'll need to implement actual ADC reading based on your hardware
    
    // For T-Display S3, you might need to configure GPIO4 as ADC input
    // and use voltage divider calculations
    
    return 3.7f; // Placeholder value
}

uint8_t EnergyManager::getBatteryPercentage() const {
    float voltage = getBatteryVoltage();
    
    // Simple linear mapping from voltage to percentage
    if (voltage <= BATTERY_MIN_VOLTAGE) {
        return 0;
    }
    if (voltage >= BATTERY_MAX_VOLTAGE) {
        return 100;
    }
    
    float percentage = ((voltage - BATTERY_MIN_VOLTAGE) / 
                       (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE)) * 100.0f;
    
    return static_cast<uint8_t>(percentage);
}

bool EnergyManager::isLowBattery() const {
    return getBatteryVoltage() < (BATTERY_MIN_VOLTAGE + 0.2f); // 20% threshold
}

uint32_t EnergyManager::getCurrentSleepTimeout() const {
    return sleepTimeoutMs;
}