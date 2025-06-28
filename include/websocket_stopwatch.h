#ifndef WEBSOCKET_STOPWATCH_H
#define WEBSOCKET_STOPWATCH_H

#include <WebSocketsClient.h>
#include <ArduinoJson.h>

// WebSocket message types
#define WS_MSG_PING "ping"
#define WS_MSG_PONG "pong"
#define WS_MSG_START "start"
#define WS_MSG_RESET "reset"
#define WS_MSG_SPLIT "split"
#define WS_MSG_EVENT_HEAT "event-heat"
#define WS_MSG_CLEAR "clear"

// Stopwatch states
enum StopwatchState {
    STOPWATCH_STOPPED,
    STOPWATCH_RUNNING,
    STOPWATCH_PAUSED
};

// Lap data structure
struct LapData {
    uint32_t lapTimeMs;
    uint32_t totalTimeMs;
    uint64_t serverTimestamp;
};

class WebSocketStopwatch {
private:
    WebSocketsClient webSocket;
    
    // Connection settings
    String serverHost;
    uint16_t serverPort;
    String serverPath;
    bool useSSL;
    
    // Connection state
    bool wsConnected;
    unsigned long lastReconnectAttempt;
    unsigned long lastPingTime;
    unsigned long lastPongTime;
    int pingMs;
    int bestPingMs; // Track best (lowest) ping time for more accurate lag compensation
    uint8_t pingSampleCount; // Number of ping samples collected
    int64_t serverTimeOffset; // Client time offset from server time
    bool timeSync; // Whether time synchronization is active
    static const unsigned long RECONNECT_INTERVAL = 5000;
    static const unsigned long PING_INTERVAL = 5000; // Send ping every 5 seconds (per new spec)
    static const uint8_t MAX_PING_SAMPLES = 10; // Number of samples to consider for best ping
    
    // Stopwatch state
    StopwatchState currentState;
    uint32_t startTimeMs;
    uint32_t elapsedMs;
    
    // Event and Heat information
    String currentEvent;
    String currentHeat;
    
    // Lane and split time information
    struct SplitTimeInfo {
        uint8_t lane;
        uint64_t timestamp;
        String formattedTime;
        bool isValid;
    };
    static const uint8_t MAX_LANES = 10;
    SplitTimeInfo splitTimes[MAX_LANES];
    
    // Lap management
    static const uint8_t MAX_LAPS = 90;
    LapData laps[MAX_LAPS];
    uint8_t lapCount;
    uint8_t laneNumber;
    
    // Timing
    unsigned long lastDisplayUpdate;
    static const unsigned long DISPLAY_REFRESH_INTERVAL = 50; // 20Hz
    
    // WebSocket event handlers
    static void webSocketEventWrapper(WStype_t type, uint8_t* payload, size_t length);
    void handleWebSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    void handleStartMessage(JsonDocument& doc);
    void handleResetMessage(JsonDocument& doc);
    void handleSplitMessage(JsonDocument& doc);
    void handleEventHeatMessage(JsonDocument& doc);
    void handleClearMessage(JsonDocument& doc);
    void handlePingMessage(JsonDocument& doc);
    void handlePongMessage(JsonDocument& doc);
    
    // Network and timing
    void sendSplitTime(uint32_t elapsedTime);
    void sendMessage(const String& message);
    void sendJsonPing(); // Send JSON-based ping message
    
    // Time synchronization
    uint64_t getServerTime();
    uint64_t getSynchronizedTime(); // Get current time with server offset applied
    
public:
    WebSocketStopwatch();
    
    // Configuration
    void setServerConfig(const String& host, uint16_t port, const String& path = "/ws", bool ssl = true);
    void setLaneNumber(uint8_t lane);
    
    // Connection management
    bool connect();
    void disconnect();
    bool isConnected();
    void loop();
    
    // Stopwatch control
    void start();
    void stop();
    void reset();
    void addLap();
    
    // State queries
    StopwatchState getState();
    uint32_t getElapsedTime();
    uint8_t getLapCount();
    const LapData* getLaps();
    bool hasServerTime();
    String getCurrentEvent();
    String getCurrentHeat();
    const SplitTimeInfo* getSplitTimes();
    int getPingMs(); // Get current ping time in milliseconds
    
    // Display control
    void clearSplitTimes();
    void clearDisplay();
    
    // Remote control (via WebSocket)
    void handleRemoteStart(uint64_t serverTime);
    void handleRemoteReset();
    
    // Time formatting
    String formatTime(uint32_t milliseconds);
    
    // Callbacks (to be set by main application)
    void (*onStateChanged)(StopwatchState newState);
    void (*onLapAdded)(uint8_t lapNumber, uint32_t lapTime, uint32_t totalTime);
    void (*onConnectionChanged)(bool connected);
    void (*onTimeSync)(bool synced);
    void (*onEventHeatChanged)(const String& event, const String& heat);
    void (*onSplitTimeReceived)(uint8_t lane, const String& time);
    void (*onDisplayClear)();
};

#endif // WEBSOCKET_STOPWATCH_H
