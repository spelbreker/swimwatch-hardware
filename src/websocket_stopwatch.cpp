/**
 * @file websocket_stopwatch.cpp
 * @brief WebSocket client for remote swim meet competition timing
 *
 * Changes from original:
 * - Removed custom ping/pong RTT time sync (replaced by NTP)
 * - Uses StopwatchTimer (esp_timer_get_time) for elapsed timing
 * - Uses time() (NTP-synced) for wall-clock timestamps
 * - Simplified constructor and state management
 * - Reduced logging behind DEBUG_LOG macro
 */
#include "websocket_stopwatch.h"
#include "config.h"
#include <WiFi.h>
#include <time.h>
#include <sys/time.h>

// Static instance pointer for WebSocket callback
static WebSocketStopwatch* wsInstance = nullptr;

// ═══════════════════════════════════════════════════════════════
// Construction & Configuration
// ═══════════════════════════════════════════════════════════════

WebSocketStopwatch::WebSocketStopwatch()
    : serverHost(DEFAULT_SERVER_IP)
    , serverPort(DEFAULT_SERVER_PORT)
    , serverPath(DEFAULT_WS_PATH)
    , useSSL(false)
    , wsConnected(false)
    , lastReconnectAttempt(0)
    , lastPingTime(0)
    , pingMs(-1)
    , deviceMAC("")
    , deviceRole("lane")
    , isRegistered(false)
    , currentState(STOPWATCH_STOPPED)
    , syncStartTimestamp(0)
    , startLocked(false)
    , currentEvent("")
    , currentHeat("")
    , lapCount(0)
    , laneNumber(9) {

    wsInstance = this;

    for (uint8_t i = 0; i < MAX_LANES; i++) {
        splitTimes[i] = {0, 0, "", false};
    }
}

void WebSocketStopwatch::setServerConfig(const String& host, uint16_t port,
                                          const String& path, bool ssl) {
    serverHost = host;
    serverPort = port;
    serverPath = path;
    useSSL = ssl;
    DEBUG_LOG("WS config: %s%s:%d%s", ssl ? "wss://" : "ws://",
              host.c_str(), port, path.c_str());
}

void WebSocketStopwatch::setLaneNumber(uint8_t lane) {
    laneNumber = lane;
    DEBUG_LOG("Lane: %d", laneNumber);
}

void WebSocketStopwatch::setDeviceRole(const String& role) {
    deviceRole = role;
    DEBUG_LOG("Role: %s", deviceRole.c_str());
}

// ═══════════════════════════════════════════════════════════════
// Connection Management
// ═══════════════════════════════════════════════════════════════

bool WebSocketStopwatch::connect() {
    DEBUG_LOG("Connecting to WebSocket server...");
    deviceMAC = WiFi.macAddress();

    if (useSSL) {
        webSocket.beginSSL(serverHost.c_str(), serverPort, serverPath.c_str());
    } else {
        webSocket.begin(serverHost.c_str(), serverPort, serverPath.c_str());
    }

    webSocket.onEvent(webSocketEventWrapper);
    webSocket.setReconnectInterval(RECONNECT_INTERVAL);
    webSocket.enableHeartbeat(15000, 3000, 2);
    return true;
}

void WebSocketStopwatch::disconnect() {
    webSocket.disconnect();
    wsConnected = false;
    isRegistered = false;
    if (onConnectionChanged) onConnectionChanged(false);
}

bool WebSocketStopwatch::isConnected() {
    return wsConnected;
}

void WebSocketStopwatch::loop() {
    webSocket.loop();

    unsigned long now = millis();

    // Periodic JSON ping for latency measurement
    if (wsConnected && (now - lastPingTime > PING_INTERVAL)) {
        lastPingTime = now;
        sendJsonPing();
    }
}

// ═══════════════════════════════════════════════════════════════
// Stopwatch Control
// ═══════════════════════════════════════════════════════════════

void WebSocketStopwatch::start() {
    if (currentState == STOPWATCH_RUNNING) return;
    timer.start();
    currentState = STOPWATCH_RUNNING;
    lapCount = 0;
    DEBUG_LOG("Stopwatch started");
    if (onStateChanged) onStateChanged(currentState);
}

void WebSocketStopwatch::stop() {
    if (currentState != STOPWATCH_RUNNING) return;
    timer.stop();
    currentState = STOPWATCH_STOPPED;
    DEBUG_LOG("Stopwatch stopped at %u ms", timer.getElapsedMs());
    if (onStateChanged) onStateChanged(currentState);
}

void WebSocketStopwatch::reset() {
    timer.reset();
    currentState = STOPWATCH_STOPPED;
    syncStartTimestamp = 0;
    lapCount = 0;
    clearSplitTimes();
    DEBUG_LOG("Stopwatch reset");
    if (onStateChanged) onStateChanged(currentState);
}

void WebSocketStopwatch::addLap(int64_t capturedTimeUs) {
    if (currentState != STOPWATCH_RUNNING || lapCount >= MAX_LAPS) return;

    // Use ISR-captured timestamp if available, otherwise current time
    uint32_t currentElapsed;
    if (capturedTimeUs > 0) {
        timer.addSplit(laneNumber, capturedTimeUs);
        currentElapsed = timer.getSplits().back().elapsedMs;
    } else {
        currentElapsed = timer.getElapsedMs();
        timer.addSplit(laneNumber);
    }

    // Calculate lap time (delta from previous split)
    const auto& splits = timer.getSplits();
    uint32_t lapTime = currentElapsed;
    if (splits.size() > 1) {
        lapTime = currentElapsed - splits[splits.size() - 2].elapsedMs;
    }

    lapCount++;
    DEBUG_LOG("Lap %d: %u ms (total: %u ms)", lapCount, lapTime, currentElapsed);

    // Send split to server with NTP wall-clock timestamp
    sendSplitTime(currentElapsed);

    if (onLapAdded) {
        onLapAdded(lapCount, lapTime, currentElapsed);
    }
}

void WebSocketStopwatch::sendStart(const String& event, const String& heat) {
    if (!wsConnected) return;
    if (startLocked || currentState == STOPWATCH_RUNNING) {
        DEBUG_LOG("Start blocked: locked or already running");
        return;
    }

    // Capture NTP-synced wall-clock with microsecond precision
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    uint64_t timestampMs = (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)tv.tv_usec / 1000ULL;
    uint16_t timestampUs = (uint16_t)(tv.tv_usec % 1000);  // Sub-millisecond microseconds (0-999)

    StaticJsonDocument<256> doc;
    doc["type"] = WS_MSG_START;
    doc["event"] = event;
    doc["heat"] = heat;
    doc["timestamp"] = timestampMs;      // Milliseconds since epoch
    doc["timestamp_us"] = timestampUs;  // Microseconds component (0-999)

    String message;
    serializeJson(doc, message);
    sendMessage(message);
    startLocked = true;
    DEBUG_LOG("Start sent: event=%s heat=%s timestamp=%llu.%03u ms",
             event.c_str(), heat.c_str(), timestampMs, timestampUs);
}

// ═══════════════════════════════════════════════════════════════
// State Queries
// ═══════════════════════════════════════════════════════════════

StopwatchState WebSocketStopwatch::getState()       { return currentState; }
uint32_t WebSocketStopwatch::getElapsedTime()       { return timer.getElapsedMs(); }
uint8_t  WebSocketStopwatch::getLapCount()           { return lapCount; }
String   WebSocketStopwatch::getCurrentEvent()       { return currentEvent; }
String   WebSocketStopwatch::getCurrentHeat()        { return currentHeat; }
int      WebSocketStopwatch::getPingMs()             { return pingMs; }

const SplitTimeInfo* WebSocketStopwatch::getSplitTimes() {
    return splitTimes;
}

// ═══════════════════════════════════════════════════════════════
// Display Control
// ═══════════════════════════════════════════════════════════════

void WebSocketStopwatch::clearSplitTimes() {
    for (uint8_t i = 0; i < MAX_LANES; i++) {
        splitTimes[i] = {0, 0, "", false};
    }
}

void WebSocketStopwatch::clearDisplay() {
    clearSplitTimes();
    currentEvent = "";
    currentHeat = "";
    if (onDisplayClear) onDisplayClear();
}

// ═══════════════════════════════════════════════════════════════
// Remote Control
// ═══════════════════════════════════════════════════════════════

void WebSocketStopwatch::handleRemoteStart(uint64_t timestampMs, uint16_t timestampUs) {
    syncStartTimestamp = timestampMs;
    if (currentState != STOPWATCH_RUNNING) {
        if (timestampMs > 0) {
            // Calculate how long ago the start actually happened using
            // NTP-synced clocks on both devices. This compensates for
            // the WebSocket message delivery delay (typically 5-50ms LAN).
            struct timeval now;
            gettimeofday(&now, nullptr);
            uint64_t nowMs = (uint64_t)now.tv_sec * 1000ULL + (uint64_t)now.tv_usec / 1000ULL;
            uint16_t nowUs = (uint16_t)(now.tv_usec % 1000);
            
            // Calculate total delay in microseconds
            int64_t delayMs = (int64_t)(nowMs - timestampMs);
            int64_t delayUs = delayMs * 1000LL + (int64_t)nowUs - (int64_t)timestampUs;
            if (delayUs < 0) delayUs = 0;  // clock skew guard
            
            timer.startWithOffset(delayUs);
            currentState = STOPWATCH_RUNNING;
            lapCount = 0;
            DEBUG_LOG("Remote start, offset %lld µs (%.3f ms)",
                     delayUs, delayUs / 1000.0);
            if (onStateChanged) onStateChanged(currentState);
        } else {
            // No timestamp — fall back to starting NOW (legacy server)
            start();
            DEBUG_LOG("Remote start (no timestamp, no offset)");
        }
    }
}

void WebSocketStopwatch::handleRemoteReset() {
    reset();
    DEBUG_LOG("Remote reset");
}

// ═══════════════════════════════════════════════════════════════
// Utility
// ═══════════════════════════════════════════════════════════════

String WebSocketStopwatch::formatTime(uint32_t milliseconds) {
    char buf[16];
    StopwatchTimer::formatMs(milliseconds, true, buf, sizeof(buf));
    return String(buf);
}

// ═══════════════════════════════════════════════════════════════
// Network Helpers
// ═══════════════════════════════════════════════════════════════

void WebSocketStopwatch::sendSplitTime(uint32_t elapsedTime) {
    if (!wsConnected) return;

    // NTP wall-clock in milliseconds (same precision as sendStart)
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    uint64_t timestampMs = (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)tv.tv_usec / 1000ULL;

    StaticJsonDocument<300> doc;
    doc["type"] = WS_MSG_SPLIT;
    doc["lane"] = laneNumber;
    doc["elapsed_ms"] = elapsedTime;
    doc["timestamp"] = timestampMs;  // Milliseconds since epoch (NTP-synced)

    String message;
    serializeJson(doc, message);
    sendMessage(message);
    DEBUG_LOG("Split sent: lane %d, elapsed %u ms", laneNumber, elapsedTime);
}

void WebSocketStopwatch::sendMessage(const String& message) {
    if (wsConnected) {
        String msg = message;
        webSocket.sendTXT(msg);
    }
}

void WebSocketStopwatch::sendJsonPing() {
    StaticJsonDocument<128> doc;
    doc["type"] = WS_MSG_PING;
    doc["time"] = millis();  // For RTT measurement only (not time sync)

    String message;
    serializeJson(doc, message);
    sendMessage(message);
}

void WebSocketStopwatch::sendDeviceRegistration() {
    if (!wsConnected || deviceMAC.isEmpty()) return;

    StaticJsonDocument<300> doc;
    doc["type"] = WS_MSG_DEVICE_REGISTER;
    doc["mac"] = deviceMAC;
    doc["ip"] = WiFi.localIP().toString();
    doc["role"] = deviceRole;
    if (deviceRole == "lane") {
        doc["lane"] = laneNumber;
    }

    String message;
    serializeJson(doc, message);
    sendMessage(message);
    isRegistered = true;
    DEBUG_LOG("Registered: role=%s, MAC=%s", deviceRole.c_str(), deviceMAC.c_str());
}

// ═══════════════════════════════════════════════════════════════
// WebSocket Event Handling
// ═══════════════════════════════════════════════════════════════

void WebSocketStopwatch::webSocketEventWrapper(WStype_t type, uint8_t* payload, size_t length) {
    if (wsInstance) wsInstance->handleWebSocketEvent(type, payload, length);
}

void WebSocketStopwatch::handleWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            DEBUG_LOG("WS disconnected");
            wsConnected = false;
            if (onConnectionChanged) onConnectionChanged(false);
            break;

        case WStype_CONNECTED:
            DEBUG_LOG("WS connected to: %s", payload);
            wsConnected = true;
            isRegistered = false;
            pingMs = -1;
            lastPingTime = 0;  // Force immediate ping
            if (onConnectionChanged) onConnectionChanged(true);
            break;

        case WStype_TEXT: {
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, payload);
            if (error) {
                DEBUG_LOG("JSON parse error: %s", error.c_str());
                return;
            }

            const char* msgType = doc["type"];
            if (!msgType) return;

            // Dispatch to handler
            if      (strcmp(msgType, WS_MSG_START) == 0)              handleStartMessage(doc);
            else if (strcmp(msgType, WS_MSG_RESET) == 0)              handleResetMessage(doc);
            else if (strcmp(msgType, WS_MSG_SPLIT) == 0)              handleSplitMessage(doc);
            else if (strcmp(msgType, WS_MSG_EVENT_HEAT) == 0 ||
                     strcmp(msgType, WS_MSG_SELECT_EVENT) == 0)       handleEventHeatMessage(doc);
            else if (strcmp(msgType, WS_MSG_CLEAR) == 0)              handleClearMessage(doc);
            else if (strcmp(msgType, WS_MSG_PONG) == 0)               handlePongMessage(doc);
            else if (strcmp(msgType, WS_MSG_DEVICE_UPDATE_ROLE) == 0) handleDeviceUpdateRoleMessage(doc);
            else if (strcmp(msgType, WS_MSG_DEVICE_UPDATE_LANE) == 0) handleDeviceUpdateLaneMessage(doc);
            break;
        }

        case WStype_ERROR:
            DEBUG_LOG("WS error: %s", payload);
            break;

        default:
            break;
    }
}

// ═══════════════════════════════════════════════════════════════
// Message Handlers
// ═══════════════════════════════════════════════════════════════

void WebSocketStopwatch::handleStartMessage(JsonDocument& doc) {
    uint64_t timestamp = doc.containsKey("timestamp") ? doc["timestamp"].as<uint64_t>() : 0;
    uint16_t timestampUs = doc.containsKey("timestamp_us") ? doc["timestamp_us"].as<uint16_t>() : 0;
    handleRemoteStart(timestamp, timestampUs);
    startLocked = true;
}

void WebSocketStopwatch::handleResetMessage(JsonDocument& doc) {
    handleRemoteReset();
    startLocked = false;
}

void WebSocketStopwatch::handleSplitMessage(JsonDocument& doc) {
    if (!doc.containsKey("lane") || !doc.containsKey("timestamp")) return;

    uint8_t lane = doc["lane"].as<uint8_t>();
    uint64_t timestamp = doc["timestamp"].as<uint64_t>();
    String timeStr = doc.containsKey("time") ? doc["time"].as<String>() : "00:00.00";

    if (lane < MAX_LANES) {
        splitTimes[lane] = { lane, timestamp, timeStr, true };
        DEBUG_LOG("Split received: lane %d = %s", lane, timeStr.c_str());
        if (onSplitTimeReceived) onSplitTimeReceived(lane, timeStr);
    }
}

void WebSocketStopwatch::handleEventHeatMessage(JsonDocument& doc) {
    if (!doc.containsKey("event") || !doc.containsKey("heat")) return;
    currentEvent = doc["event"].as<String>();
    currentHeat = doc["heat"].as<String>();
    DEBUG_LOG("Event/Heat: %s/%s", currentEvent.c_str(), currentHeat.c_str());
    if (onEventHeatChanged) onEventHeatChanged(currentEvent, currentHeat);
}

void WebSocketStopwatch::handleClearMessage(JsonDocument& doc) {
    clearDisplay();
}

void WebSocketStopwatch::handlePongMessage(JsonDocument& doc) {
    if (!doc.containsKey("client_ping_time")) return;
    uint64_t clientPingTime = doc["client_ping_time"];
    pingMs = millis() - clientPingTime;
    DEBUG_LOG("Pong: %d ms", pingMs);

    // Register device after first successful pong
    if (!isRegistered) {
        sendDeviceRegistration();
    }
}

void WebSocketStopwatch::handleDeviceUpdateRoleMessage(JsonDocument& doc) {
    if (!doc.containsKey("mac") || !doc.containsKey("role")) return;
    String mac = doc["mac"].as<String>();
    String role = doc["role"].as<String>();

    if (mac == deviceMAC && (role == "lane" || role == "starter")) {
        deviceRole = role;
        DEBUG_LOG("Role updated: %s", deviceRole.c_str());
        if (onDeviceConfigChanged) onDeviceConfigChanged(deviceRole, laneNumber);
    }
}

void WebSocketStopwatch::handleDeviceUpdateLaneMessage(JsonDocument& doc) {
    if (!doc.containsKey("mac") || !doc.containsKey("lane")) return;
    String mac = doc["mac"].as<String>();
    uint8_t lane = doc["lane"].as<uint8_t>();

    if (mac == deviceMAC) {
        laneNumber = lane;
        DEBUG_LOG("Lane updated: %d", laneNumber);
        if (onDeviceConfigChanged) onDeviceConfigChanged(deviceRole, laneNumber);
    }
}
