/**
 * @file stopwatch_timer.h
 * @brief High-precision stopwatch timer using ESP32 hardware timer
 *
 * Uses esp_timer_get_time() for 1µs resolution elapsed timing.
 * Independent of NTP/RTC — not affected by time sync adjustments.
 *
 * Precision: 1µs resolution, ~±20ppm drift (~±36ms over 30 minutes)
 * Overflow:  64-bit counter, safe for 292,000 years
 *
 * Hardware: ESP32-S3 hardware timer (240MHz crystal derived)
 */
#pragma once

#include <cstdint>
#include <vector>
#include <time.h>

/** Recorded split/lap time */
struct SplitTime {
    uint32_t elapsedMs;    ///< Elapsed ms since start (from hardware timer)
    time_t   wallClock;    ///< Absolute UTC timestamp (from NTP-synced RTC)
    uint8_t  lane;         ///< Lane number (0 = local button)
};

/**
 * @class StopwatchTimer
 * @brief Precision elapsed timer using ESP32 hardware counter
 *
 * Provides start/stop/reset/split functionality with 1µs internal
 * resolution. Display output is formatted to deciseconds (running)
 * or centiseconds (stopped).
 *
 * Usage:
 *   StopwatchTimer timer;
 *   timer.start();
 *   // ... later ...
 *   timer.addSplit(laneNumber);
 *   timer.stop();
 *   char buf[16];
 *   timer.getFormattedTime(buf, sizeof(buf), true);  // "01:23.45"
 */
class StopwatchTimer {
public:
    /** Start the timer. Clears splits. No-op if already running. */
    void start();

    /**
     * Start the timer, backdating by an offset to compensate for network delay.
     * The elapsed time will read as if the timer started `offsetUs` microseconds ago.
     * @param offsetUs  Microseconds to subtract from the current time as the start point
     */
    void startWithOffset(int64_t offsetUs);

    /** Stop the timer. Freezes elapsed time. No-op if not running. */
    void stop();

    /** Reset timer to zero. Stops if running. Clears all splits. */
    void reset();

    /**
     * Record a split time at the current elapsed position.
     * @param lane  Lane number (0 for local button press)
     */
    void addSplit(uint8_t lane = 0);

    /**
     * Record a split time using a pre-captured hardware timestamp.
     * Use this when the timestamp was captured in an ISR for maximum accuracy.
     * @param lane            Lane number (0 for local button press)
     * @param capturedTimeUs  esp_timer_get_time() value captured at button press
     */
    void addSplit(uint8_t lane, int64_t capturedTimeUs);

    /** @return true if the timer is currently running */
    bool isRunning() const;

    /** @return elapsed time in milliseconds (1ms precision) */
    uint32_t getElapsedMs() const;

    /** @return reference to all recorded split times */
    const std::vector<SplitTime>& getSplits() const;

    /**
     * Write formatted time string into buffer.
     *
     * @param buffer        Output buffer (min 12 bytes recommended)
     * @param bufferSize    Size of output buffer
     * @param fullPrecision false → "MM:SS.d" (1-digit, for running display)
     *                      true  → "MM:SS.cc" (2-digit, for stopped display)
     */
    void getFormattedTime(char* buffer, size_t bufferSize, bool fullPrecision = false) const;

    /**
     * Format a given elapsed millisecond value (static utility).
     * @param ms            Elapsed time in milliseconds
     * @param fullPrecision Whether to show centiseconds
     * @param buffer        Output buffer
     * @param bufferSize    Buffer size
     */
    static void formatMs(uint32_t ms, bool fullPrecision, char* buffer, size_t bufferSize);

private:
    int64_t  _startTimeUs = 0;     ///< esp_timer_get_time() at start
    int64_t  _stopTimeUs  = 0;     ///< esp_timer_get_time() at stop
    bool     _running     = false;
    std::vector<SplitTime> _splits;
};
