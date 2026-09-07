#include "ConfigManager.h"

ConfigManager::ConfigManager() {}

void ConfigManager::begin() {
    _prefs.begin("advhome", false);
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
// Adding new settings to ConfigManager.cpp
bool ConfigManager::getShowBattery() {
    return _prefs.getBool("show_battery", true);
}

void ConfigManager::setShowBattery(bool show) {
    _prefs.putBool("show_battery", show);
}

int ConfigManager::getReconnectInterval() {
    return _prefs.getInt("recon_int", 5000);
}

void ConfigManager::setReconnectInterval(int ms) {
    _prefs.putInt("recon_int", ms);
}

int ConfigManager::getBackButtonStyle() {
    return _prefs.getInt("back_btn_sty", 0);
}

void ConfigManager::setBackButtonStyle(int style) {
    _prefs.putInt("back_btn_sty", style);
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
