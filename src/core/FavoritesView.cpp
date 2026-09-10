#include "FavoritesView.h"
#include <algorithm>

FavoritesView::FavoritesView(EntityManager& entityManager, ConfigManager& config, std::function<void(String)> onEntitySelect, std::function<void(String)> onEntityToggle, std::function<void(String, int)> onEntityAdjust, int topY)
    : _entityManager(entityManager), _config(config), _onEntitySelect(onEntitySelect),
      _onEntityToggle(onEntityToggle), _onEntityAdjust(onEntityAdjust),
      _list(entityManager, config) {
    _list.setTopY(topY);
}

void FavoritesView::refreshCache(bool force) {
    const auto& entitiesMap = _entityManager.getEntitiesMap();
    uint32_t favRevision = _config.getFavoritesRevision();
    int sort = _config.getFavoritesSort();
    uint32_t now = millis();

    // Entity states churn constantly, but the membership of this list only
    // changes when a favourite is added or removed, an entity first appears, or
    // one goes (un)available. A short poll covers the last case cheaply.
    if (!force && _cacheValid && favRevision == _cachedFavRevision &&
        entitiesMap.size() == _cachedEntityCount && sort == _cachedSort &&
        now - _lastRefresh < 1000) {
        return;
    }

    _cachedFavRevision = favRevision;
    _cachedEntityCount = entitiesMap.size();
    _cachedSort = sort;
    _lastRefresh = now;
    _cacheValid = true;

    bool hideUnavailable = _config.getHideUnavailable();
    _list.items.clear();
    for (const auto& id : _config.getFavorites()) {
        auto entityIt = entitiesMap.find(id);
        if (entityIt == entitiesMap.end()) continue;
        if (hideUnavailable && entityIt->second.state == "unavailable") continue;
        _list.items.push_back(&entityIt->second);
    }

    if (sort == 1) {
        std::sort(_list.items.begin(), _list.items.end(), [](const Entity* a, const Entity* b) {
            int cmp = strcasecmp(a->friendlyName.c_str(), b->friendlyName.c_str());
            if (cmp == 0) return a->friendlyName < b->friendlyName;
            return cmp < 0;
        });
    }

    _list.clampSelection();
}

void FavoritesView::onEnter() {
    refreshCache(true);
}

void FavoritesView::draw(DisplayManager& display) {
    auto canvas = display.getCanvas();
    refreshCache();

    if (_list.items.empty()) {
        // First screen a new device shows, so it spells out the whole loop
        // rather than assuming TAB is a known way between tabs.
        canvas->setTextSize(1);
        canvas->setTextColor(TFT_LIGHTGREY);
        canvas->setCursor(5, 44);
        canvas->print("No favorites yet.");

        canvas->setTextColor(TFT_CYAN);
        canvas->setCursor(5, 62);
        canvas->print("1.");
        canvas->setCursor(5, 76);
        canvas->print("2.");
        canvas->setCursor(5, 90);
        canvas->print("3.");

        canvas->setTextColor(TFT_LIGHTGREY);
        canvas->setCursor(22, 62);
        canvas->print("TAB to the Entities tab");
        canvas->setCursor(22, 76);
        canvas->print("Highlight something");
        canvas->setCursor(22, 90);
        canvas->print("Ctrl-F adds it here");

        canvas->setTextColor(0x6B6D);
        canvas->setCursor(5, 110);
        canvas->print("Menu > Help lists every key");
        return;
    }

    _list.draw(*canvas, false);
}

bool FavoritesView::handleInput(KeyboardManager& keyboard) {
    if (_list.items.empty()) return false;

    EntityList::Input input = _list.handleInput(keyboard);
    bool handled = input.changed;

    // Read the row once: un-favouriting below can empty the list.
    const Entity* item = _list.current();
    if (!item) return handled;
    String id = item->id;

    if (input.adjust != 0 && _onEntityAdjust) {
        _onEntityAdjust(id, input.adjust);
    }

    if (keyboard.wasEnterPressed()) {
        if (_onEntitySelect) _onEntitySelect(id);
        handled = true;
    } else if (keyboard.wasSpacePressed() && _config.getListToggleEnabled()) {
        if (_onEntityToggle) _onEntityToggle(id);
        handled = true;
    }

    if (input.favToggle) {
        _config.removeFavorite(id);
        refreshCache(true);
    }

    return handled;
}
