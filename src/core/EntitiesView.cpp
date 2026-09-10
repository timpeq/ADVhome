#include "EntitiesView.h"
#include <algorithm>

const char* const EntitiesView::kDomains[] = {
    "Favorites", "All", "alarm_control_panel", "automation", "button", "climate",
    "cover", "fan", "input_boolean", "light", "lock", "media_player", "scene",
    "script", "sensor", "switch"
};
const int EntitiesView::kDomainCount = sizeof(kDomains) / sizeof(kDomains[0]);

EntitiesView::EntitiesView(EntityManager& entityManager, ConfigManager& config, std::function<void(String)> onEntitySelect, std::function<void(String)> onEntityToggle, std::function<void(String, int)> onEntityAdjust)
    : _entityManager(entityManager), _config(config), _onEntitySelect(onEntitySelect),
      _onEntityToggle(onEntityToggle), _onEntityAdjust(onEntityAdjust),
      _list(entityManager, config) {
    _list.setTopY(37);
    _visibleTabs.push_back(0);
    _visibleTabs.push_back(1);
}

String EntitiesView::currentDomain() const {
    if (_visibleTabs.empty()) return "All";
    return kDomains[_visibleTabs[_currentTab]];
}

void EntitiesView::rebuildVisibleTabs() {
    const auto& entitiesMap = _entityManager.getEntitiesMap();
    if (_tabsBuiltFor == entitiesMap.size()) return;
    _tabsBuiltFor = entitiesMap.size();

    String keepDomain = currentDomain();

    // Favorites and All are always offered; the rest earn their slot.
    _visibleTabs.clear();
    for (int i = 0; i < kDomainCount; i++) {
        if (i < 2) {
            _visibleTabs.push_back(i);
            continue;
        }
        for (const auto& pair : entitiesMap) {
            if (pair.second.domain == kDomains[i]) {
                _visibleTabs.push_back(i);
                break;
            }
        }
    }

    _currentTab = 0;
    for (size_t i = 0; i < _visibleTabs.size(); i++) {
        if (keepDomain == kDomains[_visibleTabs[i]]) {
            _currentTab = i;
            break;
        }
    }
}

void EntitiesView::refreshCache() {
    String domain = currentDomain();
    const auto& entitiesMap = _entityManager.getEntitiesMap();

    _list.items.clear();

    if (domain == "Favorites") {
        for (const auto& id : _config.getFavorites()) {
            auto entityIt = entitiesMap.find(id);
            if (entityIt != entitiesMap.end()) _list.items.push_back(&entityIt->second);
        }
    } else {
        for (const auto& pair : entitiesMap) {
            if (domain == "All" || pair.second.domain == domain) {
                _list.items.push_back(&pair.second);
            }
        }
    }

    // Every other sub-tab is alphabetical, but Favorites has a user-defined
    // order. Sorting it here overrode the Favorites Sort setting and made
    // reordering invisible, since the stored order was re-sorted on every
    // rebuild.
    bool alphabetical = (domain != "Favorites") || _config.getFavoritesSort() == 1;
    if (alphabetical) {
        std::sort(_list.items.begin(), _list.items.end(), [](const Entity* a, const Entity* b) {
            int cmp = strcasecmp(a->friendlyName.c_str(), b->friendlyName.c_str());
            if (cmp == 0) return a->friendlyName < b->friendlyName;
            return cmp < 0;
        });
    }

    _list.clampSelection();
}

void EntitiesView::stepTab(int direction) {
    if (_visibleTabs.empty()) return;
    _currentTab = (_currentTab + direction + (int)_visibleTabs.size()) % (int)_visibleTabs.size();
    refreshCache();
}

void EntitiesView::onEnter() {
    rebuildVisibleTabs();
    refreshCache();
}

void EntitiesView::draw(DisplayManager& display) {
    auto canvas = display.getCanvas();

    size_t knownCount = _tabsBuiltFor;
    rebuildVisibleTabs();
    if (knownCount != _tabsBuiltFor) refreshCache();

    // Sub-tab carousel, centred on the active domain.
    canvas->fillRect(0, 20, 240, 15, 0x2124);
    canvas->setTextWrap(false);
    canvas->setTextSize(1);

    int currentX = 120;
    for (int i = 0; i < _currentTab; i++) {
        currentX -= (int)strlen(kDomains[_visibleTabs[i]]) * 6 + 10;
    }
    currentX -= ((int)currentDomain().length() * 6 + 10) / 2;

    for (size_t i = 0; i < _visibleTabs.size(); i++) {
        String label = kDomains[_visibleTabs[i]];
        int tabWidth = label.length() * 6 + 10;

        if (currentX + tabWidth > 0 && currentX < 240) {
            if (i == (size_t)_currentTab) {
                canvas->fillRect(currentX, 20, tabWidth, 15, _subTabFocus ? TFT_BLUE : TFT_DARKCYAN);
                canvas->setTextColor(TFT_WHITE);
            } else {
                canvas->setTextColor(TFT_LIGHTGREY);
            }
            label.replace("_", " ");
            canvas->setCursor(currentX + 5, 24);
            canvas->print(label);
        }
        currentX += tabWidth;
    }
    canvas->setTextWrap(true);

    if (_list.items.empty()) {
        canvas->setCursor(5, 45);
        canvas->setTextColor(TFT_LIGHTGREY);
        canvas->print("No entities found.");
        return;
    }

    _list.draw(*canvas, true);
}

bool EntitiesView::handleInput(KeyboardManager& keyboard) {
    // Reordering only means anything on the Favorites sub-tab, and only while
    // the sort is "Order"; under "Name" the list would snap straight back.
    _list.setReorderable(currentDomain() == "Favorites" && _config.getFavoritesSort() == 0);

    if (keyboard.wasLeftPressed()) {
        stepTab(-1);
        _subTabFocus = true;
        return true;
    }
    if (keyboard.wasRightPressed()) {
        stepTab(1);
        _subTabFocus = true;
        return true;
    }

    if (_subTabFocus) {
        if (keyboard.wasDownPressed()) {
            _subTabFocus = false;
            return true;
        }
        return false;
    }

    if (_list.items.empty()) return false;

    EntityList::Input input = _list.handleInput(keyboard);
    bool handled = input.changed;

    if (input.hitTop) _subTabFocus = true;

    // Read the row once: toggling a favourite can rebuild the list under us.
    const Entity* item = _list.current();
    if (!item) return handled;
    String id = item->id;

    if (input.reorder != 0) {
        // Swap with the neighbouring *visible* row rather than moving one place
        // in storage: a hidden unavailable favourite in between would otherwise
        // swallow the press.
        int idx = _list.selectedIndex();
        int other = idx + input.reorder;
        if (other >= 0 && other < (int)_list.items.size() &&
            _config.swapFavorites(id, _list.items[other]->id)) {
            refreshCache();
            _list.moveSelection(input.reorder);
        }
        return true;
    }

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
        if (_config.isFavorite(id)) _config.removeFavorite(id);
        else _config.addFavorite(id);
        if (currentDomain() == "Favorites") refreshCache();
    }

    return handled;
}
