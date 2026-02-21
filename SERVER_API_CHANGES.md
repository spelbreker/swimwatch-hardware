# WebSocket Server API Changes Required

## Issue: Update `start` Message to Support Microsecond Timestamps

### Summary

The ESP32 client devices have been updated to send and receive **microsecond-precision NTP timestamps** in `start` messages to enable network delay compensation and achieve ±1-2ms synchronization accuracy across all devices.

The WebSocket server needs to be updated to handle the new timestamp format.

---

## Current Behavior (Old Format)

**Starter device sends:**
```json
{
  "type": "start",
  "event": "100m Free",
  "heat": "3",
  "timestamp": 1234567890
}
```
- `timestamp` in **seconds** (Unix epoch)

**Problem:** 
- Only 1-second resolution
- Lane devices start timer when message arrives (5-50ms network delay)
- No network delay compensation possible
- Poor synchronization (±5-50ms between starter and lanes)

---

## Required Changes (New Format)

### 1. Accept Microsecond Timestamps in `start` Message

**Starter device now sends:**
```json
{
  "type": "start",
  "event": "100m Free",
  "heat": "3",
  "timestamp": 1234567890123,
  "timestamp_us": 456
}
```

**Fields:**
- `timestamp` (uint64): Unix epoch **milliseconds** from NTP-synced `gettimeofday()`
- `timestamp_us` (uint16): Sub-millisecond **microseconds** (0-999) for full microsecond precision

**Combined precision:** `timestamp` (ms) + `timestamp_us` (µs) = microsecond accuracy

### 2. Broadcast to Lane Devices

**Lane devices must receive:**
```json
{
  "type": "start",
  "event": "100m Free",
  "heat": "3",
  "timestamp": 1234567890123,
  "timestamp_us": 456
}
```

The server should:
1. Parse `timestamp` and `timestamp_us` from the starter's message
2. Forward both fields unchanged when broadcasting to lane devices
3. Maintain `event` and `heat` fields in the broadcast

### 3. Backward Compatibility Detection

The server can distinguish old vs new clients by checking for `timestamp_us` field:

```javascript
// Pseudo-code for server handling
if (message.timestamp_us !== undefined) {
    // New format: microsecond precision
    broadcast({
        type: "start",
        event: message.event,
        heat: message.heat,
        timestamp: message.timestamp,
        timestamp_us: message.timestamp_us
    });
} else if (message.timestamp > 10000000000) {
    // Intermediate format: milliseconds only (if you deployed that)
    broadcast({
        type: "start",
        event: message.event,
        heat: message.heat,
        timestamp: message.timestamp,
        timestamp_us: 0
    });
} else {
    // Old format: seconds
    broadcast({
        type: "start",
        event: message.event,
        heat: message.heat,
        timestamp: message.timestamp * 1000,
        timestamp_us: 0
    });
}
```

**Detection logic:**
1. If `timestamp_us` exists → new format (full microsecond precision)
2. Else if `timestamp > 10000000000` → milliseconds only
3. Else → old format (seconds), convert to milliseconds

---

## Client Behavior (For Reference)

### Starter Device (ESP32)
```cpp
void WebSocketStopwatch::sendStart(const String& event, const String& heat) {
    struct timeval tv;
    gettimeofday(&tv, nullptr);  // NTP-synced time
    uint64_t timestampMs = (uint64_t)tv.tv_sec * 1000ULL + (uint64_t)tv.tv_usec / 1000ULL;
    uint16_t timestampUs = (uint16_t)(tv.tv_usec % 1000);  // Sub-millisecond microseconds

    StaticJsonDocument<256> doc;
    doc["type"] = "start";
    doc["event"] = event;
    doc["heat"] = heat;
    doc["timestamp"] = timestampMs;      // Milliseconds since epoch
    doc["timestamp_us"] = timestampUs;   // Microseconds (0-999)
    
    sendMessage(doc);
}
```

### Lane Device (ESP32)
```cpp
void WebSocketStopwatch::handleRemoteStart(uint64_t timestampMs, uint16_t timestampUs) {
    if (timestampMs > 0) {
        // Calculate network delay with microsecond precision
        struct timeval now;
        gettimeofday(&now, nullptr);
        uint64_t nowMs = (uint64_t)now.tv_sec * 1000ULL + (uint64_t)now.tv_usec / 1000ULL;
        uint16_t nowUs = (uint16_t)(now.tv_usec % 1000);
        
        // Total delay in microseconds
        int64_t delayMs = (int64_t)(nowMs - timestampMs);
        int64_t delayUs = delayMs * 1000LL + (int64_t)nowUs - (int64_t)timestampUs;
        
        // Backdate timer start by the delay
        timer.startWithOffset(delayUs);
    } else {
        // Fallback for legacy servers (no timestamp)
        timer.start();
    }
}
```

---

## Benefits

✅ **Synchronization accuracy:** ±1-2ms (improved from ±5-50ms)  
✅ **Professional-grade timing:** Exceeds FINA requirements (±10ms)  
✅ **Network-independent:** Compensates for variable WiFi latency  
✅ **NTP-based:** Uses shared time server for clock agreement  
✅ **Microsecond precision:** Full sub-millisecond accuracy via `timestamp_us` field
✅ **Backward compatible:** Detection via `timestamp_us` field presence

---

## Testing

After implementing changes:

1. **Verify timestamp format:**
   - Starter sends start with `timestamp` (milliseconds) and `timestamp_us` (0-999)
   - All lane devices receive both fields
   - Verify microsecond component varies: `timestamp_us` should not always be 0

2. **Check synchronization:**
   - Start race with 1 starter + 8 lane devices
   - All devices should show identical elapsed time within ±2ms
   - Compare displays visually or via serial debug output

3. **Backward compatibility (if implemented):**
   - Test with old client sending `timestamp` in seconds only (no `timestamp_us`)
   - Server should detect and convert: `timestamp × 1000`, `timestamp_us = 0`
   - New clients should receive microsecond timestamp

---

## Migration Guide

### For Server Developers

**Minimal change (no backward compatibility):**
```javascript
// Just forward both timestamp fields unchanged
broadcast({
    type: "start",
    event: message.event,
    heat: message.heat,
    timestamp: message.timestamp,
    timestamp_us: message.timestamp_us
});
```

**With backward compatibility:**
```javascript
let timestamp = message.timestamp;
let timestamp_us = message.timestamp_us || 0;

// Detect old format (seconds) and convert to milliseconds
if (timestamp_us === 0 && timestamp < 10000000000) {
    timestamp = timestamp * 1000;
}

broadcast({
    type: "start",
    event: message.event,
    heat: message.heat,
    timestamp: timestamp,
    timestamp_us: timestamp_us
});
```

---

## Related Client Changes

**Repository:** LilyGO T-Display S3 SwimWatch firmware  
**Files modified:**
- `src/websocket_stopwatch.cpp` — Updated `sendStart()` and `handleRemoteStart()` to use microsecond timestamp
- `include/websocket_stopwatch.h` — Changed `handleRemoteStart(uint64_t timestampMs, uint16_t timestampUs)` signature
- `src/stopwatch_timer.cpp` — Added `startWithOffset(int64_t offsetUs)` method
- `include/stopwatch_timer.h` — Added `startWithOffset()` declaration
- `docs/API.md` — Updated WebSocket message format documentation
- `docs/DEVELOPER.md` — Added network delay compensation explanation

**Commit:** Network delay compensation via microsecond NTP timestamps

---

## Compatibility Matrix

| Client Version | `timestamp` Format | `timestamp_us` | Server Action | Result |
|----------------|-------------------|----------------|---------------|--------|
| **Old** | Seconds (e.g., `1234567890`) | Missing | Multiply by 1000 → `timestamp_us = 0` | Compatible |
| **Intermediate** | Milliseconds (e.g., `1234567890123`) | Missing | Forward as-is → `timestamp_us = 0` | Compatible |
| **New** | Milliseconds | 0-999 | Forward both fields → full µs precision | ±1-2ms sync |
| **New** receiving old server | Missing or `0` | Missing or `0` | Fallback to `start()` without offset | No sync, but works |

---

## Priority

**High** — Current client firmware expects microsecond timestamps (`timestamp` + `timestamp_us`) and will achieve significantly better synchronization with server-side support.

**Migration Path:** The change is backward compatible if the server implements field-based detection. Old clients will continue to work, new clients will gain sub-millisecond precision.

---

## Contact

For questions or clarification, please reference the ESP32 client implementation in the SwimWatch firmware repository.
