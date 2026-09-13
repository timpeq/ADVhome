#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <vector>
#include "SettingsDefaults.h"

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

    // The deepest automatic level. 0 = off, the device stops at Soft Sleep.
    // 1 = light sleep, resumes in place. 2 = deep sleep, resets the chip and
    // costs a reboot and Home Assistant reconnect on wake.
    int getDeepSleepMode();
    void setDeepSleepMode(int mode);

    // Restricts wake to the GO button. Ignored while the mode is off.
    bool getWakeOnGoOnly();
    void setWakeOnGoOnly(bool on);

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

    // Reordering swaps in RAM and bumps the revision so lists redraw at once.
    // The NVS write is deferred: holding Ctrl-Down to carry a row several
    // places would otherwise be one flash write per step.
    bool swapFavorites(const String& a, const String& b);
    void flushPendingFavorites();
    
    // UI Settings
    bool getShowBattery();
    void setShowBattery(bool show);
    
    bool getHideUnavailable();
    void setHideUnavailable(bool hide);

    bool getShowChat();
    void setShowChat(bool show);

    bool getGoButtonToChat();
    void setGoButtonToChat(bool on);

    // List shortcuts. Both act on the highlighted row without opening it, which
    // is fast but easy to trigger by accident, so each can be turned off.
    bool getListToggleEnabled();
    void setListToggleEnabled(bool on);

    bool getListAdjustEnabled();
    void setListAdjustEnabled(bool on);
    
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
    bool _favDirty = false;
    uint32_t _favDirtyAt = 0;

    // Cached copies of settings read every loop() iteration by checkPowerManagement();
    // avoids hammering NVS (and flooding logs with NOT_FOUND) when keys are unset.
    bool _cacheLoaded = false;
    bool _escDeepSleep = Defaults::EscForSleep;
    int _brightness = Defaults::Brightness.def;
    int _dimTO = Defaults::DimTimeout.def;
    int _dispOffTO = Defaults::DisplayOffTimeout.def;
    int _softSleepTO = Defaults::SoftSleepTimeout.def;
    int _deepSleepTO = Defaults::DeepSleepTimeout.def;
};

#endif // CONFIG_MANAGER_H
