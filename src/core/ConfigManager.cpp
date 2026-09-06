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
