#include "connectivity.h"

// WiFi Configuration Portal Settings
const char* ConnectivityManager::CONFIG_PORTAL_SSID = "T-Display-S3-Setup";
const char* ConnectivityManager::CONFIG_PORTAL_PASSWORD = "stopwatch123";

ConnectivityManager::ConnectivityManager() : 
    ntpUDP(),
    timeClient(ntpUDP, "pool.ntp.org", 0, 60000),
    wifiConnected(false),
    ntpSynced(false),
    lastNtpSync(0),
    server(nullptr),
    dnsServer(nullptr),
    portalRunning(false) {
}

ConnectivityManager::~ConnectivityManager() {
    stopConfigPortal();
}

bool ConnectivityManager::initWiFi() {
    Serial.println("Initializing WiFi...");
    
    // Initialize preferences for configuration storage
    if (!preferences.begin("stopwatch", false)) {
        Serial.println("Failed to initialize preferences");
        return false;
    }
    
    // Try to connect with saved credentials
    String ssid = loadConfig("wifi_ssid");
    String password = loadConfig("wifi_password");
    
    if (ssid.length() > 0) {
        Serial.println("Attempting to connect with saved credentials...");
        WiFi.begin(ssid.c_str(), password.c_str());
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            Serial.println();
            Serial.println("WiFi connected successfully!");
            Serial.println("IP address will be available later");
            return true;
        }
    }
    
    Serial.println();
    Serial.println("No saved credentials or connection failed");
    wifiConnected = false;
    return false;
}

bool ConnectivityManager::startCustomConfigPortal() {
    if (portalRunning) {
        Serial.println("Config portal already running");
        return true;
    }
    
    Serial.println("Starting simple WiFi configuration portal...");
    
    // Disconnect from any existing connection
    WiFi.disconnect(true);
    delay(1000);
    
    // Start Access Point
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(CONFIG_PORTAL_SSID, CONFIG_PORTAL_PASSWORD)) {
        Serial.println("Failed to create Access Point");
        return false;
    }
    
    // Configure AP IP
    IPAddress apIP(192, 168, 4, 1);
    IPAddress netMsk(255, 255, 255, 0);
    WiFi.softAPConfig(apIP, apIP, netMsk);
    
    delay(1000);
    
    Serial.println("=== Simple WiFi Setup Portal ===");
    Serial.print("AP Name: "); Serial.println(CONFIG_PORTAL_SSID);
    Serial.print("AP Password: "); Serial.println(CONFIG_PORTAL_PASSWORD);
    Serial.println("Portal URL: http://192.168.4.1");
    Serial.println("================================");
    
    // Initialize DNS server for captive portal
    dnsServer = new DNSServer();
    dnsServer->start(53, "*", apIP);
    
    // Initialize simple web server
    server = new WebServer(80);
    
    // Serve main configuration page
    server->on("/", [this]() {
        String html = "<!DOCTYPE html><html><head><title>WiFi Setup</title>";
        html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
        html += "<style>body{font-family:Arial;margin:20px;}";
        html += "input{width:100%;padding:8px;margin:5px 0;}</style></head><body>";
        html += "<h2>T-Display S3 WiFi Setup</h2>";
        html += "<form action='/save' method='POST'>";
        html += "WiFi Name:<input type='text' name='ssid' required><br>";
        html += "WiFi Password:<input type='password' name='password'><br>";
        html += "Server:<input type='text' name='ws_server' value='scherm.azckamp.nl'><br>";
        html += "Port:<input type='number' name='ws_port' value='443'><br>";
        html += "Lane:<input type='number' name='lane' value='9' min='1' max='99'><br>";
        html += "<button type='submit'>Save</button>";
        html += "</form></body></html>";
        
        server->send(200, "text/html", html);
    });
    
    // Handle configuration save
    server->on("/save", [this]() {
        String ssid = server->arg("ssid");
        String password = server->arg("password");
        String ws_server = server->arg("ws_server");
        String ws_port = server->arg("ws_port");
        String lane = server->arg("lane");
        
        Serial.println("Saving configuration...");
        Serial.println("SSID: " + ssid);
        Serial.println("WebSocket Server: " + ws_server);
        Serial.println("WebSocket Port: " + ws_port);
        Serial.println("Lane: " + lane);
        
        // Save configuration
        saveConfig("wifi_ssid", ssid);
        saveConfig("wifi_password", password);
        saveConfig("ws_server", ws_server);
        saveConfig("ws_port", ws_port);
        saveConfig("lane", lane);
        
        String html = "<!DOCTYPE html><html><head><title>Saved</title>";
        html += "<meta http-equiv='refresh' content='3;url=/'>";
        html += "</head><body>";
        html += "<h2>Configuration Saved!</h2>";
        html += "<p>Restarting in 3 seconds...</p>";
        html += "</body></html>";
        
        server->send(200, "text/html", html);
        
        // Schedule restart after sending response
        Serial.println("Configuration saved, restarting in 3 seconds...");
        delay(3000);
        ESP.restart();
    });
    
    // Captive portal redirect
    server->onNotFound([this]() {
        server->sendHeader("Location", "http://192.168.4.1/");
        server->send(302, "text/plain", "");
    });
    
    server->begin();
    portalRunning = true;
    
    Serial.println("Simple config portal started successfully!");
    return true;
}

void ConnectivityManager::stopConfigPortal() {
    if (!portalRunning) return;
    
    Serial.println("Stopping config portal...");
    
    if (server) {
        server->stop();
        delete server;
        server = nullptr;
    }
    
    if (dnsServer) {
        dnsServer->stop();
        delete dnsServer;
        dnsServer = nullptr;
    }
    
    WiFi.softAPdisconnect(true);
    portalRunning = false;
    
    Serial.println("Config portal stopped");
}

bool ConnectivityManager::isConfigPortalRunning() {
    return portalRunning;
}

void ConnectivityManager::startConfigPortal() {
    startCustomConfigPortal();
}

void ConnectivityManager::handleWiFiEvents() {
    if (portalRunning) {
        if (dnsServer) {
            dnsServer->processNextRequest();
        }
        if (server) {
            server->handleClient();
        }
    }
}

void ConnectivityManager::updateWiFiStatus() {
    wifiConnected = (WiFi.status() == WL_CONNECTED);
}

bool ConnectivityManager::initNTP() {
    if (!wifiConnected) {
        Serial.println("Cannot initialize NTP: WiFi not connected");
        return false;
    }
    
    Serial.println("Initializing NTP client...");
    timeClient.begin();
    return syncTimeWithNTP();
}

bool ConnectivityManager::syncTimeWithNTP() {
    if (!wifiConnected) {
        return false;
    }
    
    Serial.println("Syncing time with NTP server...");
    
    int attempts = 0;
    while (!timeClient.update() && attempts < 10) {
        delay(1000);
        attempts++;
        Serial.print(".");
    }
    
    if (timeClient.isTimeSet()) {
        ntpSynced = true;
        lastNtpSync = millis();
        Serial.println();
        Serial.println("NTP time synchronized successfully");
        return true;
    } else {
        Serial.println();
        Serial.println("Failed to sync with NTP server");
        return false;
    }
}

unsigned long ConnectivityManager::getEpochTime() {
    if (!ntpSynced) return 0;
    
    // Check if we need to resync
    if (millis() - lastNtpSync > NTP_SYNC_INTERVAL) {
        syncTimeWithNTP();
    }
    
    return timeClient.getEpochTime();
}

bool ConnectivityManager::isTimeSynced() {
    return ntpSynced;
}

bool ConnectivityManager::saveConfig(const String& key, const String& value) {
    if (!preferences.begin("stopwatch", false)) {
        Serial.println("Failed to open preferences for writing");
        return false;
    }
    
    bool success = preferences.putString(key.c_str(), value);
    preferences.end();
    
    if (success) {
        Serial.println("Saved " + key + ": " + value);
    } else {
        Serial.println("Failed to save " + key);
    }
    
    return success;
}

String ConnectivityManager::loadConfig(const String& key) {
    if (!preferences.begin("stopwatch", true)) {
        Serial.println("Failed to open preferences for reading");
        return "";
    }
    
    String value = preferences.getString(key.c_str(), "");
    preferences.end();
    
    return value;
}

bool ConnectivityManager::clearConfig() {
    if (!preferences.begin("stopwatch", false)) {
        Serial.println("Failed to open preferences for clearing");
        return false;
    }
    
    bool success = preferences.clear();
    preferences.end();
    
    if (success) {
        Serial.println("Configuration cleared");
    } else {
        Serial.println("Failed to clear configuration");
    }
    
    return success;
}

bool ConnectivityManager::isConnected() {
    // During portal mode, always return false to avoid WiFi status checks
    if (portalRunning) {
        return false;
    }
    
    // Safe WiFi status check
    return wifiConnected && (WiFi.status() == WL_CONNECTED);
}

// ULTRA-SAFE version of getConnectionStatus - avoid ALL potentially unsafe operations
String ConnectivityManager::getConnectionStatus() {
    // Only check basic WiFi status
    if (!wifiConnected) {
        return "WiFi disconnected";
    }
    
    wl_status_t status = WiFi.status();
    if (status != WL_CONNECTED) {
        return "WiFi disconnected";
    }
    
    // Just return connected status without trying to get IP or SSID
    return "WiFi connected";
}

String ConnectivityManager::getTimeStatus() {
    if (!wifiConnected) {
        return "WiFi not connected";
    }
    
    if (!ntpSynced) {
        return "Time not synchronized";
    }
    
    unsigned long epoch = getEpochTime();
    return String("Time synced (") + String(epoch) + ")";
}

// ULTRA-SAFE version of getLocalIP - return zero IP if any issue
IPAddress ConnectivityManager::getLocalIP() {
    if (!wifiConnected || WiFi.status() != WL_CONNECTED) {
        return IPAddress(0, 0, 0, 0);
    }
    
    // Just return zero IP to avoid any crashes
    return IPAddress(0, 0, 0, 0);
}

bool ConnectivityManager::isAccessPointRunning() {
    return portalRunning;
}
