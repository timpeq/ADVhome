#ifndef ENTITY_DETAIL_VIEW_H
#define ENTITY_DETAIL_VIEW_H

#include "View.h"
#include "EntityManager.h"
#include "ConfigManager.h"
#include <functional>

class EntityDetailView : public View {
public:
    EntityDetailView(EntityManager& entityManager, ConfigManager& config, std::function<void()> onBack, std::function<void(String, String)> onCallService);
    
    void setEntityId(const String& id);
    String getEntityId() const { return _entityId; }
    
    void draw(DisplayManager& display) override;
    bool handleInput(KeyboardManager& keyboard) override;

private:
    EntityManager& _entityManager;
    std::function<void()> _onBack;
    std::function<void(String, String)> _onCallService; // format: domain, service
    ConfigManager& _config;
    
    String _entityId;
};

#endif // ENTITY_DETAIL_VIEW_H
