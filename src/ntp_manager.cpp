/**
 * @file ntp_manager.cpp
 * @brief NTP synchronization implementation
 *
 * Uses ESP-IDF SNTP library (built into Arduino-ESP32).
 * Smooth sync mode adjusts clock gradually — no time jumps.
 *
 * IMPORTANT: This only adjusts the RTC (time()/getLocalTime()).
 * esp_timer_get_time() is a separate hardware counter and is
 * NOT affected by NTP corrections. Stopwatch elapsed timing
 * remains precise regardless of NTP behavior.
 */
#include "ntp_manager.h"
#include "config.h"
#include <Arduino.h>
#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h"

NTPManager* NTPManager::_instance = nullptr;

void NTPManager::begin(const char* ntpServer) {
    _instance = this;
    _synced = false;

    // Configure NTP using the Arduino-ESP32 configTime helper
    // This internally sets SNTP to poll mode
    configTime(0, 0, ntpServer);

    // Register sync notification so we know when first sync completes
    sntp_set_time_sync_notification_cb(
        [](struct timeval* tv) {
            if (_instance) {
                _instance->_synced = true;
                DEBUG_LOG("NTP sync callback fired");
            }
        }
    );

    DEBUG_LOG("NTP started — server: %s", ntpServer);
}

bool NTPManager::isSynced() const {
    return _synced;
}

bool NTPManager::waitForSync(uint32_t timeoutMs) {
    uint32_t start = millis();
    while (!_synced && (millis() - start) < timeoutMs) {
        delay(100);
    }
    if (_synced) {
        char buf[9];
        getFormattedTime(buf, sizeof(buf));
        DEBUG_LOG("NTP synced: %s UTC", buf);
    } else {
        DEBUG_LOG("NTP sync timeout after %ums", timeoutMs);
    }
    return _synced;
}

time_t NTPManager::getEpochTime() const {
    return time(nullptr);
}

void NTPManager::getFormattedTime(char* buffer, size_t bufferSize) const {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 0)) {
        strftime(buffer, bufferSize, "%H:%M:%S", &timeinfo);
    } else {
        snprintf(buffer, bufferSize, "--:--:--");
    }
}
