# Display Layout Implementation - T-Display S3 Stopwatch

## Overview
The display has been updated to match the specified layout with the stopwatch and laps on the left side, and WiFi, WebSocket, lane, and battery status on the right side.

## Layout Structure

```
┌─────────────────────────────────┬─────────────────┐
│                                 │      WiFi       │
│           Stopwatch             │    Connected    │
│            02:03:4              │                 │
│                                 ├─────────────────┤
│                                 │   WebSocket     │
├─────────────────────────────────┤    Connected    │
│ Lap 1: 00:45:23                │                 │
├─────────────────────────────────┼─────────────────┤
│ Lap 2: 01:32:11                │                 │
├─────────────────────────────────┤      Lane       │
│ Lap 3: 02:03:44                │        9        │
└─────────────────────────────────┼─────────────────┤
                                  │    Battery      │
                                  │      75%        │
                                  └─────────────────┘
```

## Display Dimensions
- **Total Display**: 320x170 pixels
- **Left Area (Main)**: 240x170 pixels (0-240px width)
- **Right Area (Status)**: 80x170 pixels (240-320px width)

## Area Definitions

### Left Side Areas
- **Stopwatch Area**: 240x80 pixels (0,0 to 240,80)
- **Lap 1 Area**: 240x30 pixels (0,80 to 240,110)
- **Lap 2 Area**: 240x30 pixels (0,110 to 240,140)
- **Lap 3 Area**: 240x30 pixels (0,140 to 240,170)

### Right Side Areas
- **WiFi Status**: 80x40 pixels (240,0 to 320,40)
- **WebSocket Status**: 80x40 pixels (240,40 to 320,80)
- **Lane Info**: 80x45 pixels (240,80 to 320,125)
- **Battery Status**: 80x45 pixels (240,125 to 320,170)

## Functions for Each Area

### Main Stopwatch Display
```cpp
display.updateStopwatchDisplay(elapsedMs, isRunning);
display.showStartupMessage("Connecting...");
display.clearStartupMessage();
```

### Lap Times Display
```cpp
display.updateLapTime(1, "00:45:23");  // Lap 1
display.updateLapTime(2, "01:32:11");  // Lap 2
display.updateLapTime(3, "02:03:44");  // Lap 3
display.clearLapTimes();
```

### Status Areas
```cpp
display.updateWiFiStatus("Connected", true);
display.updateWebSocketStatus("Connected", true);
display.updateLaneInfo(9);
display.updateBatteryDisplay(3.8f, 75);
```

### Layout Management
```cpp
display.drawBorders();              // Draw area separators
display.clearStatusAreas();         // Clear all status areas
display.forceRefresh();            // Mark all areas for redraw
```

## Features

### Startup Messages
The stopwatch area can show startup messages during system initialization:
- "Connecting to WiFi..."
- "WiFi Connected"
- "Synchronizing time..."
- "Connecting to server..."

### Status Display
- **WiFi**: Shows connection status (Connected/Disconnected)
- **WebSocket**: Shows server connection status
- **Lane**: Shows configured lane number
- **Battery**: Shows percentage and voltage (or USB power indicator)

### Lap Display
- Shows up to 3 lap times
- Format: "Lap X: mm:ss:ms"
- Automatically clears when stopwatch is reset

### Time Format
- **Running**: Shows 1 digit milliseconds (mm:ss:m)
- **Stopped**: Shows 2 digit milliseconds (mm:ss:ms)

## Color Coding
- **Green (COLOR_TIME_DISPLAY)**: Active/connected states, running time
- **Red (COLOR_ERROR)**: Error states, disconnected WiFi
- **Orange (COLOR_WARNING)**: Warning states, stopped time
- **Cyan (COLOR_STATUS)**: Normal status information
- **Yellow (COLOR_LAP_INFO)**: Lap time information

## Integration with Main Application
The main.cpp has been updated to properly use these new layout functions:

1. **Initialization**: Shows startup messages in stopwatch area
2. **Status Updates**: Updates all status areas every second
3. **Stopwatch**: Updates time display every 100ms
4. **Callbacks**: Lap times are displayed when added
5. **Battery**: Shows power status (battery or USB)

## Backward Compatibility
Legacy functions are still available but redirect to the new layout:
- `showGeneralStatus()` → uses WiFi status area
- `updateTimeDisplay()` → calls `updateStopwatchDisplay()`
- `showBatteryStatus()` → calls `updateBatteryDisplay()`

This ensures existing code continues to work while taking advantage of the new structured layout.
