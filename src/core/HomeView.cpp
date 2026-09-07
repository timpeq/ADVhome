#include "HomeView.h"

HomeView::HomeView(EntityManager& entityManager, ConfigManager& config, std::function<void(String)> onEntitySelect, std::function<void(String)> onEntityToggle, std::function<void(String, int)> onEntityAdjust)
    : _favoritesWidget(entityManager, config, onEntitySelect, onEntityToggle, onEntityAdjust) {
    _widgets.push_back(&_favoritesWidget);
}

void HomeView::draw(DisplayManager& display) {
    for (auto* widget : _widgets) widget->draw(display);
}

bool HomeView::handleInput(KeyboardManager& keyboard) {
    if (_widgets.empty()) return false;
    return _widgets.front()->handleInput(keyboard);
}