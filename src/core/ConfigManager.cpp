#include "ConfigManager.h"
#include <algorithm>

ConfigManager::ConfigManager() {}

void ConfigManager::begin() {
    _prefs.begin("advhome", false);
    reloadPowerSettingsCache();
}

void ConfigManager::reloadPowerSettingsCache() {
    _escForSleep = _prefs.getBool("esc_dpsleep", Defaults::EscForSleep);
    _brightness = _prefs.getInt("brightness", Defaults::Brightness.def);
    _dimTO = _prefs.getInt("dimTO", Defaults::DimTimeout.def);
    _dispOffTO = _prefs.getInt("dispOffTO", Defaults::DisplayOffTimeout.def);
    _softSleepTO = _prefs.getInt("softSleepTO", Defaults::SoftSleepTimeout.def);
    _sleepTO = _prefs.getInt("deepSleepTO", Defaults::SleepTimeout.def);
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

bool ConfigManager::getEscForSleep() {
    return _escForSleep;
}

void ConfigManager::setEscForSleep(bool enable) {
    _escForSleep = enable;
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

// Favourites are read on every list redraw, so they are parsed from NVS once
// and kept in RAM; writes go through to NVS immediately.
void ConfigManager::loadFavorites() {
    if (_favLoaded) return;
    _favLoaded = true;
    String favStr = _prefs.getString("favorites", "");
    int start = 0;
    while (start < (int)favStr.length()) {
        int end = favStr.indexOf(',', start);
        if (end == -1) end = favStr.length();
        if (end > start) _favorites.push_back(favStr.substring(start, end));
        start = end + 1;
    }
}

void ConfigManager::saveFavorites() {
    String favStr = "";
    for (const auto& fav : _favorites) {
        if (favStr.length() > 0) favStr += ",";
        favStr += fav;
    }
    _prefs.putString("favorites", favStr);
    _favRevision++;
}

const std::vector<String>& ConfigManager::getFavorites() {
    loadFavorites();
    return _favorites;
}

void ConfigManager::addFavorite(const String& entity_id) {
    if (isFavorite(entity_id)) return;
    _favorites.push_back(entity_id);
    saveFavorites();
}

void ConfigManager::removeFavorite(const String& entity_id) {
    loadFavorites();
    for (size_t i = 0; i < _favorites.size(); i++) {
        if (_favorites[i] == entity_id) {
            _favorites.erase(_favorites.begin() + i);
            saveFavorites();
            return;
        }
    }
}

bool ConfigManager::isFavorite(const String& entity_id) {
    loadFavorites();
    for (const auto& fav : _favorites) {
        if (fav == entity_id) return true;
    }
    return false;
}

bool ConfigManager::swapFavorites(const String& a, const String& b) {
    loadFavorites();
    int ia = -1, ib = -1;
    for (size_t i = 0; i < _favorites.size(); i++) {
        if (_favorites[i] == a) ia = i;
        else if (_favorites[i] == b) ib = i;
    }
    if (ia < 0 || ib < 0) return false;

    std::swap(_favorites[ia], _favorites[ib]);
    _favRevision++;
    _favDirty = true;
    _favDirtyAt = millis();
    return true;
}

void ConfigManager::flushPendingFavorites() {
    if (!_favDirty) return;
    if (millis() - _favDirtyAt < 1500) return;
    _favDirty = false;

    String favStr = "";
    for (const auto& fav : _favorites) {
        if (favStr.length() > 0) favStr += ",";
        favStr += fav;
    }
    _prefs.putString("favorites", favStr);
}

int ConfigManager::getFavoritesSort() {
    return _prefs.getInt("fav_sort", Defaults::FavoritesSort);
}

void ConfigManager::setFavoritesSort(int sort) {
    _prefs.putInt("fav_sort", sort);
}

bool ConfigManager::getShowBattery() {
    return _prefs.getBool("show_battery", Defaults::ShowBatteryInTab);
}

void ConfigManager::setShowBattery(bool show) {
    _prefs.putBool("show_battery", show);
}

bool ConfigManager::getHideUnavailable() {
    return _prefs.getBool("hideUnavail", Defaults::HideUnavailable);
}

void ConfigManager::setHideUnavailable(bool hide) {
    _prefs.putBool("hideUnavail", hide);
}

bool ConfigManager::getShowChat() {
    return _prefs.getBool("show_chat", Defaults::ShowChatTab);
}

void ConfigManager::setShowChat(bool show) {
    _prefs.putBool("show_chat", show);
}

bool ConfigManager::getGoButtonToChat() {
    return _prefs.getBool("goToChat", Defaults::GoButtonToChat);
}

void ConfigManager::setGoButtonToChat(bool on) {
    _prefs.putBool("goToChat", on);
}

// Sleep Depth, Sleep T/O and ESC for Sleep keep their original NVS keys
// (deepSleepMd, deepSleepTO, esc_dpsleep) so saved values survive the rename.
int ConfigManager::getSleepDepth() {
    return _prefs.getInt("deepSleepMd", Defaults::SleepDepth);
}

void ConfigManager::setSleepDepth(int mode) {
    _prefs.putInt("deepSleepMd", mode);
}

bool ConfigManager::getWakeOnGoOnly() {
    return _prefs.getBool("wakeGoOnly", Defaults::WakeOnGoOnly);
}

void ConfigManager::setWakeOnGoOnly(bool on) {
    _prefs.putBool("wakeGoOnly", on);
}

bool ConfigManager::getListToggleEnabled() {
    return _prefs.getBool("listToggle", Defaults::SpaceTogglesInList);
}

void ConfigManager::setListToggleEnabled(bool on) {
    _prefs.putBool("listToggle", on);
}

bool ConfigManager::getListAdjustEnabled() {
    return _prefs.getBool("listAdjust", Defaults::PlusMinusAdjustsInList);
}

void ConfigManager::setListAdjustEnabled(bool on) {
    _prefs.putBool("listAdjust", on);
}

bool ConfigManager::getTtsEnabled() {
    return _prefs.getBool("ttsEnabled", Defaults::TtsPlayback);
}

void ConfigManager::setTtsEnabled(bool enabled) {
    _prefs.putBool("ttsEnabled", enabled);
}

int ConfigManager::getTtsVolume() {
    return _prefs.getInt("ttsVol", Defaults::TtsVolume.def);
}

void ConfigManager::setTtsVolume(int pct) {
    _prefs.putInt("ttsVol", Defaults::TtsVolume.clamp(pct));
}

bool ConfigManager::getTtsDebug() {
    return _prefs.getBool("ttsDebug", Defaults::TtsDebug);
}

void ConfigManager::setTtsDebug(bool on) {
    _prefs.putBool("ttsDebug", on);
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

int ConfigManager::getSleepTimeout() {
    return _sleepTO;
}

void ConfigManager::setSleepTimeout(int timeout) {
    _sleepTO = timeout;
    _prefs.putInt("deepSleepTO", timeout);
}

int ConfigManager::getReconnectInterval() {
    return _prefs.getInt("recon_int", Defaults::ReconnectInterval.def);
}

void ConfigManager::setReconnectInterval(int ms) {
    _prefs.putInt("recon_int", ms);
}

int ConfigManager::getScrollDelay() {
    return _prefs.getInt("scroll_delay", Defaults::ScrollStartDelay.def);
}

void ConfigManager::setScrollDelay(int ms) {
    _prefs.putInt("scroll_delay", ms);
}

int ConfigManager::getScrollSpeed() {
    return _prefs.getInt("scroll_speed", Defaults::ScrollRepeat.def);
}

void ConfigManager::setScrollSpeed(int ms) {
    _prefs.putInt("scroll_speed", ms);
}

int ConfigManager::getTempStep() {
    return _prefs.getInt("temp_step", Defaults::TempStep);
}

void ConfigManager::setTempStep(int mode) {
    _prefs.putInt("temp_step", mode);
}

int ConfigManager::getSeekStep() {
    return _prefs.getInt("seek_step", Defaults::SeekStepMin);
}

void ConfigManager::setSeekStep(int seconds) {
    _prefs.putInt("seek_step", seconds);
}

int ConfigManager::getSeekStepMax() {
    return _prefs.getInt("seek_step_max", Defaults::SeekStepMax);
}

void ConfigManager::setSeekStepMax(int seconds) {
    _prefs.putInt("seek_step_max", seconds);
}
