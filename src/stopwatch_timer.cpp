/**
 * @file stopwatch_timer.cpp
 * @brief High-precision elapsed timer implementation
 *
 * Uses esp_timer_get_time() — a 64-bit microsecond counter driven by
 * the ESP32-S3's crystal oscillator. This timer is:
 * - Independent of FreeRTOS tick rate
 * - Not affected by NTP/RTC adjustments (SNTP_SYNC_MODE_SMOOTH)
 * - 1µs resolution, no overflow for 292,000 years
 *
 * Wall-clock timestamps (time()) are attached to splits for logging
 * but are NOT used for elapsed calculations.
 */
#include "stopwatch_timer.h"
#include "config.h"
#include <esp_timer.h>
#include <cstdio>

void StopwatchTimer::start() {
    if (_running) return;
    _startTimeUs = esp_timer_get_time();
    _running = true;
    _splits.clear();
    DEBUG_LOG("Timer started at %lld µs", _startTimeUs);
}

void StopwatchTimer::stop() {
    if (!_running) return;
    _stopTimeUs = esp_timer_get_time();
    _running = false;
    DEBUG_LOG("Timer stopped: %u ms", getElapsedMs());
}

void StopwatchTimer::reset() {
    _running = false;
    _startTimeUs = 0;
    _stopTimeUs = 0;
    _splits.clear();
    DEBUG_LOG("Timer reset");
}

void StopwatchTimer::addSplit(uint8_t lane) {
    if (!_running) return;
    SplitTime split;
    split.elapsedMs = getElapsedMs();
    split.wallClock = time(nullptr);   // NTP-synced RTC for absolute timestamp
    split.lane = lane;
    _splits.push_back(split);
    DEBUG_LOG("Split: %u ms (lane %u)", split.elapsedMs, split.lane);
}

bool StopwatchTimer::isRunning() const {
    return _running;
}

uint32_t StopwatchTimer::getElapsedMs() const {
    if (_startTimeUs == 0) return 0;
    int64_t now = _running ? esp_timer_get_time() : _stopTimeUs;
    return static_cast<uint32_t>((now - _startTimeUs) / 1000);
}

const std::vector<SplitTime>& StopwatchTimer::getSplits() const {
    return _splits;
}

void StopwatchTimer::getFormattedTime(char* buffer, size_t bufferSize, bool fullPrecision) const {
    formatMs(getElapsedMs(), fullPrecision, buffer, bufferSize);
}

void StopwatchTimer::formatMs(uint32_t ms, bool fullPrecision, char* buffer, size_t bufferSize) {
    uint32_t totalSeconds = ms / 1000;
    uint32_t minutes = totalSeconds / 60;
    uint32_t seconds = totalSeconds % 60;
    uint32_t remainder = ms % 1000;

    if (fullPrecision) {
        // Stopped: show 2-digit centiseconds "MM:SS.cc"
        snprintf(buffer, bufferSize, "%02u:%02u.%02u", minutes, seconds, remainder / 10);
    } else {
        // Running: show 1-digit deciseconds "MM:SS.d"
        snprintf(buffer, bufferSize, "%02u:%02u.%01u", minutes, seconds, remainder / 100);
    }
}
