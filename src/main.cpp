/**
 * LilyGO T-Display S3 Stopwatch - Hardware Interrupts
 * BUTTON1 (Start/Lap): GPIO35 (active LOW)
 * BUTTON2 (Stop):      GPIO14 (active LOW)
 * Display: ST7789V via TFT_eSPI
 * 
 * Features:
 * - Hardware interrupt buttons
 * - Software debounce
 * - Time measurement using millis()
 * - 25Hz display refresh rate
 */

#include <TFT_eSPI.h>
#include <SPI.h>

// Constants and Pin Definitions
#define BUTTON_START_LAP_PIN 0
#define BUTTON_STOP_PIN      14
#define DEBOUNCE_TIME_MS     50    // Reduced debounce time for better responsiveness
#define DISPLAY_REFRESH_MS   50    // 20Hz refresh rate

constexpr uint8_t MAX_LAPS = 5;

// Type definitions
enum StopwatchState { STOPPED, RUNNING };
struct LapData { uint32_t lapTimeMs, totalTimeMs; };

// Forward declarations
void resetStopwatch();
void drawStopwatch(uint32_t currentElapsedMs);
void processStartLap();
void processStop();
void addLap(uint32_t currentElapsedMs);
void drawLaps();
String formatTime(uint32_t ms);

// Global variables
TFT_eSPI tft = TFT_eSPI();

volatile bool startLapInterrupt = false;
volatile bool stopInterrupt = false;

volatile uint32_t lastStartLapInterrupt = 0;
volatile uint32_t lastStopInterrupt = 0;

StopwatchState stopwatchState = STOPPED;
uint32_t startTimeMs = 0;
uint32_t elapsedMs = 0;
uint32_t lastDisplayUpdateMs = 0;

LapData laps[MAX_LAPS];
uint8_t lapCount = 0;

// ---------- Interrupt Service Routines ----------
void IRAM_ATTR handleStartLapInterrupt() {
  uint32_t now = millis();
  if (now - lastStartLapInterrupt > DEBOUNCE_TIME_MS) {
    startLapInterrupt = true;
    lastStartLapInterrupt = now;
    Serial.println("Start/Lap button pressed"); // Debug output
  }
}

void IRAM_ATTR handleStopInterrupt() {
  uint32_t now = millis();
  if (now - lastStopInterrupt > DEBOUNCE_TIME_MS) {
    stopInterrupt = true;
    lastStopInterrupt = now;
    Serial.println("Stop button pressed"); // Debug output
  }
}

// --------------- Setup -----------------
void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  
  // Button setup with explicit GPIO configuration
  pinMode(BUTTON_START_LAP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_STOP_PIN, INPUT_PULLUP);

  // Attach interrupts with explicit GPIO numbers
  attachInterrupt(BUTTON_START_LAP_PIN, handleStartLapInterrupt, FALLING);
  attachInterrupt(BUTTON_STOP_PIN, handleStopInterrupt, FALLING);

  // Initialize display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(4);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);

  resetStopwatch();
  drawStopwatch(0);

  // Serial.begin(115200); // debugging indien gewenst
}

// --------------- Loop -----------------
void loop() {
  uint32_t now = millis();

  // ---- Interrupts verwerken ----
  if (startLapInterrupt) {
    startLapInterrupt = false;
    processStartLap();
  }
  if (stopInterrupt) {
    stopInterrupt = false;
    processStop();
  }

  // ---- Display Update ----
  uint32_t showElapsedMs = (stopwatchState == RUNNING)
      ? now - startTimeMs
      : elapsedMs;
  if (now - lastDisplayUpdateMs > DISPLAY_REFRESH_MS || lastDisplayUpdateMs == 0) {
    drawStopwatch(showElapsedMs);
    lastDisplayUpdateMs = now;
  }
}

// ------- Functionaliteit ---------
void processStartLap() {
  if (stopwatchState == STOPPED) {
    startTimeMs = millis();
    lapCount = 0;
    stopwatchState = RUNNING;
    // Clear lap display area
    tft.fillRect(0, 65, 320, 170-65, TFT_BLACK);
  } else if (stopwatchState == RUNNING && lapCount < MAX_LAPS) {
    uint32_t nowMs = millis();
    addLap(nowMs - startTimeMs);
  }
}

void processStop() {
  if (stopwatchState == RUNNING) {
    elapsedMs = millis() - startTimeMs;
    stopwatchState = STOPPED;
    drawLaps();
  }
}

void resetStopwatch() {
  stopwatchState = STOPPED;
  startTimeMs = 0;
  elapsedMs = 0;
  lapCount = 0;
  for (uint8_t i = 0; i < MAX_LAPS; ++i) {
    laps[i].lapTimeMs = 0;
    laps[i].totalTimeMs = 0;
  }
}

void addLap(uint32_t currentElapsedMs) {
  uint32_t lapTime = currentElapsedMs;
  if (lapCount > 0) {
    lapTime = currentElapsedMs - laps[lapCount - 1].totalTimeMs;
  }
  laps[lapCount].lapTimeMs = lapTime;
  laps[lapCount].totalTimeMs = currentElapsedMs;
  lapCount++;
  drawLaps();
}

// -------------- Display Logic ---------------
void drawStopwatch(uint32_t currentElapsedMs) {
  tft.fillRect(0, 0, 320, 60, TFT_BLACK);
  tft.setCursor(10, 10);
  tft.setTextFont(7);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.print(formatTime(currentElapsedMs));
}

void drawLaps() {
  tft.fillRect(0, 65, 320, 170-65, TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  for (uint8_t i = 0; i < lapCount; ++i) {
    tft.setCursor(10, 70 + i*20);
    tft.printf("Lap %d: %s (%s)", i+1, formatTime(laps[i].lapTimeMs).c_str(), formatTime(laps[i].totalTimeMs).c_str());
  }
  if (stopwatchState == STOPPED && lapCount > 0) {
    tft.setCursor(10, 70 + lapCount*20);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.print("Press Start to reset.");
  }
}

// ------------- Tijd notatie -----------------
String formatTime(uint32_t ms) {
  uint16_t minutes = ms / 60000;
  uint8_t seconds = (ms / 1000) % 60;
  uint8_t millisec = (ms % 1000) / 10;
  char buffer[16];
  sprintf(buffer, "%02d:%02d:%02d", minutes, seconds, millisec);
  return String(buffer);
}