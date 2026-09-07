#include "FavoritesView.h"
#include "TextScroller.h"
#include <algorithm>

namespace {
void sortFavorites(std::vector<Entity>& entities, int sortMode) {
    if (sortMode != 1) return;

    std::sort(entities.begin(), entities.end(), [](const Entity& first, const Entity& second) {
        int cmp = strcasecmp(first.friendlyName.c_str(), second.friendlyName.c_str());
        if (cmp == 0) return first.friendlyName < second.friendlyName;
        return cmp < 0;
    });
}
}

FavoritesView::FavoritesView(EntityManager& entityManager, ConfigManager& config, std::function<void(String)> onEntitySelect, std::function<void(String)> onEntityToggle, std::function<void(String, int)> onEntityAdjust, int topY)
    : _entityManager(entityManager), _config(config), _onEntitySelect(onEntitySelect), _onEntityToggle(onEntityToggle), _onEntityAdjust(onEntityAdjust), _topY(topY), _scrollRepeater(config), _valueRepeater(config) {}

void FavoritesView::onEnter() {
    auto favIds = _config.getFavorites();
    _cachedEntities.clear();
    bool hideUnavailable = _config.getHideUnavailable();
    for (const auto& id : favIds) {
        Entity e = _entityManager.getEntity(id);
        if (e.id != "") {
            if (hideUnavailable && e.state == "unavailable") continue;
            _cachedEntities.push_back(e);
        }
    }
    sortFavorites(_cachedEntities, _config.getFavoritesSort());
    
    if (_selectedIndex >= _cachedEntities.size()) {
        _selectedIndex = _cachedEntities.empty() ? 0 : _cachedEntities.size() - 1;
    }
}

void FavoritesView::draw(DisplayManager& display) {
    auto canvas = display.getCanvas();
    
    int y = _topY;
    int itemsPerPage = (135 - y) / 15;
    
    // Refresh cache
    auto favIds = _config.getFavorites();
    _cachedEntities.clear();
    bool hideUnavailable = _config.getHideUnavailable();
    for (const auto& id : favIds) {
        Entity e = _entityManager.getEntity(id);
        if (e.id != "") {
            if (hideUnavailable && e.state == "unavailable") continue;
            _cachedEntities.push_back(e);
        }
    }
    sortFavorites(_cachedEntities, _config.getFavoritesSort());
    
    if (_cachedEntities.empty()) {
        canvas->setCursor(5, y);
        canvas->setTextColor(TFT_LIGHTGREY);
        canvas->println("No favorites yet.");
        canvas->setCursor(5, y + 15);
        canvas->println("Press Ctrl-F in Entities");
        canvas->setCursor(5, y + 30);
        canvas->println("tab to add some.");
        return;
    }
    
    for (int i = 0; i < itemsPerPage; i++) {
        int idx = _scrollOffset + i;
        if (idx >= _cachedEntities.size()) break;
        
        const auto& entity = _cachedEntities[idx];
        
        if (idx == _selectedIndex) {
            canvas->fillRect(0, y + (i * 15) - 2, 240, 15, TFT_BLUE);
            canvas->setTextColor(TFT_WHITE);
        } else {
            canvas->setTextColor(TFT_LIGHTGREY);
        }
        
        canvas->setCursor(5, y + (i * 15));

        String dispName = TextScroller::visible(entity.friendlyName, 22, idx == _selectedIndex);
        canvas->print(dispName);
        
        // Scenes store their last-activated timestamp as state; keep the list compact.
        String displayState = entity.domain == "scene" ? "Scene" : entity.state;
        displayState.replace("_", " ");

        // State on the right
        canvas->setCursor(165, y + (i * 15));
        if (entity.state == "on") canvas->setTextColor(TFT_GREEN);
        else if (entity.state == "off") canvas->setTextColor(TFT_RED);
        else canvas->setTextColor(TFT_CYAN);
        
        canvas->print(TextScroller::visible(displayState, 12, false));
    }
}

bool FavoritesView::handleInput(KeyboardManager& keyboard) {
    if (_cachedEntities.empty()) return false;
    
    bool handled = false;
    int itemsPerPage = (135 - _topY) / 15;

    int direction = _scrollRepeater.update(keyboard);
    if (direction < 0 && _selectedIndex > 0) {
        _selectedIndex--;
        if (_selectedIndex < _scrollOffset) _scrollOffset--;
        handled = true;
    } else if (direction > 0 && _selectedIndex < (int)_cachedEntities.size() - 1) {
        _selectedIndex++;
        if (_selectedIndex >= _scrollOffset + itemsPerPage) _scrollOffset++;
        handled = true;
    } else {
        int adjDir = _valueRepeater.updatePlusMinus(keyboard);
        if (adjDir != 0) {
            if (_onEntityAdjust) {
                _onEntityAdjust(_cachedEntities[_selectedIndex].id, adjDir);
            }
            handled = true;
        }
    }
    
    auto chars = keyboard.getNewChars();
    for (char c : chars) {
        if ((c == 'f' || c == 'F') && keyboard.isCtrlHeld()) {
            String id = _cachedEntities[_selectedIndex].id;
            _config.removeFavorite(id);
            handled = true;
            
            // Adjust selection if we removed the last item
            if (_selectedIndex >= _cachedEntities.size() - 1 && _selectedIndex > 0) {
                _selectedIndex--;
            }
        } else if (isAlphaNumeric(c) || c == ' ') {
            if (millis() - _lastSearchTime > 1000) {
                _searchPrefix = "";
            }
            _searchPrefix += String(c);
            _lastSearchTime = millis();
            
            for (size_t i = 0; i < _cachedEntities.size(); i++) {
                if (_cachedEntities[i].friendlyName.substring(0, _searchPrefix.length()).equalsIgnoreCase(_searchPrefix)) {
                    _selectedIndex = i;
                    int itemsPerPage = (135 - _topY) / 15;
                    if (_selectedIndex < _scrollOffset) _scrollOffset = _selectedIndex;
                    else if (_selectedIndex >= _scrollOffset + itemsPerPage) {
                        _scrollOffset = _selectedIndex - itemsPerPage + 1;
                    }
                    handled = true;
                    break;
                }
            }
        }
    }
    
    if (keyboard.wasEnterPressed()) {
        if (_onEntitySelect) {
            _onEntitySelect(_cachedEntities[_selectedIndex].id);
        }
        handled = true;
    } else if (keyboard.wasSpacePressed()) {
        if (_onEntityToggle) {
            _onEntityToggle(_cachedEntities[_selectedIndex].id);
        }
        handled = true;
    }
    
    return handled;
}
