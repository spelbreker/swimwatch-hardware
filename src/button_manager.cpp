/**
 * @file button_manager.cpp
 * @brief ISR-driven button handling for SwimWatch
 *
 * All three buttons use FALLING edge interrupts (active LOW).
 * Software debounce uses BUTTON_DEBOUNCE_MS (config.h).
 *
 * GPIO0  (BUTTON1): onboard, has internal pull-up — FALLING on press
 * GPIO14 (BUTTON2): onboard, has internal pull-up — FALLING on press
 * GPIO2  (external): needs INPUT_PULLUP — FALLING on press
 */
#include "button_manager.h"

ButtonManager* ButtonManager::_instance = nullptr;

ButtonManager::ButtonManager()
    : _startStopFlag(false)
    , _resetFlag(false)
    , _splitFlag(false)
    , _lastStartStop(0)
    , _lastReset(0)
    , _lastSplit(0)
    , _splitPinWasLow(true) {
    _instance = this;
}

bool ButtonManager::init() {
    DEBUG_LOG("Initializing buttons: GPIO%d(start/stop), GPIO%d(reset), GPIO%d(split)",
              PIN_BUTTON_START_STOP, PIN_BUTTON_RESET, PIN_BUTTON_SPLIT);

    // GPIO0  — onboard BUTTON1 (has external pull-up on board)
    pinMode(PIN_BUTTON_START_STOP, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_BUTTON_START_STOP), _isrStartStop, FALLING);

    // GPIO14 — onboard BUTTON2 (has external pull-up on board)
    pinMode(PIN_BUTTON_RESET, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_BUTTON_RESET), _isrReset, FALLING);

    // GPIO2  — external split trigger (active HIGH, internal pull-down)
    // Button connects GPIO2 to 3.3V — triggered on RISING edge
    pinMode(PIN_BUTTON_SPLIT, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(PIN_BUTTON_SPLIT), _isrSplit, RISING);

    DEBUG_LOG("Buttons initialized, debounce: %dms", BUTTON_DEBOUNCE_MS);
    return true;
}

ButtonEvent ButtonManager::getButtonEvent() {
    // Priority: start/stop > reset > split
    if (_startStopFlag) {
        _startStopFlag = false;
        return BUTTON_START_STOP;
    }
    if (_resetFlag) {
        _resetFlag = false;
        return BUTTON_RESET;
    }
    // GPIO2: check ISR flag OR poll directly (ISR unreliable on strapping pins)
    // Button connects to 3.3V — detect rising edge (LOW→HIGH)
    bool splitByPolling = false;
    bool pinHigh = (digitalRead(PIN_BUTTON_SPLIT) == HIGH);
    if (pinHigh && _splitPinWasLow) {
        // Rising edge detected via polling
        uint32_t now = millis();
        if (now - _lastSplit > BUTTON_DEBOUNCE_MS) {
            _lastSplit = now;
            splitByPolling = true;
        }
    }
    _splitPinWasLow = !pinHigh;
    if (_splitFlag || splitByPolling) {
        _splitFlag = false;
        return BUTTON_LAP_PRESSED;
    }
    return BUTTON_NONE;
}

void ButtonManager::clearEvents() {
    _startStopFlag = false;
    _resetFlag = false;
    _splitFlag = false;
}

// ── Static ISR wrappers ────────────────────────────────────────

void IRAM_ATTR ButtonManager::_isrStartStop() {
    if (!_instance) return;
    uint32_t now = millis();
    if (now - _instance->_lastStartStop > BUTTON_DEBOUNCE_MS) {
        _instance->_startStopFlag = true;
        _instance->_lastStartStop = now;
    }
}

void IRAM_ATTR ButtonManager::_isrReset() {
    if (!_instance) return;
    uint32_t now = millis();
    if (now - _instance->_lastReset > BUTTON_DEBOUNCE_MS) {
        _instance->_resetFlag = true;
        _instance->_lastReset = now;
    }
}

void IRAM_ATTR ButtonManager::_isrSplit() {
    if (!_instance) return;
    // Verify pin is actually HIGH (filter noise on RISING edge)
    if (digitalRead(PIN_BUTTON_SPLIT) != HIGH) return;
    uint32_t now = millis();
    if (now - _instance->_lastSplit > BUTTON_DEBOUNCE_MS) {
        _instance->_splitFlag = true;
        _instance->_lastSplit = now;
    }
}
