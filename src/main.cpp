/**
 * LilyGO T-Display S3 Stopwatch - Hardware Interrupts & WebSocket Client
 * * Hardware:
 * - BUTTON1 (Start/Lap): GPIO0 (active LOW, internal pullup)
 * - BUTTON2 (Stop):      GPIO14 (active LOW, internal pullup)
 * - BUTTON3 (Start):     GPIO2 (active HIGH, externe pull-down weerstand vereist)
 * -> BELANGRIJK: Voor BUTTON3 (GPIO2) sluit de knop aan tussen 3.3V en GPIO2.
 * Plaats een externe 1kΩ weerstand tussen GPIO2 en GND.
 * - Display: ST7789V via TFT_eSPI
 * * Features:
 * - Hardware interrupt buttons
 * - Software debounce
 * - Time measurement using millis()
 * - 25Hz display refresh rate
 * - WiFi connectivity with stored credentials
 * - WebSocket client with SSL support
 * * WebSocket Events:
 * Receive:
 * - {"type":"start","time":timestamp} : Start stopwatch with server time
 * - {"type":"reset"} : Reset stopwatch to zero
 * * Send:
 * - {"type":"split","lane":"X","time-ms":timestamp,"time":"MM:SS:MS"} : Split time
 * * Server:
 * - wss://scherm.azckamp.nl:443
 */

#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFi.h>
#include <Preferences.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

// Constants and Pin Definitions
#define BUTTON_START_LAP_PIN 0
#define BUTTON_STOP_PIN      14
#define BUTTON_START_2_PIN   2      // Additional start button on GPIO2
#define DEBOUNCE_TIME_MS     100     // Reduced debounce time for better responsiveness
#define DISPLAY_REFRESH_MS   50     // 20Hz refresh rate
#define WIFI_CONNECT_TIMEOUT_MS 15000 // 15s connect timeout

constexpr uint8_t MAX_LAPS = 90;      // Store up to 50 laps
constexpr uint8_t DISPLAY_LAPS = 5;   // Show only last 5 laps

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
void connectWiFi();
void saveWiFiCredentials(const char* ssid, const char* password);
bool getWiFiCredentials(String &ssid, String &password);
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void setupWebSocket();

// Global variables
Preferences preferences;
TFT_eSPI tft = TFT_eSPI();

volatile bool startLapInterrupt = false;
volatile bool stopInterrupt = false;
volatile bool start2Interrupt = false;

volatile uint32_t lastStartLapInterrupt = 0;
volatile uint32_t lastStopInterrupt = 0;
volatile uint32_t lastStart2Interrupt = 0;

StopwatchState stopwatchState = STOPPED;
uint32_t startTimeMs = 0;
uint32_t elapsedMs = 0;
uint32_t lastDisplayUpdateMs = 0;

LapData laps[MAX_LAPS];
uint8_t lapCount = 0;

// WebSocket Client
WebSocketsClient webSocket;
bool wsConnected = false;

// Additional global variables
uint64_t serverStartTimeMs = 0;   // Server start time in milliseconds
uint8_t laneNumber = 9;   // Default lane number, can be changed later

// ---------- Interrupt Service Routines ----------
void IRAM_ATTR handleStart2Interrupt() {
  uint32_t now = millis();
  // Controleer of de knop daadwerkelijk HOOG is bij een RISING edge,
  // dit helpt bij het filteren van ruis als de pull-down niet perfect is.
  if (digitalRead(BUTTON_START_2_PIN) == HIGH && now - lastStart2Interrupt > DEBOUNCE_TIME_MS) {
    start2Interrupt = true;
    lastStart2Interrupt = now;
    // Serial.println("Start button 2 pressed"); // Verplaatst naar processStartLap()
  }
}

void IRAM_ATTR handleStartLapInterrupt() {
  uint32_t now = millis();
  if (now - lastStartLapInterrupt > DEBOUNCE_TIME_MS) {
    startLapInterrupt = true;
    lastStartLapInterrupt = now;
    // Serial.println("Start/Lap button pressed"); // Verplaatst naar processStartLap()
  }
}

void IRAM_ATTR handleStopInterrupt() {
  uint32_t now = millis();
  if (now - lastStopInterrupt > DEBOUNCE_TIME_MS) {
    stopInterrupt = true;
    lastStopInterrupt = now;
    // Serial.println("Stop button pressed"); // Verplaatst naar processStop()
  }
}

// --------------- Setup -----------------
void setup() {
  // Initialize Serial first for debugging
  Serial.begin(115200);
  Serial.println("Starting setup...");

  // Initialize display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(4);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  Serial.println("Display initialized"); 
  
  // Button setup with explicit GPIO configuration
  pinMode(BUTTON_START_LAP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_STOP_PIN, INPUT_PULLUP);
  // Aangepast: Gebruik INPUT voor BUTTON_START_2_PIN met externe pull-down
  pinMode(BUTTON_START_2_PIN, INPUT_PULLDOWN); // GPIO2 is connected to 3.3V, so we use INPUT_PULLUP
  Serial.println("Buttons configured");

  // Attach interrupts with explicit GPIO numbers
  attachInterrupt(BUTTON_START_LAP_PIN, handleStartLapInterrupt, FALLING);
  attachInterrupt(BUTTON_STOP_PIN, handleStopInterrupt, FALLING);
  // Aangepast: Gebruik RISING voor BUTTON_START_2_PIN (knop verbindt met 3.3V)
  attachInterrupt(BUTTON_START_2_PIN, handleStart2Interrupt, RISING);    
  Serial.println("Interrupts attached");

  resetStopwatch();
  drawStopwatch(0);
  Serial.println("Stopwatch initialized");

  // Initialize WiFi last
  //saveWiFiCredentials("SSID","PASSWORD"); // Uncomment to save credentials
  connectWiFi();
  setupWebSocket();
}

// --------------- Loop -----------------
void loop() {
  uint32_t now = millis();

  // ---- Interrupts verwerken ----
  if (startLapInterrupt) {
    startLapInterrupt = false;
    Serial.println("Start/Lap button pressed"); // Debug output verplaatst
    processStartLap();
  }
  if (start2Interrupt) { // Aparte check voor start2Interrupt
    start2Interrupt = false;
    Serial.println("Start button 2 pressed"); // Debug output verplaatst
    processStartLap(); // Roep dezelfde functie aan als Start/Lap
  }
  if (stopInterrupt) {
    stopInterrupt = false;
    Serial.println("Stop button pressed"); // Debug output verplaatst
    processStop();
  }

  // ---- WebSocket Loop ----
  if (WiFi.status() == WL_CONNECTED) {
    webSocket.loop();
  }

  // ---- Display Update ----
  uint32_t showElapsedMs = (stopwatchState == RUNNING)
      ? now - startTimeMs
      : elapsedMs;
  if (now - lastDisplayUpdateMs > DISPLAY_REFRESH_MS || lastDisplayUpdateMs == 0) {
    drawStopwatch(showElapsedMs);
    lastDisplayUpdateMs = now;
  }

  // ---- WebSocket loop ----
  webSocket.loop();
}

// ------- Functionaliteit ---------
void processStartLap() {
  if (stopwatchState == STOPPED) {
    startTimeMs = millis();
    lapCount = 0;
    stopwatchState = RUNNING;
  } else if (stopwatchState == RUNNING && lapCount < MAX_LAPS) {
    uint32_t nowMs = millis();
    uint32_t elapsedTime = nowMs - startTimeMs;
    addLap(nowMs - startTimeMs);
      // Send lap time via WebSocket
    if (wsConnected && serverStartTimeMs > 0) {
      StaticJsonDocument<200> doc;
      doc["type"] = "split";
      doc["lane"] = String(laneNumber);
      doc["time-ms"] = serverStartTimeMs + elapsedTime;
      doc["time"] = formatTime(elapsedTime);
      
      char jsonBuffer[200];
      serializeJson(doc, jsonBuffer);
      webSocket.sendTXT(jsonBuffer);
      
      // Debug output
      Serial.printf("Sending split for lane %d: %s\n", laneNumber, jsonBuffer);
    }
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
  
  // Calculate start index to show last DISPLAY_LAPS
  int startIdx = (lapCount > DISPLAY_LAPS) ? lapCount - DISPLAY_LAPS : 0;
  
  for (int i = startIdx; i < lapCount; ++i) {
    int displayRow = i - startIdx;
    tft.setCursor(10, 70 + displayRow*20);
    tft.printf("Lap %d: %s (%s)", i+1, formatTime(laps[i].lapTimeMs).c_str(), formatTime(laps[i].totalTimeMs).c_str());
  }
  
  if (lapCount > DISPLAY_LAPS) {
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(280, 70);
    tft.printf("%d+", startIdx);
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

// ------------- WiFi Connection Logic ---------------

void saveWiFiCredentials(const char* ssid, const char* password) {
    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
    Serial.println("WiFi credentials saved");
}

bool getWiFiCredentials(String &ssid, String &password) {
    preferences.begin("wifi", true);
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    preferences.end();
    return ssid.length() > 0 && password.length() > 0;
}

void showWiFiStatus(const char* message) {
    tft.fillRect(0, 60, 320, 80, TFT_BLACK);
    tft.setCursor(10, 70);
    tft.setTextFont(2);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.print(message);
}

// Connect using stored credentials
void connectWiFi() {
    String stored_ssid, stored_password;
    
    if (!getWiFiCredentials(stored_ssid, stored_password)) {
        Serial.println("No WiFi credentials stored!");
        showWiFiStatus("No WiFi credentials stored!");
        // Hier kunnen we later code toevoegen om credentials in te voeren
        return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(stored_ssid.c_str(), stored_password.c_str());
    Serial.printf("Connecting to %s\n", stored_ssid.c_str());

    tft.fillRect(0, 60, 320, 80, TFT_BLACK);
    tft.setCursor(10, 70);
    tft.setTextFont(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.printf("Connecting to %s...", stored_ssid.c_str());

    uint32_t startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startAttempt) < WIFI_CONNECT_TIMEOUT_MS) {
        delay(250);
        Serial.print(".");
        tft.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
        
        showWiFiStatus("WiFi connected!");
        tft.setCursor(10, 100);
        tft.print(WiFi.localIP());
    } else {
        Serial.println("\nWiFi connection failed!");
        showWiFiStatus("WiFi connection failed!");
    }
}

// ------------- WebSocket Logic ---------------

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("WebSocket Disconnected!");
            wsConnected = false;
            showWiFiStatus("WebSocket Disconnected!");
            break;
        case WStype_CONNECTED:
            Serial.println("WebSocket Connected!");
            wsConnected = true;
            showWiFiStatus("WebSocket Connected!");
            break;
        case WStype_TEXT: {
            Serial.printf("WebSocket received text: %s\n", payload);
            
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, payload);
            
            if (error) {
                Serial.print("deserializeJson() failed: ");
                Serial.println(error.c_str());
                return;
            }
              const char* msgType = doc["type"];
            if (strcmp(msgType, "start") == 0) {
                serverStartTimeMs = doc["time"].as<uint64_t>();
                if (stopwatchState == STOPPED) {
                    processStartLap();
                }
            } else if (strcmp(msgType, "reset") == 0) {
                if (stopwatchState == RUNNING) {
                    processStop();
                }
                resetStopwatch();
                Serial.println("Stopwatch reset via WebSocket");
            }
            break;
        }
    }
}

void setupWebSocket() {
    webSocket.beginSSL("scherm.azckamp.nl", 443);
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
    Serial.println("WebSocket setup completed");
}
