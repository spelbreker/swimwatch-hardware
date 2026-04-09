/**
 * @file button_manager.h
 * @brief Hardware button handler with ISR + debounce for SwimWatch
 *
 * Buttons:
 *   GPIO0  (BUTTON1): Start/Stop toggle — onboard, active LOW
 *   GPIO14 (BUTTON2): Reset (only when stopped) — onboard, active LOW
 *   GPIO2  (external): Split trigger — active LOW, external pullup
 *
 * Each button uses a hardware interrupt with software debounce
 * (BUTTON_DEBOUNCE_MS from config.h, default 200ms).
 */
#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>
#include <esp_timer.h>
#include "config.h"

/** Button events returned by getButtonEvent() */
enum ButtonEvent {
    BUTTON_NONE,
    BUTTON_START_STOP,   ///< GPIO0 pressed  — toggle start/stop
    BUTTON_RESET,        ///< GPIO14 pressed — reset (only when stopped)
    BUTTON_LAP_PRESSED   ///< GPIO2 pressed  — split / starter send
};

/** Button event with ISR-captured hardware timestamp for split accuracy */
struct ButtonEventData {
    ButtonEvent event;         ///< Which button was pressed
    int64_t     timestampUs;   ///< esp_timer_get_time() captured at press (0 = not captured)
};

/**
 * @class ButtonManager
 * @brief ISR-driven button handler with software debounce
 */
class ButtonManager {
public:
    ButtonManager();

    /** Configure GPIOs and attach interrupts. Call once in setup(). */
    bool init();

    /** Return the next pending event with ISR-captured timestamp (one per call, priority order). */
    ButtonEventData getButtonEvent();

    /** Discard all pending events. */
    void clearEvents();

private:
    // ISR flags (set in ISR, cleared in getButtonEvent)
    volatile bool _startStopFlag;
    volatile bool _resetFlag;
    volatile bool _splitFlag;

    // ISR-captured hardware timestamps (esp_timer_get_time, µs resolution)
    volatile int64_t _splitTimestampUs;
    volatile int64_t _startStopTimestampUs;

    // Debounce timestamps
    volatile uint32_t _lastStartStop;
    volatile uint32_t _lastReset;
    volatile uint32_t _lastSplit;

    // Polling state for GPIO2 fallback (strapping pin ISR workaround)
    // GPIO2 uses INPUT_PULLDOWN + RISING: track last-seen LOW state
    bool _splitPinWasLow;

    // Static instance + ISR wrappers (attachInterrupt requires static)
    static ButtonManager* _instance;
    static void IRAM_ATTR _isrStartStop();
    static void IRAM_ATTR _isrReset();
    static void IRAM_ATTR _isrSplit();
};

#endif // BUTTON_MANAGER_H
