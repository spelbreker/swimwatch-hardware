#include "websocket_stopwatch.h"

// Static instance pointer for WebSocket callback
WebSocketStopwatch* wsStopwatchInstance = nullptr;

WebSocketStopwatch::WebSocketStopwatch() 
    : serverHost("scherm.azckamp.nl")
    , serverPort(443)
    , serverPath("/ws")
    , useSSL(true)
    , wsConnected(false)
    , lastReconnectAttempt(0)
    , lastPingTime(0)
    , lastPongTime(0)
    , pingMs(-1)
    , bestPingMs(-1)
    , pingSampleCount(0)
    , serverTimeOffset(0)
    , timeSync(false)
    , currentState(STOPWATCH_STOPPED)
    , startTimeMs(0)
    , elapsedMs(0)
    , lapCount(0)
    , laneNumber(9)
    , lastDisplayUpdate(0)
    , onStateChanged(nullptr)
    , onLapAdded(nullptr)
    , onConnectionChanged(nullptr)
    , onTimeSync(nullptr)
    , onEventHeatChanged(nullptr)
    , onSplitTimeReceived(nullptr)
    , onDisplayClear(nullptr) {
    
    // Set static instance for callback
    wsStopwatchInstance = this;
    
    // Initialize lap array
    for (uint8_t i = 0; i < MAX_LAPS; i++) {
        laps[i] = {0, 0, 0};
    }
    
    // Initialize split times array
    for (uint8_t i = 0; i < MAX_LANES; i++) {
        splitTimes[i] = {0, 0, "", false};
    }
    
    // Initialize event/heat
    currentEvent = "";
    currentHeat = "";
}

void WebSocketStopwatch::setServerConfig(const String& host, uint16_t port, const String& path, bool ssl) {
    serverHost = host;
    serverPort = port;
    serverPath = path;
    useSSL = ssl;
    
    Serial.printf("WebSocket server config: %s%s:%d%s\n", 
                  ssl ? "wss://" : "ws://", host.c_str(), port, path.c_str());
}

void WebSocketStopwatch::setLaneNumber(uint8_t lane) {
    laneNumber = lane;
    Serial.printf("Lane number set to: %d\n", laneNumber);
}

bool WebSocketStopwatch::connect() {
    Serial.println("Connecting to WebSocket server...");
    
    if (useSSL) {
        webSocket.beginSSL(serverHost.c_str(), serverPort, serverPath.c_str());
    } else {
        webSocket.begin(serverHost.c_str(), serverPort, serverPath.c_str());
    }
    
    webSocket.onEvent(webSocketEventWrapper);
    webSocket.setReconnectInterval(RECONNECT_INTERVAL);
    webSocket.enableHeartbeat(15000, 3000, 2); // Enable heartbeat
    
    Serial.println("WebSocket connection initiated");
    return true;
}

void WebSocketStopwatch::disconnect() {
    webSocket.disconnect();
    wsConnected = false;
    Serial.println("WebSocket disconnected");
    
    if (onConnectionChanged) {
        onConnectionChanged(false);
    }
}

bool WebSocketStopwatch::isConnected() {
    return wsConnected;
}

void WebSocketStopwatch::loop() {
    webSocket.loop();
    
    unsigned long now = millis();
    
    // Handle reconnection if needed
    if (!wsConnected && now - lastReconnectAttempt > RECONNECT_INTERVAL) {
        lastReconnectAttempt = now;
        Serial.println("Attempting WebSocket reconnection...");
    }
    
    // Send JSON ping with appropriate interval based on sync state
    unsigned long pingInterval = PING_INTERVAL;
    
    // Initial rapid ping sequence (5 pings at 500ms intervals) if not synced
    if (!timeSync && pingSampleCount < 5) {
        pingInterval = 500; // 500ms for rapid initial sync
    }
    
    if (wsConnected && now - lastPingTime > pingInterval) {
        lastPingTime = now;
        sendJsonPing();
        
        if (!timeSync && pingSampleCount < 5) {
            Serial.printf("Initial ping %d/5 sent\n", pingSampleCount + 1);
        } else {
            Serial.println("Regular JSON ping sent");
        }
    }
}

void WebSocketStopwatch::start() {
    if (currentState != STOPWATCH_RUNNING) {
        startTimeMs = millis();
        currentState = STOPWATCH_RUNNING;
        lapCount = 0;
        
        Serial.println("Stopwatch started locally");
        
        if (onStateChanged) {
            onStateChanged(currentState);
        }
    }
}

void WebSocketStopwatch::stop() {
    if (currentState == STOPWATCH_RUNNING) {
        elapsedMs = millis() - startTimeMs;
        currentState = STOPWATCH_STOPPED;
        
        Serial.printf("Stopwatch stopped at: %s\n", formatTime(elapsedMs).c_str());
        
        if (onStateChanged) {
            onStateChanged(currentState);
        }
    }
}

void WebSocketStopwatch::reset() {
    currentState = STOPWATCH_STOPPED;
    startTimeMs = 0;
    elapsedMs = 0;
    lapCount = 0;
    
    // Clear lap data
    for (uint8_t i = 0; i < MAX_LAPS; i++) {
        laps[i] = {0, 0, 0};
    }
    
    // Clear split times
    clearSplitTimes();
    
    Serial.println("Stopwatch reset");
    
    if (onStateChanged) {
        onStateChanged(currentState);
    }
}

void WebSocketStopwatch::addLap() {
    if (currentState == STOPWATCH_RUNNING && lapCount < MAX_LAPS) {
        uint32_t currentElapsed = millis() - startTimeMs;
        uint32_t lapTime = currentElapsed;
        
        if (lapCount > 0) {
            lapTime = currentElapsed - laps[lapCount - 1].totalTimeMs;
        }
        
        laps[lapCount].lapTimeMs = lapTime;
        laps[lapCount].totalTimeMs = currentElapsed;
        laps[lapCount].serverTimestamp = getServerTime();
        
        lapCount++;
        
        Serial.printf("Lap %d added: %s (Total: %s)\n", 
                      lapCount, formatTime(lapTime).c_str(), formatTime(currentElapsed).c_str());
        
        // Send split time via WebSocket
        sendSplitTime(currentElapsed);
        
        if (onLapAdded) {
            onLapAdded(lapCount, lapTime, currentElapsed);
        }
    }
}

StopwatchState WebSocketStopwatch::getState() {
    return currentState;
}

uint32_t WebSocketStopwatch::getElapsedTime() {
    if (currentState == STOPWATCH_RUNNING) {
        return millis() - startTimeMs;
    }
    return elapsedMs;
}

uint8_t WebSocketStopwatch::getLapCount() {
    return lapCount;
}

const LapData* WebSocketStopwatch::getLaps() {
    return laps;
}

bool WebSocketStopwatch::hasServerTime() {
    return timeSync; // Return true if we have time synchronization active
}

String WebSocketStopwatch::getCurrentEvent() {
    return currentEvent;
}

String WebSocketStopwatch::getCurrentHeat() {
    return currentHeat;
}

const WebSocketStopwatch::SplitTimeInfo* WebSocketStopwatch::getSplitTimes() {
    return splitTimes;
}

void WebSocketStopwatch::clearSplitTimes() {
    for (uint8_t i = 0; i < MAX_LANES; i++) {
        splitTimes[i] = {0, 0, "", false};
    }
    Serial.println("Split times cleared");
}

void WebSocketStopwatch::clearDisplay() {
    clearSplitTimes();
    currentEvent = "";
    currentHeat = "";
    
    if (onDisplayClear) {
        onDisplayClear();
    }
    
    Serial.println("Display cleared");
}

void WebSocketStopwatch::handleRemoteStart(uint64_t serverTime) {
    if (currentState != STOPWATCH_RUNNING) {
        start();
        Serial.printf("Remote start received with server time: %llu\n", serverTime);
    }
}

void WebSocketStopwatch::handleRemoteReset() {
    reset();
    Serial.println("Remote reset received");
}

String WebSocketStopwatch::formatTime(uint32_t milliseconds) {
    uint16_t minutes = milliseconds / 60000;
    uint8_t seconds = (milliseconds / 1000) % 60;
    uint8_t centiseconds = (milliseconds % 1000) / 10;
    
    char buffer[16];
    sprintf(buffer, "%02d:%02d:%02d", minutes, seconds, centiseconds);
    return String(buffer);
}

void WebSocketStopwatch::sendSplitTime(uint32_t elapsedTime) {
    if (!wsConnected) {
        return;
    }
    
    // Use synchronized time for split timestamp
    uint64_t splitTimestamp = getSynchronizedTime();
    
    StaticJsonDocument<300> doc;
    doc["type"] = WS_MSG_SPLIT;
    doc["lane"] = laneNumber; // Use integer instead of string per new spec
    doc["timestamp"] = splitTimestamp; // Use synchronized timestamp
    
    String message;
    serializeJson(doc, message);
    sendMessage(message);
    
    Serial.printf("Split time sent for lane %d: timestamp=%llu (synchronized)\n", 
                  laneNumber, splitTimestamp);
}

void WebSocketStopwatch::sendMessage(const String& message) {
    if (wsConnected) {
        String msg = message;  // Create non-const copy
        webSocket.sendTXT(msg);
    }
}

uint64_t WebSocketStopwatch::getServerTime() {
    // Use the synchronized time from ping/pong 
    return getSynchronizedTime();
}

// Static WebSocket event wrapper
void WebSocketStopwatch::webSocketEventWrapper(WStype_t type, uint8_t* payload, size_t length) {
    if (wsStopwatchInstance) {
        wsStopwatchInstance->handleWebSocketEvent(type, payload, length);
    }
}

void WebSocketStopwatch::handleWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.println("WebSocket Disconnected!");
            wsConnected = false;
            if (onConnectionChanged) {
                onConnectionChanged(false);
            }
            break;
            
        case WStype_CONNECTED:
            Serial.printf("WebSocket Connected to: %s\n", payload);
            wsConnected = true;
            
            // Reset synchronization state for fresh measurements on new connection
            bestPingMs = -1;
            pingSampleCount = 0;
            timeSync = false;
            serverTimeOffset = 0;
            Serial.println("Time sync reset for new connection - starting initial ping sequence");
            
            // Start initial rapid ping sequence (per new spec)
            lastPingTime = 0; // Force immediate ping
            
            if (onConnectionChanged) {
                onConnectionChanged(true);
            }
            break;
            
        case WStype_TEXT: {
            Serial.printf("WebSocket received: %s\n", payload);
            
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, payload);
            
            if (error) {
                Serial.printf("JSON parse error: %s\n", error.c_str());
                return;
            }
            
            const char* msgType = doc["type"];
            if (strcmp(msgType, WS_MSG_PING) == 0) {
                handlePingMessage(doc);
            } else if (strcmp(msgType, WS_MSG_PONG) == 0) {
                handlePongMessage(doc);
            } else if (strcmp(msgType, WS_MSG_START) == 0) {
                handleStartMessage(doc);
            } else if (strcmp(msgType, WS_MSG_RESET) == 0) {
                handleResetMessage(doc);
            } else if (strcmp(msgType, WS_MSG_SPLIT) == 0) {
                handleSplitMessage(doc);
            } else if (strcmp(msgType, WS_MSG_EVENT_HEAT) == 0) {
                handleEventHeatMessage(doc);
            } else if (strcmp(msgType, WS_MSG_CLEAR) == 0) {
                handleClearMessage(doc);
            }
            break;
        }
        
        case WStype_ERROR:
            Serial.printf("WebSocket Error: %s\n", payload);
            break;
            
        default:
            break;
    }
}

void WebSocketStopwatch::handleStartMessage(JsonDocument& doc) {
    if (doc.containsKey("timestamp")) {
        uint64_t serverTime = doc["timestamp"].as<uint64_t>();
        handleRemoteStart(serverTime);
    } else {
        start(); // Start without server time
    }
}

void WebSocketStopwatch::handleResetMessage(JsonDocument& doc) {
    handleRemoteReset();
}

void WebSocketStopwatch::handleSplitMessage(JsonDocument& doc) {
    if (doc.containsKey("lane") && doc.containsKey("timestamp")) {
        uint8_t lane = doc["lane"].as<uint8_t>();
        uint64_t timestamp = doc["timestamp"].as<uint64_t>();
        String timeStr = doc.containsKey("time") ? doc["time"].as<String>() : "00:00:00";
        
        if (lane < MAX_LANES) {
            splitTimes[lane].lane = lane;
            splitTimes[lane].timestamp = timestamp;
            splitTimes[lane].formattedTime = timeStr;
            splitTimes[lane].isValid = true;
            
            Serial.printf("Split time received for lane %d: %s\n", lane, timeStr.c_str());
            
            if (onSplitTimeReceived) {
                onSplitTimeReceived(lane, timeStr);
            }
        }
    }
}

void WebSocketStopwatch::handleEventHeatMessage(JsonDocument& doc) {
    if (doc.containsKey("event") && doc.containsKey("heat")) {
        currentEvent = doc["event"].as<String>();
        currentHeat = doc["heat"].as<String>();
        
        Serial.printf("Event/Heat updated: %s / %s\n", currentEvent.c_str(), currentHeat.c_str());
        
        if (onEventHeatChanged) {
            onEventHeatChanged(currentEvent, currentHeat);
        }
    }
}

void WebSocketStopwatch::handleClearMessage(JsonDocument& doc) {
    clearDisplay();
}

int WebSocketStopwatch::getPingMs() {
    return pingMs;
}

void WebSocketStopwatch::sendJsonPing() {
    StaticJsonDocument<128> doc;
    doc["type"] = WS_MSG_PING;
    doc["time"] = millis(); // Send current client time
    
    String message;
    serializeJson(doc, message);
    sendMessage(message);
}

void WebSocketStopwatch::handlePingMessage(JsonDocument& doc) {
    // Server sent us a ping, we should respond with pong
    // This is unusual but we handle it per spec
    StaticJsonDocument<128> response;
    response["type"] = WS_MSG_PONG;
    response["client_ping_time"] = doc["time"];
    response["server_time"] = millis(); // Our time (acting as server)
    
    String message;
    serializeJson(response, message);
    sendMessage(message);
    
    Serial.println("Responded to server ping with pong");
}

void WebSocketStopwatch::handlePongMessage(JsonDocument& doc) {
    // Server responded to our ping
    lastPongTime = millis();
    
    if (doc.containsKey("client_ping_time") && doc.containsKey("server_time")) {
        uint64_t clientPingTime = doc["client_ping_time"];
        uint64_t serverTime = doc["server_time"];
        
        // Calculate round-trip time
        pingMs = lastPongTime - clientPingTime;
        
        // Calculate server time offset using: offset = server_time - client_time - rtt/2
        int64_t clientTime = lastPongTime;
        serverTimeOffset = serverTime - clientTime - (pingMs / 2);
        timeSync = true;
        
        // Track best ping time for more accurate lag compensation
        if (bestPingMs == -1 || pingMs < bestPingMs) {
            bestPingMs = pingMs;
            Serial.printf("New best ping: %dms\n", bestPingMs);
        }
        pingSampleCount++;
        
        Serial.printf("Pong received - ping: %dms, best: %dms, offset: %lldms, samples: %d\n", 
                     pingMs, bestPingMs, serverTimeOffset, pingSampleCount);
        
        if (onTimeSync) {
            onTimeSync(timeSync);
        }
    } else {
        Serial.println("Invalid pong message format");
    }
}

uint64_t WebSocketStopwatch::getSynchronizedTime() {
    if (timeSync) {
        return millis() + serverTimeOffset;
    }
    return millis(); // Fallback to local time if not synchronized
}
