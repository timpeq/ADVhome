#ifndef FAVORITES_VIEW_H
#define FAVORITES_VIEW_H

#include "View.h"
#include "EntityList.h"
#include <functional>

class FavoritesView : public View {
public:
    FavoritesView(EntityManager& entityManager, ConfigManager& config, std::function<void(String)> onEntitySelect, std::function<void(String)> onEntityToggle = nullptr, std::function<void(String, int)> onEntityAdjust = nullptr, int topY = 25);

    void onEnter() override;
    void draw(DisplayManager& display) override;
    bool handleInput(KeyboardManager& keyboard) override;

    bool isEmpty() const { return _list.items.empty(); }
    void setTopY(int topY) { _list.setTopY(topY); }

private:
    // Favourites change rarely; rebuild only when the stored list or the set of
    // known entities actually moves rather than on every frame.
    void refreshCache(bool force = false);

    EntityManager& _entityManager;
    ConfigManager& _config;
    std::function<void(String)> _onEntitySelect;
    std::function<void(String)> _onEntityToggle;
    std::function<void(String, int)> _onEntityAdjust;

    EntityList _list;
    uint32_t _cachedFavRevision = 0;
    size_t _cachedEntityCount = 0;
    int _cachedSort = -1;
    uint32_t _lastRefresh = 0;
    bool _cacheValid = false;
};

#endif // FAVORITES_VIEW_H
