# Copilot Instructions

## Project Context

- **Hardware:** LilyGO T-Display S3 (ESP32-S3 with 1.9" ST7789V IPS TFT display)
- **MCU:** ESP32-S3R8 Dual-core LX7 microprocessor
- **Wireless Connectivity:** Wi-Fi 802.11, BLE 5 + BT mesh
- **Programming Platform:** Arduino IDE, Micropython
- **Flash:** 16MB
- **PSRAM:** 8MB
- **Display:** 1.9" 170(H)RGB x320(V) ST7789V IPS LCD, 8-Bit Parallel Interface
- **Working Power Supply:** 3.3V
- **Onboard Buttons:**
  - **BUTTON1:** GPIO0
  - **BUTTON2:** GPIO14
- **Other Pins:**
  - **TFT_MOSI:** GPIO19
  - **TFT_SCLK:** GPIO18
  - **TFT_CS:** GPIO5
  - **TFT_DC:** GPIO16
  - **TFT_BL:** GPIO4
  - **I2C_SDA:** GPIO21
  - **I2C_SCL:** GPIO22
  - **ADC_IN:** GPIO34
  - **ADC Power:** GPIO4 <not sure if this is correct>

## App
- **Functionality:** Stopwatch
- **Buttons:**
  - **BUTTON1 (GPIO35):** Start/Stop
  - **BUTTON2 (GPIO14):** Reset

- **Display:** Show elapsed time in minutes:seconds:miliseconds (mm:ss:ms)
- **Features:**
  - Start/Stop functionality using BUTTON1
  - Reset functionality using BUTTON2
  - Display time in mm:ss:ms format on the TFT display
    - the miliseconds only 1 digit
  - use hardware interrupts for button presses
  - Use internal RTC for timing
  - When start save the current RTC value
  - Refresh display every 100ms
  - When stopped, show full time with 2 digits for milliseconds

## Style Rules

- Use descriptive variable and function names.
- Keep functions short and focused on a single task.
- Use comments to explain hardware-specific logic and pin assignments.
- Group related code into functions or classes.
- Prefer `constexpr` or `#define` for pin numbers and constants.
- Use `setup()` for initialization and `loop()` for main logic.
- Debounce button inputs in software.
- Update the display only when the state changes.
- Avoid blocking code in `loop()`.
- Use `Serial.print` for debugging if needed, but disable in production.
- Document any hardware dependencies or wiring in comments at the top of the file.


# Resources
- https://lilygo.cc/products/t-display-s3
- https://github.com/Xinyuan-LilyGO/T-Display-S3
- https://gist.github.com/buzzkillb/2b6381632f73ee965e1c9329163fdfd3
