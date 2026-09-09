#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#include <Arduino.h>
#include <math.h>
#include <vector>
#include <map>

// Climate/thermostat attributes (only populated for the climate domain).
// NAN means the attribute was absent from the entity's state.
struct ClimateState {
    float currentTemperature = NAN;
    float targetTemperature = NAN;  // single-setpoint thermostats
    float targetTempHigh = NAN;     // heat_cool range mode
    float targetTempLow = NAN;
    float currentHumidity = NAN;
    float minTemp = 7.0f;
    float maxTemp = 35.0f;
    float targetTempStep = 0.5f;
    String hvacAction;              // heating / cooling / idle / off / drying / fan
    String hvacModes;               // comma-joined, e.g. "off,heat,cool,heat_cool"

    bool hasTarget() const { return !isnan(targetTemperature); }
    bool hasRange() const { return !isnan(targetTempHigh) || !isnan(targetTempLow); }
};

struct MediaPlayerState {
    String title;
    String artist;
    String album;
    float duration = 0;
    float position = 0;
    uint32_t positionUpdatedAt = 0;
    float volumeLevel = 0;
    bool isVolumeMuted = false;
};

struct Entity {
    String id;
    String domain;
    String state;
    String friendlyName;
    uint32_t lastUpdate = 0;
};

class EntityManager {
public:
    void clear();
    void updateEntity(const String& entity_id, const String& state, const String& friendly_name = "");
    void updateMediaAttributes(const String& entity_id, const String& title, const String& artist, const String& album, float duration, float position, float volume, bool muted);
    void updateClimateAttributes(const String& entity_id, const ClimateState& climate);
    
    Entity getEntity(const String& id) const;
    const std::map<String, Entity>& getEntitiesMap() const { return _entities; }
    
    bool getMediaPlayerState(const String& id, MediaPlayerState& state) const;
    bool getClimateState(const String& id, ClimateState& state) const;
    
    bool isSupportedDomain(const String& domain) const;

private:
    std::map<String, Entity> _entities;
    std::map<String, MediaPlayerState> _mediaStates;
    std::map<String, ClimateState> _climateStates;
};

#endif // ENTITY_MANAGER_H
