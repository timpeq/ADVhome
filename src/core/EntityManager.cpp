#include "EntityManager.h"
#include <algorithm>

void EntityManager::clear() {
    _entities.clear();
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
        _entities[entity_id] = e;
    } else {
        _entities[entity_id].state = state;
        if (!friendly_name.isEmpty()) {
            _entities[entity_id].friendlyName = friendly_name;
        }
    }
}

void EntityManager::updateMediaAttributes(const String& entity_id, const String& title, const String& artist, const String& album, float duration, float position, float volume, bool muted) {
    auto it = _entities.find(entity_id);
    if (it != _entities.end() && it->second.domain == "media_player") {
        it->second.mediaTitle = title;
        it->second.mediaArtist = artist;
        it->second.mediaAlbum = album;
        it->second.mediaDuration = duration;
        it->second.mediaPosition = position;
        it->second.mediaPositionUpdatedAt = millis();
        it->second.volumeLevel = volume;
        it->second.isVolumeMuted = muted;
    }
}

std::vector<Entity> EntityManager::getAllEntities() const {
    std::vector<Entity> result;
    for (const auto& pair : _entities) {
        result.push_back(pair.second);
    }
    std::sort(result.begin(), result.end(), [](const Entity& a, const Entity& b) {
        int cmp = strcasecmp(a.friendlyName.c_str(), b.friendlyName.c_str());
        if (cmp == 0) return a.friendlyName < b.friendlyName;
        return cmp < 0;
    });
    return result;
}

std::vector<Entity> EntityManager::getEntitiesByDomain(const String& domain) const {
    std::vector<Entity> result;
    for (const auto& pair : _entities) {
        if (pair.second.domain == domain) {
            result.push_back(pair.second);
        }
    }
    std::sort(result.begin(), result.end(), [](const Entity& a, const Entity& b) {
        int cmp = strcasecmp(a.friendlyName.c_str(), b.friendlyName.c_str());
        if (cmp == 0) return a.friendlyName < b.friendlyName;
        return cmp < 0;
    });
    return result;
}

Entity EntityManager::getEntity(const String& id) const {
    auto it = _entities.find(id);
    if (it != _entities.end()) {
        return it->second;
    }
    return Entity(); // Empty
}
