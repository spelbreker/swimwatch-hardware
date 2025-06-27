#include "button_manager.h"

// Static instance pointer for interrupt handlers
ButtonManager* ButtonManager::instance = nullptr;

ButtonManager::ButtonManager() 
    : startLapInterrupt(false)
    , stopInterrupt(false)
    , start2Interrupt(false)
    , lastStartLapInterrupt(0)
    , lastStopInterrupt(0)
    , lastStart2Interrupt(0) {
    
    // Set the static instance pointer
    instance = this;
}

bool ButtonManager::init() {
    Serial.println("Initializing button manager...");
    
    // Configure GPIO pins
    pinMode(BUTTON_START_LAP_PIN, INPUT_PULLUP);  // GPIO0 - internal pullup
    pinMode(BUTTON_STOP_PIN, INPUT_PULLUP);       // GPIO14 - internal pullup
    pinMode(BUTTON_START_2_PIN, INPUT_PULLDOWN);  // GPIO2 - internal pulldown (button connects to 3.3V)
    
    Serial.println("Button pins configured");
    
    // Attach interrupts
    attachInterrupt(digitalPinToInterrupt(BUTTON_START_LAP_PIN), handleStartLapInterrupt, FALLING);
    attachInterrupt(digitalPinToInterrupt(BUTTON_STOP_PIN), handleStopInterrupt, FALLING);
    attachInterrupt(digitalPinToInterrupt(BUTTON_START_2_PIN), handleStart2Interrupt, RISING);
    
    Serial.println("Button interrupts attached");
    Serial.printf("Button pins - Start/Lap: %d, Stop: %d, Start2: %d\n", 
                  BUTTON_START_LAP_PIN, BUTTON_STOP_PIN, BUTTON_START_2_PIN);
    
    return true;
}

ButtonEvent ButtonManager::getButtonEvent() {
    // Check interrupts in priority order
    if (startLapInterrupt) {
        startLapInterrupt = false;
        Serial.println("Start/Lap button event");
        return BUTTON_START_LAP_PRESSED;
    }
    
    if (start2Interrupt) {
        start2Interrupt = false;
        Serial.println("Start button 2 event");
        return BUTTON_START_2_PRESSED;
    }
    
    if (stopInterrupt) {
        stopInterrupt = false;
        Serial.println("Stop button event");
        return BUTTON_STOP_PRESSED;
    }
    
    return BUTTON_NONE;
}

void ButtonManager::clearEvents() {
    startLapInterrupt = false;
    stopInterrupt = false;
    start2Interrupt = false;
}

bool ButtonManager::isStartLapPressed() {
    return digitalRead(BUTTON_START_LAP_PIN) == LOW;
}

bool ButtonManager::isStopPressed() {
    return digitalRead(BUTTON_STOP_PIN) == LOW;
}

bool ButtonManager::isStart2Pressed() {
    return digitalRead(BUTTON_START_2_PIN) == HIGH;
}

// Static interrupt handlers
void IRAM_ATTR ButtonManager::handleStartLapInterrupt() {
    if (instance) {
        instance->handleStartLapISR();
    }
}

void IRAM_ATTR ButtonManager::handleStopInterrupt() {
    if (instance) {
        instance->handleStopISR();
    }
}

void IRAM_ATTR ButtonManager::handleStart2Interrupt() {
    if (instance) {
        instance->handleStart2ISR();
    }
}

// Instance interrupt handlers
void ButtonManager::handleStartLapISR() {
    uint32_t now = millis();
    if (now - lastStartLapInterrupt > DEBOUNCE_TIME_MS) {
        startLapInterrupt = true;
        lastStartLapInterrupt = now;
    }
}

void ButtonManager::handleStopISR() {
    uint32_t now = millis();
    if (now - lastStopInterrupt > DEBOUNCE_TIME_MS) {
        stopInterrupt = true;
        lastStopInterrupt = now;
    }
}

void ButtonManager::handleStart2ISR() {
    uint32_t now = millis();
    // Additional check for HIGH state to filter noise
    if (digitalRead(BUTTON_START_2_PIN) == HIGH && 
        now - lastStart2Interrupt > DEBOUNCE_TIME_MS) {
        start2Interrupt = true;
        lastStart2Interrupt = now;
    }
}
