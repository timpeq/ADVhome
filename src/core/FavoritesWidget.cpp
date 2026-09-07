#include "FavoritesWidget.h"

FavoritesWidget::FavoritesWidget(EntityManager& entityManager, ConfigManager& config, std::function<void(String)> onEntitySelect)
    : _favoritesView(entityManager, config, onEntitySelect, 40) {}

void FavoritesWidget::draw(DisplayManager& display) {
    auto canvas = display.getCanvas();
    canvas->setCursor(5, 21);
    canvas->setTextColor(TFT_CYAN);
    canvas->setTextSize(1);
    canvas->print("Favorites");
    _favoritesView.draw(display);
}

bool FavoritesWidget::handleInput(KeyboardManager& keyboard) {
    return _favoritesView.handleInput(keyboard);
}
