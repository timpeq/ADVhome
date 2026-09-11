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
    EntityDetailView(EntityManager& entityManager, ConfigManager& config, std::function<void()> onBack, std::function<void(String, String)> onCallService, std::function<void(String, float)> onSetVolume, std::function<void(String, float)> onSeekMedia, std::function<void(String, String, String, String)> onSecureService, std::function<void(String, float)> onSetClimateTemp, std::function<void(String, float, float)> onSetClimateRange, std::function<void(String, String)> onSetHvacMode, std::function<void(String, int)> onSetBrightness, bool showEntityName = true);
    
    void setEntityId(const String& id);
    String getEntityId() const { return _entityId; }
    
    void draw(DisplayManager& display) override;
    bool handleInput(KeyboardManager& keyboard) override;

private:
    void drawClimate(M5Canvas* canvas, const Entity& entity);
    bool handleClimateInput(KeyboardManager& keyboard, const Entity& entity);
    bool handleBrightnessInput(KeyboardManager& keyboard, const Entity& entity);

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
    std::function<void(String, int)> _onSetBrightness;   // entity, percent
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
    bool _volumeChangedLocally = false;   // showing our own value, not HA's
    bool _volumePendingSend = false;      // that value has not been sent yet
    float _volumeBaseline = 0.0f;         // server value when the edit started
    uint32_t _volumeSentAt = 0;

    // Seek: optimistic local setpoint
    float _seekTarget = 0.0f;
    uint32_t _seekHoldStartTime = 0;
    bool _seekChangedLocally = false;
    bool _seekPendingSend = false;
    uint32_t _seekSentAt = 0;
    bool _wasSeekHeld = false;

    // Light brightness: optimistic local percentage, same scheme as volume.
    int _brightnessTarget = 0;
    bool _brightnessChangedLocally = false;
    bool _brightnessPendingSend = false;
    int _brightnessBaseline = 0;           // server percentage when the edit started
    uint32_t _brightnessSentAt = 0;

    // Climate: optimistic local setpoint while an adjustment is in flight.
    float _climateTarget = 0.0f;
    float _climateLow = 0.0f;
    float _climateHigh = 0.0f;
    bool _climateChangedLocally = false;   // showing our own setpoint, not HA's
    bool _climatePendingSend = false;      // that setpoint has not been sent yet
    uint32_t _lastClimateChangeTime = 0;
    uint32_t _climateSentAt = 0;
    
    String _entityId;
};

#endif // ENTITY_DETAIL_VIEW_H
