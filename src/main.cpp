#include <TFT_eSPI.h>
#include <SPI.h>

constexpr int BUTTON_RESET_PIN = 0;   // GPIO0
constexpr int BUTTON_STARTSTOP_PIN = 14; // GPIO14

TFT_eSPI tft = TFT_eSPI();

// Constants
constexpr unsigned long DEBOUNCE_TIME = 200; // 200ms debounce time

// Stopwatch state
enum StopwatchState { STOPPED, RUNNING };
StopwatchState stopwatchState = STOPPED;

// Debounce variables
volatile unsigned long lastStartStopPress = 0;
volatile bool needsReset = false;

// Interrupt Service Routines (ISRs)
void IRAM_ATTR onResetButton() {
    stopwatchState = STOPPED;
    needsReset = true;  // Set flag to reset time in main loop
}

void IRAM_ATTR onStartStopButton() {
    unsigned long currentTime = millis();
    if (currentTime - lastStartStopPress >= DEBOUNCE_TIME) {
        if (stopwatchState == STOPPED) {
            stopwatchState = RUNNING;
        } else {
            stopwatchState = STOPPED;
        }
        lastStartStopPress = currentTime;
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
void formatTime(unsigned long ms, char* buffer, size_t len) {
  unsigned int minutes = ms / 60000;
  unsigned int seconds = (ms % 60000) / 1000;
  unsigned int millis = (ms % 1000) / 100;  // Only first digit of milliseconds
  snprintf(buffer, len, "%02u:%02u:%1u", minutes, seconds, millis);
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
    static unsigned long startTime = 0;
    static unsigned long elapsedTime = 0;
    static unsigned long lastUpdateTime = 0;
    static bool displayNeedsUpdate = true;

    // Handle reset request from ISR
    if (needsReset) {
        elapsedTime = 0;
        startTime = 0;
        displayNeedsUpdate = true;
        stopwatchState = STOPPED;  // Reset state to STOPPED
        needsReset = false;
    }

    // If state changed, update the display
    if (displayNeedsUpdate) {
        updateDisplay(elapsedTime);
        displayNeedsUpdate = false;
    }

    // Handle running state
    if (stopwatchState == RUNNING) {
        unsigned long currentTime = millis();
        
        // Update time if stopwatch is running
        if (startTime == 0) {
            startTime = currentTime - elapsedTime;  // Resume from previous elapsed time
        }
        
        elapsedTime = currentTime - startTime;
        
        // Update display every 100ms to avoid flicker
        if (currentTime - lastUpdateTime >= 100) {
            updateDisplay(elapsedTime);
            lastUpdateTime = currentTime;
        }
    } else if (stopwatchState == STOPPED) {
        // Reset start time when stopped, but keep elapsed time
        startTime = 0;
    }

    // Small delay to prevent overwhelming the processor
    delay(10);
}