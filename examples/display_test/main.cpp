/**
 * Simple display layout test for LilyGO T-Display S3
 * Tests the new layout with stopwatch on left and status on right
 */

#include <Arduino.h>
#include "../../include/display_manager.h"

DisplayManager display;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting Display Layout Test");
    
    // Initialize display
    if (!display.init()) {
        Serial.println("Display initialization failed!");
        while (true) delay(1000);
    }
    
    // Draw the layout
    display.clearScreen();
    display.drawBorders();
    
    // Test stopwatch area
    display.updateStopwatchDisplay(123456, true);  // 02:03:4 (running)
    
    // Test lap areas
    display.updateLapTime(1, "00:45:23");
    display.updateLapTime(2, "01:32:11");  
    display.updateLapTime(3, "02:03:44");
    
    // Test status areas
    display.updateWiFiStatus("Connected", true);
    display.updateWebSocketStatus("Connected", true);
    display.updateLaneInfo(9);
    display.updateBatteryDisplay(3.8f, 75);
    
    Serial.println("Display test complete");
}

void loop() {
    // Update time every second to test running display
    static unsigned long lastUpdate = 0;
    static uint32_t testTime = 123456;
    
    if (millis() - lastUpdate > 1000) {
        testTime += 1000; // Add 1 second
        display.updateStopwatchDisplay(testTime, true);
        lastUpdate = millis();
    }
    
    delay(10);
}
