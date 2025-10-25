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
#include <esp_adc_cal.h>  // For ADC calibration

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
    
    // Configure ADC for battery voltage reading - use same settings as official LilyGO example
    analogReadResolution(12); // Set ADC resolution to 12 bits (0-4095)
    analogSetAttenuation(ADC_11db); // Set attenuation to 11dB (allows reading up to ~3.3V input)
    
    Serial.printf("EnergyManager initialized - Test mode: %s, Sleep timeout: %d seconds (light sleep only)\n", 
                  testMode ? "ON" : "OFF", sleepTimeoutMs / 1000);
    Serial.println("ADC configured for battery voltage reading (12-bit, 11dB attenuation)");
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
    // Use official LilyGO method for T-Display S3 battery voltage reading
    // Based on: https://github.com/Xinyuan-LilyGO/T-Display-S3/blob/main/examples/GetBatteryVoltage/GetBatteryVoltage.ino
    
    esp_adc_cal_characteristics_t adc_chars;
    
    // Get the internal calibration value of the chip
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    
    // Read raw ADC value
    uint32_t raw = analogRead(BATTERY_ADC_PIN);
    
    // Convert raw value to calibrated voltage in mV, then to volts
    // The partial pressure is one-half (voltage divider), so multiply by 2
    uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(raw, &adc_chars) * 2;
    float voltage = voltage_mv / 1000.0f; // Convert mV to V
    
    // Debug output
    Serial.printf("ADC Raw: %d, Calibrated voltage: %.3fV\n", raw, voltage);
    
    // Check if battery is connected (LilyGO logic)
    if (voltage_mv > 4300) {
        Serial.println("Warning: No battery detected or charging voltage detected");
        // Return a reasonable battery voltage when charging/no battery
        return 4.0f;
    }
    
    // Clamp to reasonable LiPo range
    if (voltage < 3.0f) voltage = 3.0f;
    if (voltage > 4.3f) voltage = 4.3f;
    
    return voltage;
}

uint8_t EnergyManager::getBatteryPercentage() const {
    float voltage = getBatteryVoltage();
    
    Serial.printf("Battery voltage for percentage calculation: %.3fV\n", voltage);
    
    // More realistic LiPo battery voltage to percentage mapping
    // LiPo batteries have a non-linear discharge curve
    if (voltage <= BATTERY_MIN_VOLTAGE) {
        Serial.println("Battery at minimum voltage - 0%");
        return 0;
    }
    if (voltage >= BATTERY_MAX_VOLTAGE) {
        Serial.println("Battery at maximum voltage - 100%");
        return 100;
    }
    
    // Linear mapping for now, but you could use a lookup table for better accuracy
    float percentage = ((voltage - BATTERY_MIN_VOLTAGE) / 
                       (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE)) * 100.0f;
    
    uint8_t result = static_cast<uint8_t>(percentage);
    Serial.printf("Calculated battery percentage: %d%%\n", result);
    
    return result;
}

bool EnergyManager::isLowBattery() const {
    return getBatteryVoltage() < (BATTERY_MIN_VOLTAGE + 0.2f); // 20% threshold
}

uint32_t EnergyManager::getCurrentSleepTimeout() const {
    return sleepTimeoutMs;
}