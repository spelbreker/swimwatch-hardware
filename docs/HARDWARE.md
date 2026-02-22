# Hardware Specifications — SwimWatch

## LilyGO T-Display S3

| Component | Specification |
|-----------|---------------|
| **MCU** | ESP32-S3R8 Dual-core LX7 @ 240 MHz |
| **Flash** | 16 MB |
| **PSRAM** | 8 MB |
| **Display** | 1.9" ST7789V IPS LCD, 170x320, 8-bit parallel |
| **Colour depth** | 16-bit RGB565 (65K colours) |
| **USB** | USB-C (programming + power) |
| **Battery** | 3.7 V Li-Po via JST connector, built-in charger |
| **Working voltage** | 3.3 V |

---

## Pin Assignments

### Buttons

| GPIO | Function | Type | Notes |
|------|----------|------|-------|
| 0 | Start / Stop | Onboard BUTTON1 | Active LOW, internal pullup |
| 14 | Split (running) / Reset (stopped) | Onboard BUTTON2 | Active LOW, internal pullup |
| 2 | Split trigger | External | Active HIGH, internal pulldown — button connects GPIO2 to 3.3V |

### Display (managed by TFT_eSPI `User_Setup.h`)

| Pin | GPIO |
|-----|------|
| TFT_MOSI | 19 |
| TFT_SCLK | 18 |
| TFT_CS | 5 |
| TFT_DC | 16 |
| TFT_RST | 23 |
| TFT_BL | 38 |

### Power / Battery

| GPIO | Function | Notes |
|------|----------|-------|
| 15 | PIN_POWER_ON | Must be held HIGH for battery operation |
| 4 | Battery ADC | 1:2 resistive voltage divider, 12-bit ADC |

### Battery Monitoring

```cpp
constexpr uint8_t  PIN_BATTERY_ADC     = 4;
constexpr float    BATTERY_MAX_VOLTAGE  = 4.2f;   // Full LiPo
constexpr float    BATTERY_MIN_VOLTAGE  = 3.0f;   // Empty LiPo
constexpr uint8_t  BATTERY_SAMPLES      = 16;     // ADC averaging
```

Voltage calculation:

```cpp
uint32_t rawSum = 0;
for (int i = 0; i < BATTERY_SAMPLES; i++) rawSum += analogRead(PIN_BATTERY_ADC);
float voltage = (rawSum / BATTERY_SAMPLES) * 2.0f * 3.3f / 4095.0f;
uint8_t pct = constrain((voltage - BATTERY_MIN_VOLTAGE) /
              (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE) * 100, 0, 100);
```

---

## Button Wiring

```
GPIO0 (onboard, internal pullup):
    3.3V ---[PULLUP]--- GPIO0 ---[BUTTON]--- GND

GPIO14 (onboard, internal pullup, dual-use):
    3.3V ---[PULLUP]--- GPIO14 ---[BUTTON]--- GND
    - While running : records a split time
    - While stopped : resets the stopwatch

GPIO2 (external split trigger, active HIGH, internal pulldown):
    GND ---[PULLDOWN]--- GPIO2 ---[BUTTON]--- 3.3V
    No external resistor required.
```

---

## Connectivity

### Wi-Fi

| Property | Value |
|----------|-------|
| Standards | IEEE 802.11 b/g/n |
| Frequency | 2.4 GHz |
| Modes | Station, SoftAP (captive portal) |
| Security | WPA2-PSK |

### Bluetooth

| Property | Value |
|----------|-------|
| Standard | Bluetooth 5.0 + BLE |
| Status | Not used by SwimWatch |

---

## Power

### Consumption (approximate)

| Mode | Current @ 3.3 V |
|------|-----------------|
| Active (WiFi + Display) | ~150 mA |
| WiFi TX peak | ~200 mA |
| Display off | ~50 mA |
| Deep sleep | ~5 mA |

### Supply Options

1. **USB-C** — 5 V input, internal LDO to 3.3 V
2. **Li-Po battery** — 3.7 V via JST, built-in charge IC
3. **GPIO VIN** — 3.3 V direct (advanced)

---

## Display Configuration

TFT_eSPI `User_Setup.h` must contain:

```cpp
#define ST7789_DRIVER
#define TFT_WIDTH  170
#define TFT_HEIGHT 320
#define TFT_RGB_ORDER TFT_BGR

// 8-bit parallel pins for T-Display S3
#define TFT_CS   5
#define TFT_DC   16
#define TFT_RST  23
#define TFT_WR   18
#define TFT_RD   -1

#define TFT_D0   39
#define TFT_D1   40
#define TFT_D2   41
#define TFT_D3   42
#define TFT_D4   45
#define TFT_D5   46
#define TFT_D6   47
#define TFT_D7   48

#define TFT_BL   38
#define TFT_BACKLIGHT_ON HIGH
```

---

## Physical Dimensions

| Property | Value |
|----------|-------|
| Length | ~60 mm |
| Width | ~25 mm |
| Thickness | ~12 mm (with display) |
| Weight | ~15 g |
| Mounting | 4x M3 holes, breadboard compatible |

---

## Environmental

| Condition | Range |
|-----------|-------|
| Operating temp | -40 C to +85 C |
| Storage temp | -40 C to +125 C |
| Humidity | 5-95% RH (non-condensing) |

---

## Safety Notes

- Use ESD precautions when handling the board
- Do not exceed 3.6 V on GPIO pins
- Use certified USB-C cables
- Ensure GPIO15 (PIN_POWER_ON) is HIGH when running on battery

---

## Resources

- [LilyGO T-Display S3 product page](https://lilygo.cc/products/t-display-s3)
- [LilyGO GitHub repository](https://github.com/Xinyuan-LilyGO/T-Display-S3)
- [ESP32-S3 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- [ST7789V datasheet](https://www.newhavendisplay.com/appnotes/datasheets/LCDs/ST7789V.pdf)
