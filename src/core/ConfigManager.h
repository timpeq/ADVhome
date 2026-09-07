#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <vector>

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

    // Favorites Configuration
    std::vector<String> getFavorites();
    void addFavorite(const String& entity_id);
    void removeFavorite(const String& entity_id);
    bool isFavorite(const String& entity_id);
    int getFavoritesSort();
    void setFavoritesSort(int sort);
    
    // UI Settings
    bool getShowBattery();
    void setShowBattery(bool show);
    
    bool getHideUnavailable();
    void setHideUnavailable(bool hide);
    
    // Connection Settings
    int getReconnectInterval();
    void setReconnectInterval(int ms);
    
    // 0 = Left/Right scrolls 1 item, 1 = Left/Right pages up/down
    int getScrollStyle();
    void setScrollStyle(int style);
    int getScrollDelay();
    void setScrollDelay(int ms);
    int getScrollSpeed();
    void setScrollSpeed(int ms);
    
    int getSeekStep();
    void setSeekStep(int seconds);
    
    int getSeekStepMax();
    void setSeekStepMax(int seconds);
    
private:
    Preferences _prefs;
};

#endif // CONFIG_MANAGER_H
