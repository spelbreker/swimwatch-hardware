#include "button_manager.h"

// Static instance pointer for interrupt handlers
ButtonManager* ButtonManager::instance = nullptr;

ButtonManager::ButtonManager() 
    : lapInterrupt(false)
    , lastLapInterrupt(0) {
    
    // Set the static instance pointer
    instance = this;
}

bool ButtonManager::init() {
    Serial.println("Initializing button manager...");
    
    // Configure GPIO pins
    pinMode(BUTTON_LAP_PIN, INPUT_PULLDOWN);  // GPIO2 - internal pulldown (button connects to 3.3V)
    
    Serial.println("Button pins configured");
    
    // Attach interrupts
    attachInterrupt(digitalPinToInterrupt(BUTTON_LAP_PIN), handleLapInterrupt, RISING);
    
    Serial.println("Button interrupts attached");
    Serial.printf("Button pin - Lap: %d\n", BUTTON_LAP_PIN);
    Serial.printf("Debounce time: %dms\n", DEBOUNCE_TIME_MS);
    
    return true;
}

ButtonEvent ButtonManager::getButtonEvent() {
    // Check lap interrupt
    if (lapInterrupt) {
        lapInterrupt = false;
        Serial.println("Lap button event");
        return BUTTON_LAP_PRESSED;
    }
    
    return BUTTON_NONE;
}

void ButtonManager::clearEvents() {
    lapInterrupt = false;
}

bool ButtonManager::isLapPressed() {
    return digitalRead(BUTTON_LAP_PIN) == HIGH;
}

// Static interrupt handlers
void IRAM_ATTR ButtonManager::handleLapInterrupt() {
    if (instance) {
        instance->handleLapISR();
    }
}

// Instance interrupt handlers
void ButtonManager::handleLapISR() {
    uint32_t now = millis();
    // Additional check for HIGH state to filter noise
    // Use extended debounce time for GPIO2 split button
    if (digitalRead(BUTTON_LAP_PIN) == HIGH && 
        now - lastLapInterrupt > DEBOUNCE_TIME_MS) {
        lapInterrupt = true;
        lastLapInterrupt = now;
    }
}
