#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

class ConfigManager {
public:
    ConfigManager();
    void begin();
    
    // WiFi Configuration
    bool hasWifiConfig();
    String getWifiSSID();
    String getWifiPassword();
    void saveWifiConfig(const String& ssid, const String& password);
    void clearWifiConfig();

    // Home Assistant Configuration
    bool hasHAConfig();
    String getHAUrl();
    String getHAToken();
    void saveHAConfig(const String& url, const String& token);
    void clearHAConfig();

private:
    Preferences _prefs;
};

#endif // CONFIG_MANAGER_H
