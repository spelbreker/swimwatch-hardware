#include "captive_portal.h"

// HTML for the configuration page
const char CONFIG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <title>T-Display S3 Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f0f0f0; }
        .container { background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #333; text-align: center; }
        input, select { width: 100%; padding: 12px; margin: 8px 0; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }
        input[type="submit"] { background-color: #4CAF50; color: white; cursor: pointer; }
        input[type="submit"]:hover { background-color: #45a049; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
    </style>
</head>
<body>
    <div class="container">
        <h1>T-Display S3 Stopwatch Setup</h1>
        <form action="/config" method="POST">
            <div class="form-group">
                <label for="ssid">WiFi Network:</label>
                <input type="text" id="ssid" name="ssid" placeholder="Enter WiFi SSID" required>
            </div>
            
            <div class="form-group">
                <label for="password">WiFi Password:</label>
                <input type="password" id="password" name="password" placeholder="Enter WiFi Password">
            </div>
            
            <div class="form-group">
                <label for="server">WebSocket Server:</label>
                <input type="text" id="server" name="server" value="scherm.azckamp.nl" placeholder="Server address">
            </div>
            
            <div class="form-group">
                <label for="port">Server Port:</label>
                <input type="number" id="port" name="port" value="443" placeholder="443">
            </div>
            
            <div class="form-group">
                <label for="lane">Lane Number:</label>
                <input type="number" id="lane" name="lane" value="9" min="0" max="9" placeholder="Lane number">
            </div>
            
            <input type="submit" value="Save Configuration">
        </form>
    </div>
</body>
</html>)rawliteral";

const char SUCCESS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <title>Configuration Saved</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f0f0f0; text-align: center; }
        .container { background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #4CAF50; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Configuration Saved!</h1>
        <p>Your T-Display S3 will now restart and connect to the configured WiFi network.</p>
        <p>The device will be ready for stopwatch operation in a few seconds.</p>
    </div>
</body>
</html>)rawliteral";

CaptivePortalManager::CaptivePortalManager() : server(80), configComplete(false) {
}

CaptivePortalManager::~CaptivePortalManager() {
    stop();
}

bool CaptivePortalManager::begin() {
    Serial.println("Starting captive portal...");
    
    // Start WiFi in AP mode
    WiFi.mode(WIFI_AP);
    WiFi.softAP("T-Display-S3-Setup", "stopwatch123");
    
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    
    // Start DNS server for captive portal
    dnsServer.start(53, "*", WiFi.softAPIP());
    
    // Setup web server routes
    setupWebServer();
    
    // Start web server
    server.begin();
    
    Serial.println("Captive portal started successfully");
    return true;
}

void CaptivePortalManager::setupWebServer() {
    server.on("/", [this]() { handleRoot(); });
    server.on("/config", HTTP_POST, [this]() { handleConfig(); });
    server.onNotFound([this]() { handleNotFound(); });
}

void CaptivePortalManager::handleRoot() {
    server.send_P(200, "text/html", CONFIG_HTML);
}

void CaptivePortalManager::handleConfig() {
    if (server.hasArg("ssid")) {
        configuredSSID = server.arg("ssid");
        configuredPassword = server.hasArg("password") ? server.arg("password") : "";
        configuredWsServer = server.hasArg("server") ? server.arg("server") : "scherm.azckamp.nl";
        configuredWsPort = server.hasArg("port") ? server.arg("port") : "443";
        configuredLane = server.hasArg("lane") ? server.arg("lane") : "9";
        
        Serial.println("Configuration received:");
        Serial.println("SSID: " + configuredSSID);
        Serial.println("Server: " + configuredWsServer + ":" + configuredWsPort);
        Serial.println("Lane: " + configuredLane);
        
        // Save configuration
        saveConfiguration();
        
        // Send success response
        server.send_P(200, "text/html", SUCCESS_HTML);
        
        // Mark configuration as complete
        configComplete = true;
    } else {
        server.send(400, "text/plain", "Missing required parameters");
    }
}

void CaptivePortalManager::handleNotFound() {
    // Redirect all unknown requests to the config page (captive portal behavior)
    server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
    server.send(302, "text/plain", "Redirecting to setup page");
}

void CaptivePortalManager::loop() {
    dnsServer.processNextRequest();
    server.handleClient();
}

void CaptivePortalManager::saveConfiguration() {
    preferences.begin("stopwatch", false);
    preferences.putString("wifi_ssid", configuredSSID);
    preferences.putString("wifi_pass", configuredPassword);
    preferences.putString("ws_server", configuredWsServer);
    preferences.putUInt("ws_port", configuredWsPort.toInt());
    preferences.putUInt("lane", configuredLane.toInt());
    preferences.end();
    
    Serial.println("Configuration saved to preferences");
}

bool CaptivePortalManager::hasStoredCredentials() {
    Preferences prefs;
    prefs.begin("stopwatch", true);
    String ssid = prefs.getString("wifi_ssid", "");
    prefs.end();
    return ssid.length() > 0;
}

bool CaptivePortalManager::connectWithStoredCredentials() {
    Preferences prefs;
    prefs.begin("stopwatch", true);
    String ssid = prefs.getString("wifi_ssid", "");
    String password = prefs.getString("wifi_pass", "");
    prefs.end();
    
    if (ssid.length() == 0) {
        return false;
    }
    
    Serial.println("Connecting to: " + ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // Wait up to 10 seconds for connection
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        return true;
    } else {
        Serial.println("WiFi connection failed");
        return false;
    }
}

void CaptivePortalManager::stop() {
    server.stop();
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
}
