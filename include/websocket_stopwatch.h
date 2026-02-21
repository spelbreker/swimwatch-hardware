/**
 * @file websocket_stopwatch.h
 * @brief WebSocket client for remote swim meet competition timing
 *
 * Handles server communication for synchronized race timing:
 * - Receives start/reset/event-heat commands from server
 * - Sends split times with NTP-synced wall-clock timestamps
 * - Device registration (role, lane, MAC)
 * - Remote role/lane configuration from server
 *
 * Timing: Uses StopwatchTimer (esp_timer) for local elapsed precision.
 * Timestamps: Uses time() (NTP-synced RTC) for wall-clock values.
 */
#ifndef WEBSOCKET_STOPWATCH_H
#define WEBSOCKET_STOPWATCH_H

#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "stopwatch_timer.h"

// ── WebSocket message type constants ───────────────────────────
#define WS_MSG_PING               "ping"
#define WS_MSG_PONG               "pong"
#define WS_MSG_START              "start"
#define WS_MSG_RESET              "reset"
#define WS_MSG_SPLIT              "split"
#define WS_MSG_EVENT_HEAT         "event-heat"
#define WS_MSG_SELECT_EVENT       "select-event"
#define WS_MSG_CLEAR              "clear"
#define WS_MSG_DEVICE_REGISTER    "device_register"
#define WS_MSG_DEVICE_UPDATE_ROLE "device_update_role"
#define WS_MSG_DEVICE_UPDATE_LANE "device_update_lane"

/** Stopwatch states */
enum StopwatchState {
    STOPWATCH_STOPPED,
    STOPWATCH_RUNNING,
    STOPWATCH_PAUSED
};

/** Split time info received from other lanes (for starter display) */
struct SplitTimeInfo {
    uint8_t  lane;
    uint64_t timestamp;
    String   formattedTime;
    bool     isValid;
};

/**
 * @class WebSocketStopwatch
 * @brief Manages WebSocket connection and race protocol
 *
 * Wraps a StopwatchTimer for precise elapsed timing. Uses NTP-synced
 * time() for absolute timestamps sent to/from the server.
 */
class WebSocketStopwatch {
public:
    WebSocketStopwatch();

    // ── Configuration ──────────────────────────────────────────
    void setServerConfig(const String& host, uint16_t port,
                         const String& path = "/ws", bool ssl = false);
    void setLaneNumber(uint8_t lane);
    void setDeviceRole(const String& role);

    // ── Connection ─────────────────────────────────────────────
    bool connect();
    void disconnect();
    bool isConnected();
    void loop();

    // ── Stopwatch control ──────────────────────────────────────
    void start();
    void stop();
    void reset();
    void addLap();
    void sendStart(const String& event, const String& heat);

    // ── State queries ──────────────────────────────────────────
    StopwatchState getState();
    uint32_t       getElapsedTime();
    uint8_t        getLapCount();
    String         getCurrentEvent();
    String         getCurrentHeat();
    int            getPingMs();
    const SplitTimeInfo* getSplitTimes();

    // ── Display control ────────────────────────────────────────
    void clearSplitTimes();
    void clearDisplay();

    // ── Remote control (via WebSocket) ─────────────────────────
    void handleRemoteStart(uint64_t timestampMs, uint16_t timestampUs = 0);
    void handleRemoteReset();

    // ── Utility ────────────────────────────────────────────────
    String formatTime(uint32_t milliseconds);

    // ── Callbacks (set by main.cpp) ────────────────────────────
    void (*onStateChanged)(StopwatchState newState)                      = nullptr;
    void (*onLapAdded)(uint8_t lapNumber, uint32_t lapTime,
                       uint32_t totalTime)                               = nullptr;
    void (*onConnectionChanged)(bool connected)                          = nullptr;
    void (*onEventHeatChanged)(const String& event, const String& heat)  = nullptr;
    void (*onSplitTimeReceived)(uint8_t lane, const String& time)        = nullptr;
    void (*onDisplayClear)()                                             = nullptr;
    void (*onDeviceConfigChanged)(const String& role, uint8_t lane)      = nullptr;

private:
    WebSocketsClient webSocket;

    // ── Connection ─────────────────────────────────────────────
    String   serverHost;
    uint16_t serverPort;
    String   serverPath;
    bool     useSSL;
    bool     wsConnected;
    unsigned long lastReconnectAttempt;
    unsigned long lastPingTime;
    int      pingMs;

    // ── Device identity ────────────────────────────────────────
    String deviceMAC;
    String deviceRole;
    bool   isRegistered;

    // ── Stopwatch ──────────────────────────────────────────────
    StopwatchTimer   timer;
    StopwatchState   currentState;
    uint64_t         syncStartTimestamp;   ///< Server's start epoch (wall-clock)
    bool             startLocked;

    // ── Event / Heat ───────────────────────────────────────────
    String currentEvent;
    String currentHeat;

    // ── Lane split times (from server, for starter display) ────
    static const uint8_t MAX_LANES = 10;
    SplitTimeInfo splitTimes[MAX_LANES];

    // ── Lap tracking ───────────────────────────────────────────
    static const uint8_t MAX_LAPS = 90;
    uint8_t lapCount;
    uint8_t laneNumber;

    // ── Timing constants ───────────────────────────────────────
    static const unsigned long RECONNECT_INTERVAL = 5000;
    static const unsigned long PING_INTERVAL      = 5000;

    // ── Message handlers ───────────────────────────────────────
    static void webSocketEventWrapper(WStype_t type, uint8_t* payload, size_t length);
    void handleWebSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    void handleStartMessage(JsonDocument& doc);
    void handleResetMessage(JsonDocument& doc);
    void handleSplitMessage(JsonDocument& doc);
    void handleEventHeatMessage(JsonDocument& doc);
    void handleClearMessage(JsonDocument& doc);
    void handlePongMessage(JsonDocument& doc);
    void handleDeviceUpdateRoleMessage(JsonDocument& doc);
    void handleDeviceUpdateLaneMessage(JsonDocument& doc);

    // ── Network helpers ────────────────────────────────────────
    void sendSplitTime(uint32_t elapsedTime);
    void sendMessage(const String& message);
    void sendJsonPing();
    void sendDeviceRegistration();
};

#endif // WEBSOCKET_STOPWATCH_H
