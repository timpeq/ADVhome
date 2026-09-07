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
    EntityDetailView(EntityManager& entityManager, ConfigManager& config, std::function<void()> onBack, std::function<void(String, String)> onCallService, std::function<void(String, float)> onSetVolume, std::function<void(String, float)> onSeekMedia, std::function<void(String, String, String, String)> onSecureService, bool showEntityName = true);
    
    void setEntityId(const String& id);
    String getEntityId() const { return _entityId; }
    
    void draw(DisplayManager& display) override;
    bool handleInput(KeyboardManager& keyboard) override;

private:
    EntityManager& _entityManager;
    std::function<void()> _onBack;
    std::function<void(String, String)> _onCallService; // format: domain, service
    std::function<void(String, float)> _onSetVolume;
    std::function<void(String, float)> _onSeekMedia;
    std::function<void(String, String, String, String)> _onSecureService;
    ConfigManager& _config;
    ScrollRepeater _scrollRepeater;
    ScrollRepeater _seekRepeater;
    bool _showEntityName;
    SecurityCodeModal _securityModal;
    String _pendingSecureService;
    bool _playPauseKeyHeld = false;
    
    float _targetVolume = -1.0f;
    uint32_t _lastVolumeChangeTime = 0;
    bool _volumeChangedLocally = false;

    float _targetSeekPosition = -1.0f;
    uint32_t _lastSeekChangeTime = 0;
    bool _seekChangedLocally = false;
    bool _wasSeekHeld = false;
    
    String _entityId;
};

#endif // ENTITY_DETAIL_VIEW_H
