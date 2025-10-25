#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>

// Hardware Pin Definitions for LilyGO T-Display S3
#define BUTTON_LAP_PIN 2   // GPIO2 - Lap button (active HIGH, external pulldown required)

// Button timing
#define DEBOUNCE_TIME_MS 300  // Extended debounce for GPIO2 split button

// Button states
enum ButtonEvent {
    BUTTON_NONE,
    BUTTON_LAP_PRESSED
};

class ButtonManager {
private:
    // Interrupt flags
    volatile bool lapInterrupt;
    
    // Debounce timing
    volatile uint32_t lastLapInterrupt;
    
    // Static interrupt handlers (required for attachInterrupt)
    static ButtonManager* instance;
    static void IRAM_ATTR handleLapInterrupt();
    
public:
    ButtonManager();
    
    // Initialization
    bool init();
    
    // Event processing
    ButtonEvent getButtonEvent();
    void clearEvents();
    
    // Button state reading (for polling if needed)
    bool isLapPressed();
    
    // Interrupt handlers (called by static handlers)
    void handleLapISR();
};

#endif // BUTTON_MANAGER_H
