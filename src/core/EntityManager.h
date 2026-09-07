#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <map>

struct Entity {
    String id;
    String domain;
    String state;
    String friendlyName;
    // Media player attributes (only populated for media_player domain)
    String mediaTitle;
    String mediaArtist;
    String mediaAlbum;
    float mediaDuration = 0;
    float mediaPosition = 0;
    uint32_t mediaPositionUpdatedAt = 0;
    float volumeLevel = 0;
    bool isVolumeMuted = false;
};

class EntityManager {
public:
    void clear();
    void updateEntity(const String& entity_id, const String& state, const String& friendly_name = "");
    void updateMediaAttributes(const String& entity_id, const String& title, const String& artist, const String& album, float duration, float position, float volume, bool muted);
    
    std::vector<Entity> getAllEntities() const;
    std::vector<Entity> getEntitiesByDomain(const String& domain) const;
    Entity getEntity(const String& id) const;
    const std::map<String, Entity>& getEntitiesMap() const { return _entities; }
    
    bool isSupportedDomain(const String& domain) const;

private:
    std::map<String, Entity> _entities;
};

#endif // ENTITY_MANAGER_H
