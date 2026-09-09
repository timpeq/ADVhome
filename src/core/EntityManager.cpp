#include "EntityManager.h"

void EntityManager::clear() {
    _entities.clear();
    _mediaStates.clear();
    _climateStates.clear();
}

bool EntityManager::isSupportedDomain(const String& domain) const {
    return domain == "light" || 
           domain == "alarm_control_panel" ||
           domain == "climate" || 
           domain == "switch" || 
           domain == "cover" || 
           domain == "media_player" || 
           domain == "fan" ||
           domain == "script" ||
           domain == "lock" ||
           domain == "automation" ||
           domain == "input_boolean" ||
           domain == "scene" ||
           domain == "sensor" ||
           domain == "button";
}

void EntityManager::updateEntity(const String& entity_id, const String& state, const String& friendly_name) {
    int dotIndex = entity_id.indexOf('.');
    if (dotIndex == -1) return;
    
    String domain = entity_id.substring(0, dotIndex);
    
    if (!isSupportedDomain(domain)) {
        return; // Filter out unsupported domains to save RAM
    }
    
    if (_entities.find(entity_id) == _entities.end()) {
        Entity e;
        e.id = entity_id;
        e.domain = domain;
        e.state = state;
        e.friendlyName = friendly_name.isEmpty() ? entity_id : friendly_name;
        e.lastUpdate = millis();
        _entities[entity_id] = e;
    } else {
        _entities[entity_id].state = state;
        _entities[entity_id].lastUpdate = millis();
        if (!friendly_name.isEmpty()) {
            _entities[entity_id].friendlyName = friendly_name;
        }
    }
}

void EntityManager::updateMediaAttributes(const String& entity_id, const String& title, const String& artist, const String& album, float duration, float position, float volume, bool muted) {
    if (_entities.find(entity_id) != _entities.end() && _entities[entity_id].domain == "media_player") {
        MediaPlayerState& s = _mediaStates[entity_id];
        s.title = title;
        s.artist = artist;
        s.album = album;
        s.duration = duration;
        s.position = position;
        s.positionUpdatedAt = millis();
        s.volumeLevel = volume;
        s.isVolumeMuted = muted;
    }
}

void EntityManager::updateClimateAttributes(const String& entity_id, const ClimateState& climate) {
    if (_entities.find(entity_id) != _entities.end() && _entities[entity_id].domain == "climate") {
        _climateStates[entity_id] = climate;
    }
}

Entity EntityManager::getEntity(const String& id) const {
    auto it = _entities.find(id);
    if (it != _entities.end()) {
        return it->second;
    }
    return Entity(); // Empty
}

bool EntityManager::getMediaPlayerState(const String& id, MediaPlayerState& state) const {
    auto it = _mediaStates.find(id);
    if (it != _mediaStates.end()) {
        state = it->second;
        return true;
    }
    return false;
}

bool EntityManager::getClimateState(const String& id, ClimateState& state) const {
    auto it = _climateStates.find(id);
    if (it != _climateStates.end()) {
        state = it->second;
        return true;
    }
    return false;
}
