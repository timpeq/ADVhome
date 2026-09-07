#ifndef ENTITIES_VIEW_H
#define ENTITIES_VIEW_H

#include "View.h"
#include "EntityManager.h"
#include "ConfigManager.h"
#include "ScrollRepeater.h"
#include <functional>

class EntitiesView : public View {
public:
    EntitiesView(EntityManager& entityManager, ConfigManager& config, std::function<void(String)> onEntitySelect, std::function<void(String)> onEntityToggle = nullptr);
    
    void onEnter() override;
    void draw(DisplayManager& display) override;
    bool handleInput(KeyboardManager& keyboard) override;

private:
    EntityManager& _entityManager;
    ConfigManager& _config;
    std::function<void(String)> _onEntitySelect;
    std::function<void(String)> _onEntityToggle;
    
    std::vector<const Entity*> _cachedEntities;
    int _selectedIndex = 0;
    int _scrollOffset = 0;
    bool _subTabFocus = false;
    ScrollRepeater _scrollRepeater;
    
    std::vector<String> _subTabs = {"Favorites", "All", "alarm_control_panel", "automation", "button", "climate", "cover", "fan", "input_boolean", "light", "lock", "media_player", "scene", "script", "sensor", "switch"};
    int _currentSubTab = 0;
    
    String _searchPrefix = "";
    uint32_t _lastSearchTime = 0;
    
    void refreshCache();
};

#endif // ENTITIES_VIEW_H
