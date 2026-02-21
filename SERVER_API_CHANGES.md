# WebSocket Server API Changes Required

## Issue: Update `start` Message to Support Microsecond Timestamps

### Summary

The ESP32 client devices have been updated to send and receive microsecond-precision NTP timestamps in `start` messages to enable network delay compensation and achieve ±2-4ms synchronization accuracy across all devices.

The WebSocket server needs to be updated to handle the new timestamp format.

---

## Current Behavior

**Starter device sends:**
```json
{
  "type": "start",
  "event": "100m Free",
  "heat": "3",
  "timestamp": 1234567890
}
```

**Lane devices receive:**
```json
{
  "type": "start",
  "timestamp": 1234567890
}
```

**Problem:** 
- Only 1-second resolution (Unix epoch)
- Lane devices start timer when message arrives (5-50ms network delay)
- No network delay compensation possible
- Poor synchronization (±5-50ms between starter and lanes)

---

## Required Changes

### 1. Accept New Timestamp Fields in `start` Message

**Starter device now sends:**
```json
{
  "type": "start",
  "event": "100m Free",
  "heat": "3",
  "timestamp_sec": 1234567890,
  "timestamp_usec": 123456
}
```

**Fields:**
- `timestamp_sec` (int64): Unix epoch seconds from `gettimeofday()`
- `timestamp_usec` (int64): Microseconds component (0-999999)

### 2. Broadcast to Lane Devices

**Lane devices must receive:**
```json
{
  "type": "start",
  "event": "100m Free",
  "heat": "3",
  "timestamp_sec": 1234567890,
  "timestamp_usec": 123456
}
```

The server should:
1. Parse `timestamp_sec` and `timestamp_usec` from the starter's message
2. Include both fields when broadcasting to lane devices
3. Maintain `event` and `heat` fields in the broadcast

### 3. Backward Compatibility (Optional but Recommended)

For older client versions that may still send `timestamp`:

```javascript
// Pseudo-code for server handling
if (message.timestamp_sec && message.timestamp_usec) {
    // New format - forward as-is
    broadcast({
        type: "start",
        event: message.event,
        heat: message.heat,
        timestamp_sec: message.timestamp_sec,
        timestamp_usec: message.timestamp_usec
    });
} else if (message.timestamp) {
    // Legacy format - convert to new format
    broadcast({
        type: "start",
        event: message.event,
        heat: message.heat,
        timestamp_sec: message.timestamp,
        timestamp_usec: 0
    });
}
```

---

## Client Behavior (For Reference)

### Starter Device (ESP32)
```cpp
void WebSocketStopwatch::sendStart(const String& event, const String& heat) {
    struct timeval tv;
    gettimeofday(&tv, nullptr);  // NTP-synced time

    StaticJsonDocument<256> doc;
    doc["type"] = "start";
    doc["event"] = event;
    doc["heat"] = heat;
    doc["timestamp_sec"]  = (int64_t)tv.tv_sec;
    doc["timestamp_usec"] = (int64_t)tv.tv_usec;
    
    sendMessage(doc);
}
```

### Lane Device (ESP32)
```cpp
void WebSocketStopwatch::handleRemoteStart(int64_t timestampSec, int64_t timestampUsec) {
    if (timestampSec > 0) {
        // Calculate network delay
        struct timeval now;
        gettimeofday(&now, nullptr);
        int64_t delayUs = ((int64_t)now.tv_sec  - timestampSec) * 1000000LL
                        + ((int64_t)now.tv_usec - timestampUsec);
        
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

✅ **Synchronization accuracy:** ±2-4ms (from ±5-50ms)  
✅ **Professional-grade timing:** Meets FINA requirements (±10ms)  
✅ **Network-independent:** Compensates for variable WiFi latency  
✅ **NTP-based:** Uses shared time server for clock agreement  

---

## Testing

After implementing changes:

1. **Verify timestamp propagation:**
   - Starter sends start with `timestamp_sec` and `timestamp_usec`
   - All lane devices receive both fields
   - Verify microsecond precision (6 digits, 0-999999 range)

2. **Check synchronization:**
   - Start race with 1 starter + 8 lane devices
   - All devices should show identical elapsed time within ±4ms
   - Compare displays visually or via serial debug output

3. **Backward compatibility (if implemented):**
   - Test with old client sending `timestamp` (seconds only)
   - Server should convert to `timestamp_sec` + `timestamp_usec: 0`
   - New clients should still function (fallback to `start()` without offset)

---

## Related Client Changes

**Repository:** LilyGO T-Display S3 SwimWatch firmware  
**Files modified:**
- `src/websocket_stopwatch.cpp` — Updated `sendStart()` and `handleRemoteStart()`
- `include/websocket_stopwatch.h` — Changed `handleRemoteStart()` signature
- `src/stopwatch_timer.cpp` — Added `startWithOffset(int64_t offsetUs)` method
- `include/stopwatch_timer.h` — Added `startWithOffset()` declaration

**Commit:** Network delay compensation via microsecond NTP timestamps

---

## Priority

**High** — Current client firmware expects the new format and will not achieve accurate synchronization without server-side support.

---

## Contact

For questions or clarification, please reference the ESP32 client implementation in the SwimWatch firmware repository.
