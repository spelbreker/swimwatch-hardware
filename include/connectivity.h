#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H

#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Preferences.h>
#include <WebServer.h>
#include <DNSServer.h>

// WiFi Configuration
class ConnectivityManager {
private:
    WiFiUDP ntpUDP;
    NTPClient timeClient;
    Preferences preferences;
    
    bool wifiConnected;
    bool ntpSynced;
    unsigned long lastNtpSync;
    
    // Simple config portal
    WebServer* server;
    DNSServer* dnsServer;
    bool portalRunning;
    
    static const unsigned long NTP_SYNC_INTERVAL = 3600000; // 1 hour in ms
    
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
    
    // NTP Time Management
    bool initNTP();
    bool syncTimeWithNTP();
    unsigned long getEpochTime();
    bool isTimeSynced();
    
    // Configuration management
    bool saveConfig(const String& key, const String& value);
    String loadConfig(const String& key);
    bool clearConfig();
    
    // Status and information
    bool isConnected();
    String getConnectionStatus();
    String getTimeStatus();
    IPAddress getLocalIP();
    bool isWiFiConnected() const { return wifiConnected; }
    bool isNTPSynced() const { return ntpSynced; }
    bool isAccessPointRunning();  // Check if AP mode is active
};

#endif // CONNECTIVITY_H
