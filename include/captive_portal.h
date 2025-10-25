#ifndef CAPTIVE_PORTAL_H
#define CAPTIVE_PORTAL_H

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>

class CaptivePortalManager {
private:
    DNSServer dnsServer;
    WebServer server;
    Preferences preferences;
    
    bool configComplete;
    String configuredSSID;
    String configuredPassword;
    String configuredWsServer;
    String configuredWsPort;
    String configuredLane;
    
    void setupWebServer();
    void handleRoot();
    void handleConfig();
    void handleNotFound();
    
public:
    CaptivePortalManager();
    ~CaptivePortalManager();
    
    bool begin();
    void loop();
    bool isConfigComplete() const { return configComplete; }
    
    // Get configured values
    String getConfiguredSSID() const { return configuredSSID; }
    String getConfiguredPassword() const { return configuredPassword; }
    String getConfiguredWsServer() const { return configuredWsServer; }
    String getConfiguredWsPort() const { return configuredWsPort; }
    String getConfiguredLane() const { return configuredLane; }
    
    // Save configuration to preferences
    void saveConfiguration();
    
    // Check if WiFi credentials exist
    static bool hasStoredCredentials();
    static bool connectWithStoredCredentials();
    
    void stop();
};

#endif // CAPTIVE_PORTAL_H
