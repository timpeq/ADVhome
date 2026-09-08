#include "ConfigManager.h"

ConfigManager::ConfigManager() {}

void ConfigManager::begin() {
    _prefs.begin("advhome", false);
    reloadPowerSettingsCache();
}

void ConfigManager::reloadPowerSettingsCache() {
    _escDeepSleep = _prefs.getBool("esc_dpsleep", false);
    _brightness = _prefs.getInt("brightness", 200);
    _dimTO = _prefs.getInt("dimTO", 30);
    _dispOffTO = _prefs.getInt("dispOffTO", 60);
    _softSleepTO = _prefs.getInt("softSleepTO", 120);
    _deepSleepTO = _prefs.getInt("deepSleepTO", 3600);
    _cacheLoaded = true;
}

bool ConfigManager::hasWifiConfig() {
    return _prefs.isKey("wifi_ssid") && _prefs.isKey("wifi_pass");
}

String ConfigManager::getWifiSSID() {
    return _prefs.getString("wifi_ssid", "");
}

String ConfigManager::getWifiPassword() {
    return _prefs.getString("wifi_pass", "");
}

void ConfigManager::saveWifiConfig(const String& ssid, const String& password) {
    _prefs.putString("wifi_ssid", ssid);
    _prefs.putString("wifi_pass", password);
}

bool ConfigManager::getEscDeepSleep() {
    return _escDeepSleep;
}

void ConfigManager::setEscDeepSleep(bool enable) {
    _escDeepSleep = enable;
    _prefs.putBool("esc_dpsleep", enable);
}

void ConfigManager::clearWifiConfig() {
    _prefs.remove("wifi_ssid");
    _prefs.remove("wifi_pass");
}

bool ConfigManager::hasHAConfig() {
    return _prefs.isKey("ha_url") && _prefs.isKey("ha_token");
}

String ConfigManager::getHAUrl() {
    return _prefs.getString("ha_url", "");
}

String ConfigManager::getHAToken() {
    return _prefs.getString("ha_token", "");
}

void ConfigManager::saveHAConfig(const String& url, const String& token) {
    _prefs.putString("ha_url", url);
    _prefs.putString("ha_token", token);
}

void ConfigManager::clearHAConfig() {
    _prefs.remove("ha_url");
    _prefs.remove("ha_token");
}

std::vector<String> ConfigManager::getFavorites() {
    String favStr = _prefs.getString("favorites", "");
    std::vector<String> result;
    int start = 0;
    int end = favStr.indexOf(',');
    while (end != -1) {
        result.push_back(favStr.substring(start, end));
        start = end + 1;
        end = favStr.indexOf(',', start);
    }
    if (start < favStr.length()) {
        result.push_back(favStr.substring(start));
    }
    return result;
}

void ConfigManager::addFavorite(const String& entity_id) {
    if (isFavorite(entity_id)) return;
    String favStr = _prefs.getString("favorites", "");
    if (favStr.length() > 0) {
        favStr += ",";
    }
    favStr += entity_id;
    _prefs.putString("favorites", favStr);
}

void ConfigManager::removeFavorite(const String& entity_id) {
    auto favs = getFavorites();
    String newStr = "";
    for (const auto& fav : favs) {
        if (fav != entity_id) {
            if (newStr.length() > 0) newStr += ",";
            newStr += fav;
        }
    }
    _prefs.putString("favorites", newStr);
}

bool ConfigManager::isFavorite(const String& entity_id) {
    auto favs = getFavorites();
    for (const auto& fav : favs) {
        if (fav == entity_id) return true;
    }
    return false;
}

int ConfigManager::getFavoritesSort() {
    return _prefs.getInt("fav_sort", 0);
}

void ConfigManager::setFavoritesSort(int sort) {
    _prefs.putInt("fav_sort", sort);
}

// Adding new settings to ConfigManager.cpp
bool ConfigManager::getShowBattery() {
    return _prefs.getBool("show_battery", true);
}

void ConfigManager::setShowBattery(bool show) {
    _prefs.putBool("show_battery", show);
}

bool ConfigManager::getHideUnavailable() {
    return _prefs.getBool("hideUnavail", false);
}

void ConfigManager::setHideUnavailable(bool hide) {
    _prefs.putBool("hideUnavail", hide);
}

bool ConfigManager::getShowChat() {
    return _prefs.getBool("show_chat", true);
}

void ConfigManager::setShowChat(bool show) {
    _prefs.putBool("show_chat", show);
}

bool ConfigManager::getTtsEnabled() {
    return _prefs.getBool("ttsEnabled", true);
}

void ConfigManager::setTtsEnabled(bool enabled) {
    _prefs.putBool("ttsEnabled", enabled);
}

String ConfigManager::getVoicePipelineId() {
    return _prefs.getString("voicePipeId", "");
}

String ConfigManager::getVoicePipelineName() {
    return _prefs.getString("voicePipeName", "");
}

void ConfigManager::setVoicePipeline(const String& id, const String& name) {
    _prefs.putString("voicePipeId", id);
    _prefs.putString("voicePipeName", name);
}

int ConfigManager::getDisplayBrightness() {
    return _brightness;
}

void ConfigManager::setDisplayBrightness(int brightness) {
    _brightness = brightness;
    _prefs.putInt("brightness", brightness);
}

int ConfigManager::getDimTimeout() {
    return _dimTO;
}

void ConfigManager::setDimTimeout(int timeout) {
    _dimTO = timeout;
    _prefs.putInt("dimTO", timeout);
}

int ConfigManager::getDisplayOffTimeout() {
    return _dispOffTO;
}

void ConfigManager::setDisplayOffTimeout(int timeout) {
    _dispOffTO = timeout;
    _prefs.putInt("dispOffTO", timeout);
}

int ConfigManager::getSoftSleepTimeout() {
    return _softSleepTO;
}

void ConfigManager::setSoftSleepTimeout(int timeout) {
    _softSleepTO = timeout;
    _prefs.putInt("softSleepTO", timeout);
}

int ConfigManager::getDeepSleepTimeout() {
    return _deepSleepTO;
}

void ConfigManager::setDeepSleepTimeout(int timeout) {
    _deepSleepTO = timeout;
    _prefs.putInt("deepSleepTO", timeout);
}

int ConfigManager::getReconnectInterval() {
    return _prefs.getInt("recon_int", 5000);
}

void ConfigManager::setReconnectInterval(int ms) {
    _prefs.putInt("recon_int", ms);
}

int ConfigManager::getScrollStyle() {
    return _prefs.getInt("scroll_sty", 0);
}

void ConfigManager::setScrollStyle(int style) {
    _prefs.putInt("scroll_sty", style);
}

int ConfigManager::getScrollDelay() {
    return _prefs.getInt("scroll_delay", 500);
}

void ConfigManager::setScrollDelay(int ms) {
    _prefs.putInt("scroll_delay", ms);
}

int ConfigManager::getScrollSpeed() {
    return _prefs.getInt("scroll_speed", 100);
}

void ConfigManager::setScrollSpeed(int ms) {
    _prefs.putInt("scroll_speed", ms);
}

int ConfigManager::getSeekStep() {
    return _prefs.getInt("seek_step", 5);
}

void ConfigManager::setSeekStep(int seconds) {
    _prefs.putInt("seek_step", seconds);
}

int ConfigManager::getSeekStepMax() {
    return _prefs.getInt("seek_step_max", 30);
}

void ConfigManager::setSeekStepMax(int seconds) {
    _prefs.putInt("seek_step_max", seconds);
}
