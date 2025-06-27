# T-Display S3 Stopwatch - Refactored Version

## Overzicht

Dit is de gerefactoreerde versie van de LilyGO T-Display S3 stopwatch met een modulaire architectuur. De code is opgesplitst in verschillende modules voor betere onderhoudbaarheid en uitbreidbaarheid.

## Modules

### 1. ConnectivityManager (`connectivity.h/cpp`)
Beheert alle netwerkgerelateerde functionaliteiten:
- **WiFi Management**: Automatische verbinding met opgeslagen netwerken
- **WiFiManager**: Configuratieportaal voor nieuwe netwerken (192.168.4.1)
- **NTP Synchronisatie**: Tijdsynchronisatie met NTP servers
- **Configuratie Opslag**: Persistente opslag van instellingen in LittleFS

**Functies:**
- `initWiFi()`: Initialiseer WiFi verbinding met automatische configuratie
- `initNTP()`: Start NTP tijdsynchronisatie
- `saveConfig()` / `loadConfig()`: Configuratie beheer
- `isWiFiConnected()`: Status controle

### 2. DisplayManager (`display_manager.h/cpp`)
Beheert alle display functionaliteiten:
- **Scherm Gebieden**: Tijd, status, en lap informatie
- **Kleur Management**: Verschillende kleuren voor verschillende states
- **Text Formatting**: Tijd formatting en layout
- **Status Meldingen**: WiFi, WebSocket, en systeem status

**Functies:**
- `updateTimeDisplay()`: Update de tijd weergave
- `showWiFiStatus()` / `showWebSocketStatus()`: Status meldingen
- `showConfigPortalInfo()`: Configuratie portal informatie
- `updateLapDisplay()`: Lap tijden weergave

### 3. ButtonManager (`button_manager.h/cpp`)
Beheert hardware button interrupts:
- **Hardware Interrupts**: Efficiënte button detectie
- **Software Debounce**: Ruis filtering
- **Multiple Buttons**: Start/Lap (GPIO0), Stop (GPIO14), Start2 (GPIO2)

**Button Configuratie:**
- **GPIO0**: Start/Lap button (active LOW, internal pullup)
- **GPIO14**: Stop button (active LOW, internal pullup)  
- **GPIO2**: Extra start button (active HIGH, **externe pulldown vereist**)

**Functies:**
- `getButtonEvent()`: Haal button events op
- `isButtonPressed()`: Directe button status

### 4. WebSocketStopwatch (`websocket_stopwatch.h/cpp`)
Beheert stopwatch logica en WebSocket communicatie:
- **Stopwatch Logic**: Start, stop, reset, lap functionaliteit
- **WebSocket Client**: SSL verbinding met server
- **Tijd Synchronisatie**: Server tijd synchronisatie
- **Message Handling**: JSON message parsing en sending

**WebSocket Messages:**
- **Ontvangen:**
  - `{"type":"start","time":timestamp}`: Start met server tijd
  - `{"type":"reset"}`: Reset stopwatch
  - `{"type":"time_sync","server_time":timestamp}`: Tijd synchronisatie
- **Verzenden:**
  - `{"type":"split","lane":"X","time-ms":timestamp,"time":"MM:SS:MS"}`: Split tijd

## Configuratie

### WiFi Setup
Bij eerste gebruik:
1. Apparaat start configuratie Access Point: `T-Display-S3-Config`
2. Wachtwoord: `stopwatch123`
3. Verbind met dit netwerk en ga naar `192.168.4.1`
4. Configureer WiFi netwerk en WebSocket server instellingen

### WebSocket Server
Default configuratie:
- **Server**: `scherm.azckamp.nl`
- **Port**: `443` (SSL)
- **Path**: `/ws`
- **Lane**: `9`

### Opgeslagen Configuratie
Configuratie wordt opgeslagen in LittleFS:
- `ws_server`: WebSocket server adres
- `ws_port`: WebSocket poort
- `lane`: Lane nummer voor dit apparaat

## Hardware Setup

### Buttons
```
BUTTON1 (GPIO0)  : Start/Lap - Internal pullup, active LOW
BUTTON2 (GPIO14) : Stop      - Internal pullup, active LOW  
BUTTON3 (GPIO2)  : Start     - External pulldown VEREIST, active HIGH
```

**BELANGRIJK voor GPIO2**: 
- Sluit button aan tussen 3.3V en GPIO2
- Plaats 1kΩ weerstand tussen GPIO2 en GND (externe pulldown)

### Display
- **Type**: ST7789V IPS LCD
- **Resolutie**: 170x320 pixels
- **Interface**: 8-bit parallel via TFT_eSPI

## Startup Flow

1. **Display Initialisatie**: Splash screen en basis setup
2. **Button Setup**: Hardware interrupt configuratie  
3. **WiFi Verbinding**: 
   - Automatische verbinding met opgeslagen netwerk
   - Bij falen: Start configuratie portal
4. **NTP Synchronisatie**: Tijd synchronisatie (fallback)
5. **WebSocket Verbinding**: Verbinding met server
6. **Tijd Synchronisatie**: Nauwkeurige server tijd synchronisatie

## Gebruik

### Normale Werking
- **Start**: Druk BUTTON1 of BUTTON3
- **Lap**: Druk BUTTON1 tijdens het lopen
- **Stop**: Druk BUTTON2
- **Reset**: Druk BUTTON2 wanneer gestopt

### Remote Control (via WebSocket)
- Server kan stopwatch remote starten
- Server kan stopwatch remote resetten
- Split tijden worden automatisch naar server gestuurd

## Development

### Build Requirements
- **Platform**: ESP32-S3
- **Framework**: Arduino
- **Libraries**:
  - `links2004/WebSockets`
  - `bblanchon/ArduinoJson`
  - `tzapu/WiFiManager`
  - `arduino-libraries/NTPClient`
  - `lorol/LittleFS_esp32`

### Project Structure
```
include/
├── connectivity.h          # WiFi, NTP, configuratie
├── display_manager.h       # Display management
├── button_manager.h        # Button interrupts
└── websocket_stopwatch.h   # WebSocket & stopwatch logic

src/
├── main.cpp               # Hoofdapplicatie
├── connectivity.cpp       # WiFi implementatie
├── display_manager.cpp    # Display implementatie
├── button_manager.cpp     # Button implementatie
└── websocket_stopwatch.cpp # WebSocket implementatie
```

### Compile en Upload
```bash
pio run --target upload
pio device monitor
```

## Troubleshooting

### WiFi Verbinding Problemen
1. Reset configuratie: Houd BUTTON1+BUTTON2 ingedrukt tijdens opstarten
2. Configuratie portal verschijnt automatisch bij verbindingsproblemen
3. Check serial monitor voor debug informatie

### WebSocket Verbinding Problemen  
1. Controleer server adres en poort in configuratie
2. Controleer internet verbinding
3. Server status controleren

### Button Problemen
1. **GPIO2**: Controleer externe pulldown weerstand (1kΩ naar GND)
2. **GPIO0/14**: Internal pullups zijn automatisch geactiveerd
3. Check serial monitor voor button events

### Display Problemen
1. TFT_eSPI library configuratie controleren
2. Hardware verbindingen controleren
3. Voeding (3.3V) controleren

## Monitoring

Serial Monitor output toont:
- Opstartprocess
- WiFi verbindingsstatus  
- WebSocket berichten
- Button events
- Stopwatch state changes
- Error messages

Baud rate: `115200`
