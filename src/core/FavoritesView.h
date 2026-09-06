#ifndef FAVORITES_VIEW_H
#define FAVORITES_VIEW_H

#include "View.h"
#include "EntityManager.h"
#include "ConfigManager.h"
#include <functional>

class FavoritesView : public View {
public:
    FavoritesView(EntityManager& entityManager, ConfigManager& config, std::function<void(String)> onEntitySelect);
    
    void onEnter() override;
    void draw(DisplayManager& display) override;
    bool handleInput(KeyboardManager& keyboard) override;

private:
    EntityManager& _entityManager;
    ConfigManager& _config;
    std::function<void(String)> _onEntitySelect;
    
    std::vector<Entity> _cachedEntities;
    int _selectedIndex = 0;
    int _scrollOffset = 0;
    
    String _searchPrefix = "";
    uint32_t _lastSearchTime = 0;
};

#endif // FAVORITES_VIEW_H
