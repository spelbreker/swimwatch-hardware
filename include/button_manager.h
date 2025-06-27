#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>

// Hardware Pin Definitions for LilyGO T-Display S3
#define BUTTON_START_LAP_PIN 0   // GPIO0 - Start/Lap button (active LOW, internal pullup)
#define BUTTON_STOP_PIN      14  // GPIO14 - Stop button (active LOW, internal pullup) 
#define BUTTON_START_2_PIN   2   // GPIO2 - Additional start button (active HIGH, external pulldown required)

// Button timing
#define DEBOUNCE_TIME_MS 100

// Button states
enum ButtonEvent {
    BUTTON_NONE,
    BUTTON_START_LAP_PRESSED,
    BUTTON_STOP_PRESSED,
    BUTTON_START_2_PRESSED
};

class ButtonManager {
private:
    // Interrupt flags
    volatile bool startLapInterrupt;
    volatile bool stopInterrupt;
    volatile bool start2Interrupt;
    
    // Debounce timing
    volatile uint32_t lastStartLapInterrupt;
    volatile uint32_t lastStopInterrupt;
    volatile uint32_t lastStart2Interrupt;
    
    // Static interrupt handlers (required for attachInterrupt)
    static ButtonManager* instance;
    static void IRAM_ATTR handleStartLapInterrupt();
    static void IRAM_ATTR handleStopInterrupt();
    static void IRAM_ATTR handleStart2Interrupt();
    
public:
    ButtonManager();
    
    // Initialization
    bool init();
    
    // Event processing
    ButtonEvent getButtonEvent();
    void clearEvents();
    
    // Button state reading (for polling if needed)
    bool isStartLapPressed();
    bool isStopPressed();
    bool isStart2Pressed();
    
    // Interrupt handlers (called by static handlers)
    void handleStartLapISR();
    void handleStopISR();
    void handleStart2ISR();
};

#endif // BUTTON_MANAGER_H
