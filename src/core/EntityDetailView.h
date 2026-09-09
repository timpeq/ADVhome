#ifndef ENTITY_DETAIL_VIEW_H
#define ENTITY_DETAIL_VIEW_H

#include "View.h"
#include "EntityManager.h"
#include "ConfigManager.h"
#include "ScrollRepeater.h"
#include "SecurityCodeModal.h"
#include <functional>

class EntityDetailView : public View {
public:
    EntityDetailView(EntityManager& entityManager, ConfigManager& config, std::function<void()> onBack, std::function<void(String, String)> onCallService, std::function<void(String, float)> onSetVolume, std::function<void(String, float)> onSeekMedia, std::function<void(String, String, String, String)> onSecureService, std::function<void(String, float)> onSetClimateTemp, std::function<void(String, float, float)> onSetClimateRange, std::function<void(String, String)> onSetHvacMode, bool showEntityName = true);
    
    void setEntityId(const String& id);
    String getEntityId() const { return _entityId; }
    
    void draw(DisplayManager& display) override;
    bool handleInput(KeyboardManager& keyboard) override;

private:
    void drawClimate(M5Canvas* canvas, const Entity& entity);
    bool handleClimateInput(KeyboardManager& keyboard, const Entity& entity);

private:
    EntityManager& _entityManager;
    std::function<void()> _onBack;
    std::function<void(String, String)> _onCallService; // format: domain, service
    std::function<void(String, float)> _onSetVolume;
    std::function<void(String, float)> _onSeekMedia;
    std::function<void(String, String, String, String)> _onSecureService;
    std::function<void(String, float)> _onSetClimateTemp;
    std::function<void(String, float, float)> _onSetClimateRange;
    std::function<void(String, String)> _onSetHvacMode;
    ConfigManager& _config;
    ScrollRepeater _scrollRepeater;
    ScrollRepeater _seekRepeater;
    ScrollRepeater _climateRepeater;
    bool _showEntityName;
    SecurityCodeModal _securityModal;
    String _pendingSecureService;
    bool _playPauseKeyHeld = false;
    
    // Volume: optimistic local setpoint while an adjustment is in flight
    float _volumeTarget = 0.0f;
    bool _volumeChangedLocally = false;
    uint32_t _lastVolumeChangeTime = 0;
    
    // Seek: optimistic local setpoint
    float _seekTarget = 0.0f;
    uint32_t _seekHoldStartTime = 0;
    uint32_t _lastSeekChangeTime = 0;
    bool _seekChangedLocally = false;
    bool _wasSeekHeld = false;
    
    // Climate: optimistic local setpoint while an adjustment is in flight.
    float _climateTarget = 0.0f;
    float _climateLow = 0.0f;
    float _climateHigh = 0.0f;
    bool _climateChangedLocally = false;
    uint32_t _lastClimateChangeTime = 0;
    
    String _entityId;
};

#endif // ENTITY_DETAIL_VIEW_H
