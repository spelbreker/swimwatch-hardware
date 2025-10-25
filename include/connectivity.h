#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H

#include <WiFi.h>
#include <Preferences.h>
#include <WebServer.h>
#include <DNSServer.h>

// WiFi Configuration
class ConnectivityManager {
private:
    Preferences preferences;
    
    bool wifiConnected;
    
    // Simple config portal
    WebServer* server;
    DNSServer* dnsServer;
    bool portalRunning;
    
public:
    static const char* CONFIG_PORTAL_SSID;
    static const char* CONFIG_PORTAL_PASSWORD;
    
public:
    ConnectivityManager();
    ~ConnectivityManager();
    
    // WiFi Management
    bool initWiFi();
    void startConfigPortal();
    void handleWiFiEvents();
    void updateWiFiStatus();  // Update internal WiFi connection status
    
    // Custom config portal
    bool startCustomConfigPortal();
    void stopConfigPortal();
    bool isConfigPortalRunning();
    
    // Configuration management
    bool saveConfig(const String& key, const String& value);
    String loadConfig(const String& key);
    bool clearConfig();
    
    // Status and information
    bool isConnected();
    String getConnectionStatus();
    IPAddress getLocalIP();
    bool isWiFiConnected() const { return wifiConnected; }
    bool isAccessPointRunning();  // Check if AP mode is active
};

#endif // CONNECTIVITY_H
