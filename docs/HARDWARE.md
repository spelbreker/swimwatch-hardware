# Hardware Specifications - T-Display S3 Stopwatch

## 🖥️ LilyGO T-Display S3 Overview

The LilyGO T-Display S3 is a development board based on the ESP32-S3 microcontroller with an integrated TFT display, specifically designed for IoT applications requiring visual feedback.

### Core Specifications

| Component | Specification |
|-----------|--------------|
| **Microcontroller** | ESP32-S3R8 Dual-core LX7 @ 240MHz |
| **Flash Memory** | 16MB |
| **PSRAM** | 8MB |
| **Display** | 1.9" ST7789V IPS LCD |
| **Resolution** | 170(H) × 320(V) pixels |
| **Display Interface** | 8-bit parallel |
| **Operating Voltage** | 3.3V |
| **USB** | USB-C (programming/power) |

## 📺 Display Specifications

### ST7789V IPS LCD Details
```
Physical Size: 1.9 inches diagonal
Resolution: 170 × 320 pixels
Color Depth: 16-bit (65K colors)
Interface: 8-bit parallel
Viewing Angle: 170° (H/V)
Backlight: LED backlight with PWM control
```

### Display Pin Mapping
```cpp
// TFT Display Pins (handled by TFT_eSPI library)
#define TFT_MOSI     19    // Data/Command
#define TFT_SCLK     18    // Clock
#define TFT_CS       5     // Chip Select
#define TFT_DC       16    // Data/Command
#define TFT_RST      23    // Reset (handled internally)
#define TFT_BL       4     // Backlight control
```

### Display Configuration
The display requires specific TFT_eSPI configuration:
```cpp
// User_Setup.h configuration
#define ST7789_DRIVER
#define TFT_WIDTH  170
#define TFT_HEIGHT 320
#define TFT_RGB_ORDER TFT_BGR
```

## 🔘 GPIO and Button Configuration

### Available GPIO Pins
```
Digital I/O Pins: GPIO0-48 (some restrictions apply)
Analog Input: GPIO1-20 (ADC1), GPIO1-10 (ADC2)
PWM Channels: 16 channels
I2C: 2 channels (any GPIO)
SPI: 3 channels (HSPI, VSPI, FSPI)
UART: 3 channels
```

### Button Hardware Setup

#### GPIO0 - Start/Lap Button
```
Configuration: Internal pullup enabled
Trigger: Active LOW (pressed = 0V)
Function: Start stopwatch / Create lap time
Wiring: Button between GPIO0 and GND
```

#### GPIO14 - Stop/Reset Button  
```
Configuration: Internal pullup enabled
Trigger: Active LOW (pressed = 0V)
Function: Stop running / Reset when stopped
Wiring: Button between GPIO14 and GND
```

#### GPIO2 - Split Timer Button
```
Configuration: External pulldown REQUIRED
Trigger: Active HIGH (pressed = 3.3V)
Function: Create split time
Wiring: Button between 3.3V and GPIO2
        1kΩ resistor between GPIO2 and GND
```

⚠️ **Critical Note for GPIO2**: This pin requires an external pulldown resistor. Without it, the pin will float and cause erratic behavior.

### Button Wiring Diagram
```
GPIO0 Button (Internal Pullup):
    3.3V ----[PULLUP]---- GPIO0 ----[BUTTON]---- GND

GPIO14 Button (Internal Pullup):
    3.3V ----[PULLUP]---- GPIO14 ----[BUTTON]---- GND

GPIO2 Button (External Pulldown Required):
    3.3V ----[BUTTON]---- GPIO2 ----[1kΩ]---- GND
```

## 🔌 Connectivity Specifications

### Wi-Fi Capabilities
```
Standards: IEEE 802.11 b/g/n
Frequency: 2.4 GHz
Modes: Station, SoftAP, Station+SoftAP
Security: WPA2-PSK, WEP, Open
Maximum Range: ~100m (line of sight)
```

### Bluetooth/BLE
```
Standard: Bluetooth 5.0
Protocols: Classic Bluetooth, BLE
Profiles: HID, A2DP, GATT
Range: ~10m (Class 2)
```

## ⚡ Power Specifications

### Power Supply Options
1. **USB-C**: 5V input, regulated to 3.3V
2. **Battery**: 3.7V Li-Po via JST connector
3. **GPIO VIN**: 3.3V direct input

### Power Consumption
```
Active Mode (WiFi + Display): ~150mA @ 3.3V
Sleep Mode (Deep Sleep): ~5mA @ 3.3V
Display Off: ~50mA @ 3.3V
WiFi Transmit Peak: ~200mA @ 3.3V
```

### Battery Management
- **Charging**: Built-in charging circuit for Li-Po
- **ADC Monitoring**: GPIO34 for battery voltage
- **Low Battery**: Configurable threshold detection

## 🧮 Memory Layout

### Flash Memory (16MB)
```
Bootloader: 0x1000 - 0x8000 (28KB)
Partition Table: 0x8000 - 0x9000 (4KB)
NVS: 0x9000 - 0xE000 (20KB)
App0: 0x10000 - 0x390000 (~3.5MB)
App1: 0x390000 - 0x710000 (~3.5MB) [OTA]
SPIFFS/LittleFS: 0x710000 - 0xFF0000 (~9MB)
```

### PSRAM (8MB)
```
Heap Extension: Available for dynamic allocation
DMA Buffers: Display and networking buffers
Large Arrays: Image/font storage
```

## 🔧 Peripheral Integration

### I2C Configuration
```cpp
// Default I2C pins
#define I2C_SDA      21
#define I2C_SCL      22

// Alternative pins available
Wire.begin(SDA_PIN, SCL_PIN);
```

### SPI Configuration
```cpp
// Default SPI pins (VSPI)
#define SPI_MOSI     23
#define SPI_MISO     19
#define SPI_SCLK     18
#define SPI_CS       5
```

### ADC Configuration
```cpp
// Battery monitoring
#define ADC_PIN      34
#define ADC_VREF     3300    // mV
#define ADC_RESOLUTION 4095  // 12-bit

// Voltage divider calculation
float batteryVoltage = (analogRead(ADC_PIN) * ADC_VREF * 2) / ADC_RESOLUTION;
```

## 🌡️ Environmental Specifications

### Operating Conditions
```
Temperature: -40°C to +85°C
Humidity: 5% to 95% RH (non-condensing)
Altitude: Up to 2000m
```

### Storage Conditions
```
Temperature: -40°C to +125°C
Humidity: 5% to 95% RH (non-condensing)
```

## 📐 Physical Dimensions

### Board Dimensions
```
Length: 60mm
Width: 25mm
Thickness: 12mm (including display)
Weight: ~15g
```

### Mounting
- **Holes**: 4 × M3 mounting holes
- **Spacing**: Standard breadboard compatible
- **Connector**: USB-C centered on bottom edge

## 🔌 Connector Pinout

### Header Pins (Left Side)
```
Pin 1:  3.3V
Pin 2:  GND
Pin 3:  GPIO43
Pin 4:  GPIO44
Pin 5:  GPIO1
Pin 6:  GPIO2
Pin 7:  GPIO42
Pin 8:  GPIO41
Pin 9:  GPIO40
Pin 10: GPIO39
```

### Header Pins (Right Side)
```
Pin 1:  5V (USB)
Pin 2:  GND
Pin 3:  GPIO38
Pin 4:  GPIO37
Pin 5:  GPIO36
Pin 6:  GPIO35
Pin 7:  GPIO0
Pin 8:  GPIO45
Pin 9:  GPIO48
Pin 10: GPIO47
```

## 🛡️ Protection Features

### Built-in Protection
- **Overcurrent Protection**: USB and battery circuits
- **ESD Protection**: All GPIO pins
- **Reverse Polarity**: Battery connector protection
- **Thermal Protection**: Automatic throttling

### Recommended External Protection
- **TVS Diodes**: For antenna connections
- **Ferrite Beads**: For power supply filtering
- **Bypass Capacitors**: For noise reduction

## 🔗 Related Documentation

- [ESP32-S3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- [ST7789V Display Datasheet](docs/ST7789V_SPEC_V1.4.pdf)
- [LilyGO T-Display S3 Schematic](schematic/T_Display_S3.pdf)
- [Arduino ESP32 Core Documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/)

---

⚠️ **Important Safety Notes**:
- Always use proper ESD precautions when handling the board
- Do not exceed maximum voltage ratings
- Ensure proper heat dissipation in enclosed applications
- Use certified USB-C cables for programming and power
