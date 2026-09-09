#include "FavoritesWidget.h"

FavoritesWidget::FavoritesWidget(EntityManager& entityManager, ConfigManager& config, std::function<void(String)> onEntitySelect, std::function<void(String)> onEntityToggle, std::function<void(String, int)> onEntityAdjust)
    : _favoritesView(entityManager, config, onEntitySelect, onEntityToggle, onEntityAdjust, 40) {}

void FavoritesWidget::draw(DisplayManager& display) {
    auto canvas = display.getCanvas();
    
    if (_favoritesView.getScrollOffset() == 0) {
        canvas->setCursor(5, 21);
        canvas->setTextColor(TFT_CYAN);
        canvas->setTextSize(1);
        canvas->print("Favorites");
        _favoritesView.setTopY(40);
    } else {
        _favoritesView.setTopY(25);
    }
    
    _favoritesView.draw(display);
}

bool FavoritesWidget::handleInput(KeyboardManager& keyboard) {
    return _favoritesView.handleInput(keyboard);
}
