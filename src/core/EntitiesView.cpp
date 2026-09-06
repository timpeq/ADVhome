#include "EntitiesView.h"
#include "TextScroller.h"
#include <algorithm>

EntitiesView::EntitiesView(EntityManager& entityManager, ConfigManager& config, std::function<void(String)> onEntitySelect)
    : _entityManager(entityManager), _config(config), _onEntitySelect(onEntitySelect) {}

void EntitiesView::refreshCache() {
    _cachedEntities.clear();
    
    String currentDomain = _subTabs[_currentSubTab];
    const auto& entitiesMap = _entityManager.getEntitiesMap();
    
    for (const auto& pair : entitiesMap) {
        if (currentDomain == "All" || pair.second.domain == currentDomain) {
            _cachedEntities.push_back(&pair.second);
        }
    }
    
    // Sort in-place by pointer - no Entity copies needed
    std::sort(_cachedEntities.begin(), _cachedEntities.end(), [](const Entity* a, const Entity* b) {
        int cmp = strcasecmp(a->friendlyName.c_str(), b->friendlyName.c_str());
        if (cmp == 0) return a->friendlyName < b->friendlyName;
        return cmp < 0;
    });
    
    if (_selectedIndex >= (int)_cachedEntities.size()) {
        _selectedIndex = _cachedEntities.empty() ? 0 : _cachedEntities.size() - 1;
    }
    
    if (_scrollOffset > _selectedIndex) {
        _scrollOffset = 0;
    }
}

void EntitiesView::onEnter() {
    refreshCache();
}

void EntitiesView::draw(DisplayManager& display) {
    auto canvas = display.getCanvas();
    
    // Draw sub-tab bar
    canvas->fillRect(0, 20, 240, 15, 0x2124); // Slightly darker than tab bar
    canvas->setTextWrap(false);
    
    int unoffsetX = 0;
    for (int i = 0; i < _currentSubTab; i++) {
        unoffsetX += _subTabs[i].length() * 6 + 10;
    }
    
    int currentTabWidth = _subTabs[_currentSubTab].length() * 6 + 10;
    int offsetX = 120 - unoffsetX - (currentTabWidth / 2);
    
    int currentX = offsetX;
    for (size_t i = 0; i < _subTabs.size(); i++) {
        int tabWidth = _subTabs[i].length() * 6 + 10;
        
        if (currentX + tabWidth > 0 && currentX < 240) {
            if (i == (size_t)_currentSubTab) {
                canvas->fillRect(currentX, 20, tabWidth, 15, TFT_DARKCYAN);
                canvas->setTextColor(TFT_WHITE);
            } else {
                canvas->setTextColor(TFT_LIGHTGREY);
            }
            canvas->setTextSize(1);
            canvas->setCursor(currentX + 5, 24);
            canvas->print(_subTabs[i]);
        }
        currentX += tabWidth;
    }
    canvas->setTextWrap(true);
    
    // Draw the list starting below the sub-tab bar (Y=35)
    int y = 37;
    int itemsPerPage = (135 - 37) / 15;
    
    if (_cachedEntities.empty()) {
        canvas->setCursor(5, y);
        canvas->setTextColor(TFT_LIGHTGREY);
        canvas->println("No entities found.");
        return;
    }
    
    for (int i = 0; i < itemsPerPage; i++) {
        int idx = _scrollOffset + i;
        if (idx >= (int)_cachedEntities.size()) break;
        
        const auto* entity = _cachedEntities[idx];
        
        if (idx == _selectedIndex) {
            canvas->fillRect(0, y + (i * 15) - 2, 240, 15, TFT_BLUE);
            canvas->setTextColor(TFT_WHITE);
        } else {
            canvas->setTextColor(TFT_LIGHTGREY);
        }
        
        canvas->setCursor(5, y + (i * 15));
        
        bool isFav = _config.isFavorite(entity->id);
        if (isFav) {
            canvas->setTextColor(TFT_YELLOW);
            canvas->print("* ");
            if (idx == _selectedIndex) canvas->setTextColor(TFT_WHITE);
            else canvas->setTextColor(TFT_LIGHTGREY);
        } else {
            canvas->print("  ");
        }
        
        String dispName = TextScroller::visible(entity->friendlyName, 20, idx == _selectedIndex);
        canvas->print(dispName);
        
        // Scenes store their last-activated timestamp as state; keep the list compact.
        String displayState = entity->domain == "scene" ? "Scene" : entity->state;

        // State on the right
        canvas->setCursor(165, y + (i * 15));
        if (entity->state == "on") canvas->setTextColor(TFT_GREEN);
        else if (entity->state == "off") canvas->setTextColor(TFT_RED);
        else canvas->setTextColor(TFT_CYAN);
        
        canvas->print(TextScroller::visible(displayState, 12, false));
    }
}

bool EntitiesView::handleInput(KeyboardManager& keyboard) {
    bool handled = false;
    
    auto chars = keyboard.getNewChars();
    for (char c : chars) {
        if (c == '[') {
            if (_currentSubTab > 0) _currentSubTab--;
            else _currentSubTab = _subTabs.size() - 1;
            refreshCache();
            handled = true;
        } else if (c == ']') {
            if (_currentSubTab < (int)_subTabs.size() - 1) _currentSubTab++;
            else _currentSubTab = 0;
            refreshCache();
            handled = true;
        } else if (c == 'f' || c == 'F' || c == '*') {
            if (!_cachedEntities.empty()) {
                String id = _cachedEntities[_selectedIndex]->id;
                if (_config.isFavorite(id)) {
                    _config.removeFavorite(id);
                } else {
                    _config.addFavorite(id);
                }
                handled = true;
            }
        } else if (isAlphaNumeric(c) || c == ' ') {
            if (millis() - _lastSearchTime > 1000) {
                _searchPrefix = "";
            }
            _searchPrefix += String(c);
            _lastSearchTime = millis();
            
            for (size_t i = 0; i < _cachedEntities.size(); i++) {
                if (_cachedEntities[i]->friendlyName.substring(0, _searchPrefix.length()).equalsIgnoreCase(_searchPrefix)) {
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
    
    if (handled) return true;
    
    if (_cachedEntities.empty()) return false;
    
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
        if (_selectedIndex < (int)_cachedEntities.size() - 1) {
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
        if (_selectedIndex < (int)_cachedEntities.size() - 1) {
            _selectedIndex += itemsPerPage;
            if (_selectedIndex >= (int)_cachedEntities.size()) _selectedIndex = _cachedEntities.size() - 1;
            if (_selectedIndex >= _scrollOffset + itemsPerPage) {
                _scrollOffset = _selectedIndex - itemsPerPage + 1;
            }
        }
        handled = true;
    }
    
    if (keyboard.wasEnterPressed()) {
        if (_onEntitySelect) {
            _onEntitySelect(_cachedEntities[_selectedIndex]->id);
        }
        handled = true;
    }
    
    return handled;
}
