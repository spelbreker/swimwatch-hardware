#include <TFT_eSPI.h>
#include <SPI.h>
#include "esp_system.h"
#include "esp_timer.h"

constexpr int BUTTON_RESET_PIN = 0;   // GPIO0
constexpr int BUTTON_STARTSTOP_PIN = 14; // GPIO14

TFT_eSPI tft = TFT_eSPI();

// Constants
constexpr unsigned long DEBOUNCE_TIME = 200; // 200ms debounce time
constexpr unsigned long DISPLAY_REFRESH = 1000 / 20; // 20 fps refresh rate

// Stopwatch state
enum StopwatchState { STOPPED, RUNNING };
StopwatchState stopwatchState = STOPPED;

// Time tracking variables
int64_t startMicros = 0;
int64_t elapsedMicros = 0;

// Debounce variables
volatile unsigned long lastStartStopPress = 0;
volatile bool needsReset = false;

// Interrupt Service Routines (ISRs)
void IRAM_ATTR onResetButton() {
    stopwatchState = STOPPED;
    needsReset = true;  // Set flag to reset time in main loop
}

void IRAM_ATTR onStartStopButton() {
    int64_t currentMicros = esp_timer_get_time();
    if ((currentMicros - lastStartStopPress) >= (DEBOUNCE_TIME * 1000)) {
        if (stopwatchState == STOPPED) {
            stopwatchState = RUNNING;
        } else {
            stopwatchState = STOPPED;
        }
        lastStartStopPress = currentMicros;
    }
}

void setup() {
  // Initialize serial for debugging (disable in production)
  Serial.begin(9600);

  // Button pins
  pinMode(BUTTON_RESET_PIN, INPUT_PULLUP);
  pinMode(BUTTON_STARTSTOP_PIN, INPUT_PULLUP);

  // Attach interrupts (FALLING edge, active LOW)
  attachInterrupt(digitalPinToInterrupt(BUTTON_RESET_PIN), onResetButton, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_STARTSTOP_PIN), onStartStopButton, FALLING);

  // TFT display
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setFreeFont(&Orbitron_Light_24);
  tft.setCursor(20, 50);
  tft.print("00:00:000");
  tft.setTextFont(1);
  tft.setCursor(20, 100);
  tft.print("Press Start/Stop");
}

// Format time as mm:ss:ms
void formatTime(int64_t microseconds, char* buffer, size_t len) {
  int64_t totalMs = microseconds / 1000;
  unsigned int minutes = totalMs / 60000;
  unsigned int seconds = (totalMs % 60000) / 1000;
  unsigned int millis = (totalMs % 1000) / 10;  // Two digits of milliseconds (0-99)
  snprintf(buffer, len, "%02u:%02u:%02u", minutes, seconds, millis);
}

void updateDisplay(unsigned long ms) {
  char timeStr[16];
  formatTime(ms, timeStr, sizeof(timeStr));
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setFreeFont(&Orbitron_Light_24);
  tft.setCursor(20, 50);
  tft.print(timeStr);
  tft.setTextFont(1);
  tft.setCursor(20, 100);
  if (stopwatchState == RUNNING) {
    tft.print("Running");
  } else {
    tft.print("Stopped");
  }
}

void loop() {
    static int64_t lastUpdateMicros = 0;
    static bool displayNeedsUpdate = true;

    // Handle reset request from ISR
    if (needsReset) {
        elapsedMicros = 0;
        startMicros = 0;
        displayNeedsUpdate = true;
        stopwatchState = STOPPED;
        needsReset = false;
    }

    // Get current time
    int64_t currentMicros = esp_timer_get_time();

    // Handle running state
    if (stopwatchState == RUNNING) {
        if (startMicros == 0) {
            startMicros = currentMicros - elapsedMicros;  // Resume from previous elapsed time
        }
        elapsedMicros = currentMicros - startMicros;
        
        // Update display every DISPLAY_REFRESH ms
        if ((currentMicros - lastUpdateMicros) >= (DISPLAY_REFRESH * 1000)) {
            updateDisplay(elapsedMicros);
            lastUpdateMicros = currentMicros;
        }
    } else if (stopwatchState == STOPPED) {
        // When stopped, update display once to show final time
        if (displayNeedsUpdate) {
            updateDisplay(elapsedMicros);
            displayNeedsUpdate = false;
        }
        startMicros = 0;  // Reset start time but keep elapsed time
    }
    
    // Very small delay to prevent overwhelming the processor
    delay(1);
}