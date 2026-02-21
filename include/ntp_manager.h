/**
 * @file ntp_manager.h
 * @brief NTP time synchronization for ESP32-S3
 *
 * Syncs the ESP32's internal RTC via SNTP to the local server.
 * Uses SNTP_SYNC_MODE_SMOOTH to avoid time jumps during races.
 *
 * The NTP sync does NOT affect esp_timer_get_time() — elapsed
 * stopwatch timing is completely independent of NTP adjustments.
 *
 * Sync interval: 60s (configurable via NTP_SYNC_INTERVAL_MS in config.h)
 * LAN accuracy:  ±1ms per sync, ±1.2ms max drift between syncs
 */
#pragma once

#include <cstdint>
#include <time.h>

class NTPManager {
public:
    /**
     * Initialize SNTP client and start periodic background sync.
     * Non-blocking — sync happens in a FreeRTOS background task.
     * @param ntpServer  IP or hostname of NTP server (e.g. "192.168.1.10")
     */
    void begin(const char* ntpServer);

    /** @return true once the first NTP sync has completed */
    bool isSynced() const;

    /**
     * Block until NTP syncs or timeout expires.
     * Only use during startup — never in loop().
     * @param timeoutMs  Maximum wait in milliseconds
     * @return true if synced, false if timed out
     */
    bool waitForSync(uint32_t timeoutMs);

    /** @return current UTC time as epoch seconds */
    time_t getEpochTime() const;

    /**
     * Write formatted UTC time string "HH:MM:SS" into buffer.
     * @param buffer      Output buffer (min 9 bytes)
     * @param bufferSize  Size of output buffer
     */
    void getFormattedTime(char* buffer, size_t bufferSize) const;

private:
    bool _synced = false;

    /** Singleton-like pointer for the sync callback lambda */
    static NTPManager* _instance;
};
