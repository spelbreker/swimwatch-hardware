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
    , currentState(STOPWATCH_STOPPED)
    , startTimeMs(0)
    , elapsedMs(0)
    , serverStartTimeMs(0)
    , timeOffset(0)
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
    
    // Send ping periodically if connected
    if (wsConnected && now - lastPingTime > PING_INTERVAL) {
        lastPingTime = now;
        webSocket.sendPing();
        Serial.println("Ping sent");
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
    serverStartTimeMs = 0;
    
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
    return serverStartTimeMs > 0;
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
    serverStartTimeMs = serverTime;
    updateTimeOffset(serverTime);
    
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
    if (!wsConnected || serverStartTimeMs == 0) {
        return;
    }
    
    uint64_t splitServerTime = serverStartTimeMs + elapsedTime;
    
    StaticJsonDocument<300> doc;
    doc["type"] = WS_MSG_SPLIT;
    doc["lane"] = String(laneNumber);
    doc["time-ms"] = splitServerTime;
    doc["time"] = formatTime(elapsedTime);
    
    String message;
    serializeJson(doc, message);
    sendMessage(message);
    
    Serial.printf("Split time sent for lane %d: elapsed=%s, server_time=%llu (compensated)\n", 
                  laneNumber, formatTime(elapsedTime).c_str(), splitServerTime);
}

void WebSocketStopwatch::sendMessage(const String& message) {
    if (wsConnected) {
        String msg = message;  // Create non-const copy
        webSocket.sendTXT(msg);
    }
}

void WebSocketStopwatch::updateTimeOffset(uint64_t serverTime) {
    uint64_t localTime = millis();
    
    // Apply lag compensation using half of the best ping time for better accuracy
    int64_t lagCompensation = 0;
    int pingToUse = (bestPingMs > 0 && pingSampleCount >= 3) ? bestPingMs : pingMs;
    
    if (pingToUse > 0) {
        lagCompensation = pingToUse / 2;  // Half of round-trip time
        Serial.printf("Lag compensation: %lld ms (using %s ping: %d ms)\n", 
                      lagCompensation, 
                      (pingToUse == bestPingMs) ? "best" : "current", 
                      pingToUse);
    }
    
    // Calculate time offset with lag compensation
    // Server time + lag compensation - local time = offset
    timeOffset = (int64_t)(serverTime + lagCompensation) - (int64_t)localTime;
    
    Serial.printf("Time sync - Server: %llu, Local: %llu, Lag comp: %lld ms, Offset: %lld ms\n", 
                  serverTime, localTime, lagCompensation, timeOffset);
    
    if (onTimeSync) {
        onTimeSync(true);
    }
}

uint64_t WebSocketStopwatch::getServerTime() {
    return millis() + timeOffset;
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
            
            // Reset ping statistics for fresh measurements on new connection
            bestPingMs = -1;
            pingSampleCount = 0;
            Serial.println("Ping statistics reset for new connection");
            
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
            if (strcmp(msgType, WS_MSG_START) == 0) {
                handleStartMessage(doc);
            } else if (strcmp(msgType, WS_MSG_RESET) == 0) {
                handleResetMessage(doc);
            } else if (strcmp(msgType, WS_MSG_TIME_SYNC) == 0) {
                handleTimeSyncMessage(doc);
            } else if (strcmp(msgType, WS_MSG_SPLIT) == 0) {
                handleSplitMessage(doc);
            } else if (strcmp(msgType, WS_MSG_EVENT_HEAT) == 0) {
                handleEventHeatMessage(doc);
            } else if (strcmp(msgType, WS_MSG_CLEAR) == 0) {
                handleClearMessage(doc);
            }
            break;
        }
        
        case WStype_PONG:
            lastPongTime = millis();
            pingMs = lastPongTime - lastPingTime;
            
            // Track best ping time for more accurate lag compensation
            if (bestPingMs == -1 || pingMs < bestPingMs) {
                bestPingMs = pingMs;
                Serial.printf("New best ping: %dms\n", bestPingMs);
            }
            pingSampleCount++;
            
            Serial.printf("Pong received - ping: %dms, best: %dms, samples: %d\n", 
                         pingMs, bestPingMs, pingSampleCount);
            break;
            
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

void WebSocketStopwatch::handleTimeSyncMessage(JsonDocument& doc) {
    if (doc.containsKey("server_time")) {
        uint64_t serverTime = doc["server_time"].as<uint64_t>();
        updateTimeOffset(serverTime);
    }
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
