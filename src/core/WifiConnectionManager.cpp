#include "WifiConnectionManager.h"

void WifiConnectionManager::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
}

int WifiConnectionManager::scanNetworks() {
    _numNetworks = WiFi.scanNetworks();
    return _numNetworks;
}

std::vector<String> WifiConnectionManager::getNetworkList() {
    std::vector<String> networks;
    for (int i = 0; i < _numNetworks; i++) {
        String entry = WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + " dBm)";
        networks.push_back(entry);
    }
    return networks;
}

String WifiConnectionManager::getNetworkName(int index) {
    if (index >= 0 && index < _numNetworks) {
        return WiFi.SSID(index);
    }
    return "";
}

void WifiConnectionManager::connectTo(const String& ssid, const String& password) {
    WiFi.begin(ssid.c_str(), password.c_str());
    _connecting = true;
    _connectionFailed = false;
    _connectionStartTime = millis();
}

void WifiConnectionManager::update() {
    if (_connecting) {
        if (WiFi.status() == WL_CONNECTED) {
            _connecting = false;
            _connectionFailed = false;
        } else if (WiFi.status() == WL_CONNECT_FAILED || (millis() - _connectionStartTime > 15000)) { // 15s timeout
            _connecting = false;
            _connectionFailed = true;
            WiFi.disconnect();
        }
    } else if (isConnected()) {
        if (WiFi.status() != WL_CONNECTED) {
            // Auto reconnect on drop
            WiFi.disconnect();
            WiFi.reconnect();
        }
    }
}

bool WifiConnectionManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

bool WifiConnectionManager::hasConnectionFailed() const {
    return _connectionFailed;
}

String WifiConnectionManager::getIPAddress() const {
    return WiFi.localIP().toString();
}
