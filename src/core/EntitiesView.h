#ifndef ENTITIES_VIEW_H
#define ENTITIES_VIEW_H

#include "View.h"
#include "EntityManager.h"
#include "ConfigManager.h"
#include <functional>

class EntitiesView : public View {
public:
    EntitiesView(EntityManager& entityManager, ConfigManager& config, std::function<void(String)> onEntitySelect);
    
    void onEnter() override;
    void draw(DisplayManager& display) override;
    bool handleInput(KeyboardManager& keyboard) override;

private:
    EntityManager& _entityManager;
    ConfigManager& _config;
    std::function<void(String)> _onEntitySelect;
    
    std::vector<const Entity*> _cachedEntities;
    int _selectedIndex = 0;
    int _scrollOffset = 0;
    
    std::vector<String> _subTabs = {"All", "light", "switch", "climate", "fan", "cover", "media_player", "script", "lock", "automation", "input_boolean", "scene", "button"};
    int _currentSubTab = 0;
    
    String _searchPrefix = "";
    uint32_t _lastSearchTime = 0;
    
    void refreshCache();
};

#endif // ENTITIES_VIEW_H
