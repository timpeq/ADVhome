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
    bool getEscDeepSleep();
    void setEscDeepSleep(bool enable);

    void clearWifiConfig();

    // Reload cached power-management settings from NVS (call after external prefs changes)
    void reloadPowerSettingsCache();

    // Home Assistant Configuration
    bool hasHAConfig();
    String getHAUrl();
    String getHAToken();
    void saveHAConfig(const String& url, const String& token);
    void clearHAConfig();

    // Favorites Configuration
    const std::vector<String>& getFavorites();
    void addFavorite(const String& entity_id);
    void removeFavorite(const String& entity_id);
    bool isFavorite(const String& entity_id);
    // Bumped on every add/remove so views can rebuild their caches lazily.
    uint32_t getFavoritesRevision() const { return _favRevision; }
    int getFavoritesSort();
    void setFavoritesSort(int sort);
    
    // UI Settings
    bool getShowBattery();
    void setShowBattery(bool show);
    
    bool getHideUnavailable();
    void setHideUnavailable(bool hide);

    bool getShowChat();
    void setShowChat(bool show);

    bool getGoButtonToChat();
    void setGoButtonToChat(bool on);
    
    bool getTtsEnabled();
    void setTtsEnabled(bool enabled);

    int getTtsVolume();          // 0-100 (percent)
    void setTtsVolume(int pct);

    bool getTtsDebug();
    void setTtsDebug(bool on);

    String getVoicePipelineId();
    String getVoicePipelineName();
    void setVoicePipeline(const String& id, const String& name);

    int getDisplayBrightness();
    void setDisplayBrightness(int brightness);
    
    int getDimTimeout();
    void setDimTimeout(int timeout);
    
    int getDisplayOffTimeout();
    void setDisplayOffTimeout(int timeout);
    
    int getSoftSleepTimeout();
    void setSoftSleepTimeout(int timeout);
    
    int getDeepSleepTimeout();
    void setDeepSleepTimeout(int timeout);
    
    // Connection Settings
    int getReconnectInterval();
    void setReconnectInterval(int ms);
    
    int getScrollDelay();
    void setScrollDelay(int ms);
    int getScrollSpeed();
    void setScrollSpeed(int ms);
    
    // 0 = follow Home Assistant's target_temp_step, 1 = force 0.5, 2 = force 1.0
    int getTempStep();
    void setTempStep(int mode);

    int getSeekStep();
    void setSeekStep(int seconds);
    
    int getSeekStepMax();
    void setSeekStepMax(int seconds);
    
private:
    Preferences _prefs;

    void loadFavorites();
    void saveFavorites();
    std::vector<String> _favorites;
    bool _favLoaded = false;
    uint32_t _favRevision = 0;

    // Cached copies of settings read every loop() iteration by checkPowerManagement();
    // avoids hammering NVS (and flooding logs with NOT_FOUND) when keys are unset.
    bool _cacheLoaded = false;
    bool _escDeepSleep = false;
    int _brightness = 200;
    int _dimTO = 30;
    int _dispOffTO = 60;
    int _softSleepTO = 120;
    int _deepSleepTO = 3600;
};

#endif // CONFIG_MANAGER_H
