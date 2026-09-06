#include "FavoritesView.h"
#include "TextScroller.h"

FavoritesView::FavoritesView(EntityManager& entityManager, ConfigManager& config, std::function<void(String)> onEntitySelect)
    : _entityManager(entityManager), _config(config), _onEntitySelect(onEntitySelect) {}

void FavoritesView::onEnter() {
    auto favIds = _config.getFavorites();
    _cachedEntities.clear();
    for (const auto& id : favIds) {
        Entity e = _entityManager.getEntity(id);
        if (e.id != "") {
            _cachedEntities.push_back(e);
        }
    }
    
    if (_selectedIndex >= _cachedEntities.size()) {
        _selectedIndex = _cachedEntities.empty() ? 0 : _cachedEntities.size() - 1;
    }
}

void FavoritesView::draw(DisplayManager& display) {
    auto canvas = display.getCanvas();
    
    int y = 25;
    int itemsPerPage = (135 - 25) / 15;
    
    // Refresh cache
    auto favIds = _config.getFavorites();
    _cachedEntities.clear();
    for (const auto& id : favIds) {
        Entity e = _entityManager.getEntity(id);
        if (e.id != "") {
            _cachedEntities.push_back(e);
        }
    }
    
    if (_cachedEntities.empty()) {
        canvas->setCursor(5, y);
        canvas->setTextColor(TFT_LIGHTGREY);
        canvas->println("No favorites yet.");
        canvas->setCursor(5, y + 15);
        canvas->println("Press F in Entities");
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
        
        canvas->setTextColor(TFT_YELLOW);
        canvas->print("* ");
        if (idx == _selectedIndex) canvas->setTextColor(TFT_WHITE);
        else canvas->setTextColor(TFT_LIGHTGREY);
        
        String dispName = TextScroller::visible(entity.friendlyName, 20, idx == _selectedIndex);
        canvas->print(dispName);
        
        // State on the right
        canvas->setCursor(180, y + (i * 15));
        if (entity.state == "on") canvas->setTextColor(TFT_GREEN);
        else if (entity.state == "off") canvas->setTextColor(TFT_RED);
        else canvas->setTextColor(TFT_CYAN);
        
        canvas->print(entity.state);
    }
}

bool FavoritesView::handleInput(KeyboardManager& keyboard) {
    if (_cachedEntities.empty()) return false;
    
    bool handled = false;
    int scrollStyle = _config.getScrollStyle();
    int itemsPerPage = (135 - 37) / 15;
    
    if (keyboard.wasUpPressed() || (scrollStyle == 0 && keyboard.wasLeftPressed())) {
        if (_selectedIndex > 0) {
            _selectedIndex--;
            if (_selectedIndex < _scrollOffset) {
                _scrollOffset--;
            }
        }
        handled = true;
    }
    
    if (keyboard.wasDownPressed() || (scrollStyle == 0 && keyboard.wasRightPressed())) {
        if (_selectedIndex < _cachedEntities.size() - 1) {
            _selectedIndex++;
            if (_selectedIndex >= _scrollOffset + itemsPerPage) {
                _scrollOffset++;
            }
        }
        handled = true;
    }
    
    if (scrollStyle == 1 && keyboard.wasLeftPressed()) {
        if (_selectedIndex > 0) {
            _selectedIndex -= itemsPerPage;
            if (_selectedIndex < 0) _selectedIndex = 0;
            if (_selectedIndex < _scrollOffset) _scrollOffset = _selectedIndex;
        }
        handled = true;
    }
    
    if (scrollStyle == 1 && keyboard.wasRightPressed()) {
        if (_selectedIndex < _cachedEntities.size() - 1) {
            _selectedIndex += itemsPerPage;
            if (_selectedIndex >= _cachedEntities.size()) _selectedIndex = _cachedEntities.size() - 1;
            if (_selectedIndex >= _scrollOffset + itemsPerPage) {
                _scrollOffset = _selectedIndex - itemsPerPage + 1;
            }
        }
        handled = true;
    }
    
    auto chars = keyboard.getNewChars();
    for (char c : chars) {
        if (c == 'f' || c == 'F' || c == '*') {
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
                    int itemsPerPage = (135 - 37) / 15;
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
    }
    
    return handled;
}
