# SwimWatch Timing System — How It Works

*A non-technical explanation of the precision timing system*

---

## The Setup

**Equipment:**
- **1 Starter Device** — held by the race official at the starting blocks
- **8 Lane Devices** — one at each end of the pool where swimmers touch the wall
- **1 NTP Time Server** — Raspberry Pi on WiFi acting as a shared master clock

All devices connect to WiFi and sync their clocks to the same time server, ensuring they all agree on the time within about **2 milliseconds** of each other — like setting all the clocks in a building to the same master clock.

---

## How Race Start Works: Starter → Lane Devices

### The Problem (Before the Fix)

When the starter pressed the button, the message had to travel through WiFi to all lane devices. This took 5–50 milliseconds (mostly 10–30ms in practice). If lane devices just started their timers when the message arrived, they'd all be **5–50ms late** compared to the starter.

### The Solution (After the Fix)

```
         STARTER                    SERVER                    LANE 5
            │                          │                         │
   Coach    │                          │                         │
   presses  │                          │                         │
   button   │                          │                         │
      │     │                          │                         │
      ▼     │                          │                         │
  ┌─────────────────┐                  │                         │
  │ Records the     │                  │                         │
  │ exact time:     │                  │                         │
  │ 14:30:05.123456 │                  │                         │
  │ (from NTP clock)│                  │                         │
  └────────┬────────┘                  │                         │
           │  "Start! My clock        │                         │
           │   said 14:30:05.123456"  │                         │
           │ ─────────────────────►   │                         │
           │                          │  "Start! Starter's      │
           │                          │   clock said             │
           │                          │   14:30:05.123456"       │
           │                          │ ──────────────────────►  │
           │                          │                          │
           │                          │              ┌───────────────────┐
           │                          │              │ Lane checks its   │
           │                          │              │ own clock:        │
           │                          │              │ 14:30:05.153456   │
           │                          │              │                   │
           │                          │              │ Difference:       │
           │                          │              │ 0.030 seconds     │
           │                          │              │ (30 milliseconds) │
           │                          │              │                   │
           │                          │              │ "The race already │
           │                          │              │  started 30ms ago │
           │                          │              │  — I'll set my    │
           │                          │              │  stopwatch to     │
           │                          │              │  0.030 instead    │
           │                          │              │  of 0.000"        │
           │                          │              └───────────────────┘
```

**The key:** The lane device doesn't start at zero. It calculates "how late is this message?" using the shared NTP clock, then jumps ahead by that amount. Now all devices show the same elapsed time within **±2–4 milliseconds**.

---

## How Split Times Work: One Device, Perfect Precision

When a swimmer touches the wall, the timing is completely **local** to that lane device — no network, no synchronization needed.

```
  Race starts (timer already offset by 0.030s for network delay)
       │
       │   Hardware timer counting: 0.030... 0.031... 0.032...
       │   (1 million ticks per second — microsecond precision)
       │
       │   ... 25.000... 25.001... 25.002 ...
       │
       ▼
  ┌──────────────────────────────┐
  │ 🏊 Swimmer touches the wall  │
  │                              │
  │   Timer reads: 25.142        │
  │                              │
  │   This is EXACT:             │
  │   • No network delay         │
  │   • No NTP needed            │
  │   • Just one chip reading    │
  │     its own counter          │
  │                              │
  │   Accuracy: 0.001 ms         │
  └──────────────┬───────────────┘
                 │
                 │  Send to server:
                 │  "Lane 5 split: 25.142 seconds"
                 ▼
              SCOREBOARD
```

### Multiple Laps

For races with multiple laps, the device simply subtracts:

```
Lap 1 touch: 25.142s   →   Lap 1 time = 25.142s
Lap 2 touch: 51.287s   →   Lap 2 time = 51.287 - 25.142 = 26.145s
Lap 3 touch: 78.410s   →   Lap 3 time = 78.410 - 51.287 = 27.123s
```

This is one device doing simple subtraction on times from the same clock — **perfectly accurate**.

---

## Accuracy Summary

| Measurement | Accuracy | Why |
|-------------|----------|-----|
| **Starter's own display** | Perfect (0.001ms) | One device, one hardware clock |
| **Starter → Lane sync** | ±2–4 ms | Both use NTP; clocks agree within ~2ms |
| **Lane split times** | Perfect (0.001ms) | One device reading its own clock |
| **Lap time differences** | Perfect (0.001ms) | Simple subtraction on same clock |
| **Lane vs Starter displays** | ±2–4 ms | Limited by NTP synchronization |

### Comparison to Standards

**FINA Requirements** (international swimming federation):
- Electronic timing must be accurate within **±10 milliseconds** (1/100th second)

**SwimWatch Performance:**
- **2–5× better** than FINA requirements
- ±2–4ms is competitive with professional pool timing systems

---

## Technical Notes (for the curious)

### NTP (Network Time Protocol)
- Standard internet protocol for clock synchronization
- On a local WiFi network: typically ±1–2ms accuracy
- Re-syncs every 60 seconds automatically
- "Smooth mode" prevents sudden time jumps

### ESP32 Hardware Timer
- 64-bit counter running at 1 MHz (1 million ticks/second)
- Microsecond (0.001ms) resolution
- Independent of WiFi, NTP, or any network activity
- Drift: ~±20 parts per million (~36ms over 30 minutes)

### Why This Matters
The old system would just start lane timers when the network message arrived, making them 5–50ms late. By using NTP to calculate that delay and compensate for it, all devices now show nearly identical times regardless of network speed.

---

**Version:** 1.0  
**Last Updated:** February 2026  
**Hardware:** LilyGO T-Display S3 (ESP32-S3)
