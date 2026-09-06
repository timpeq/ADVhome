#ifndef WIFI_CONNECTION_MANAGER_H
#define WIFI_CONNECTION_MANAGER_H

#include <WiFi.h>
#include <vector>
#include <Arduino.h>

class WifiConnectionManager {
public:
    void begin();
    
    // Scanning
    int scanNetworks();
    std::vector<String> getNetworkList();
    String getNetworkName(int index);
    
    // Connection
    void connectTo(const String& ssid, const String& password);
    void update(); // call periodically to handle reconnects
    
    // Status
    bool isConnected() const;
    bool hasConnectionFailed() const;
    String getIPAddress() const;
    
private:
    int _numNetworks = 0;
    bool _connecting = false;
    bool _connectionFailed = false;
    uint32_t _connectionStartTime = 0;
};

#endif // WIFI_CONNECTION_MANAGER_H
