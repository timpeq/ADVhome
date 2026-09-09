#include "FavoritesWidget.h"

// The section header stays put instead of being dropped once the list scrolls,
// so rows no longer shift by a row height the first time you press down.
static constexpr int kListTopY = 30;

FavoritesWidget::FavoritesWidget(EntityManager& entityManager, ConfigManager& config, std::function<void(String)> onEntitySelect, std::function<void(String)> onEntityToggle, std::function<void(String, int)> onEntityAdjust)
    : _favoritesView(entityManager, config, onEntitySelect, onEntityToggle, onEntityAdjust, kListTopY) {}

void FavoritesWidget::draw(DisplayManager& display) {
    auto canvas = display.getCanvas();

    canvas->setCursor(5, 19);
    canvas->setTextColor(TFT_CYAN);
    canvas->setTextSize(1);
    canvas->print("Favorites");

    _favoritesView.draw(display);
}

bool FavoritesWidget::handleInput(KeyboardManager& keyboard) {
    return _favoritesView.handleInput(keyboard);
}
