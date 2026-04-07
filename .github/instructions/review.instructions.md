---
applyTo: "**/*.{cpp,h,ino}"
---

# Code Review Guidelines — SwimWatch

## Review Philosophy

- Only comment when you have **high confidence (>80%)** that an issue exists
- Be concise: one sentence per comment when possible
- Focus on actionable feedback, not observations
- When uncertain, stay silent — false positives create noise

## Priority Areas

### Timing Correctness

- `millis()` or `micros()` used for stopwatch/split timing — must use `esp_timer_get_time()`
- `millis()` used for wall-clock timestamps — must use `time()` / `getLocalTime()`
- Missing NTP sync consideration for absolute timestamps

### Hardware Safety

- Blocking calls in `loop()` — `delay()`, `while(true)`, synchronous network calls
- Hardcoded pin numbers outside `include/config.h`
- ISR handlers missing `IRAM_ATTR` or doing heavy work (allocations, Serial, etc.)
- Missing `volatile` on ISR-shared variables
- Incorrect GPIO mode (pullup vs pulldown) for the specific pin

### Memory & Stability

- Heap allocations in hot paths (loop, ISR, display update)
- Unbounded `String` concatenation in loops
- `StaticJsonDocument` sized too small for the payload
- Missing null/bounds checks on external data (WebSocket messages, NVS reads)

### Project Pattern Violations

- **Raw magic numbers** — use `constexpr` in `config.h`
- **Raw `Serial.printf()` for debug** — use `DEBUG_LOG()` macro
- **Raw type strings in WebSocket** — use `WS_MSG_*` constants
- **Full-screen redraws** — use dirty-region tracking
- **New constants in source files** — must go in `include/config.h`
- **Missing docblock** on new `.h` files or public methods

### Security

- Hardcoded credentials or API keys (beyond the AP defaults in config.h)
- WebSocket messages processed without type validation
- Unvalidated external input used in array indexing or buffer operations

## CI Pipeline Context

No CI pipeline configured. All validation is manual via `pio run` build.

## Skip These

Do not comment on:

- **Formatting/whitespace** — no automated formatter configured
- **Minor naming suggestions** — unless truly confusing or violates project conventions
- **Adding comments** — for self-documenting code
- **Refactoring** — unless there's a clear bug or pattern violation
- **Arduino framework style** — don't suggest STL alternatives for Arduino idioms
- **TFT_eSPI library code** — vendored, do not review `lib/TFT_eSPI/`

## Response Format

1. **State the problem** (1 sentence)
2. **Why it matters** (1 sentence, only if not obvious)
3. **Suggested fix** (code snippet or specific action)

## When to Stay Silent

- Code works correctly and follows project patterns
- Suggestion is a "nice to have" refactor, not a bug or pattern violation
- The pattern matches what sibling files already do
