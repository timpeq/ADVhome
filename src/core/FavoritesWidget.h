#ifndef FAVORITES_WIDGET_H
#define FAVORITES_WIDGET_H

#include "HomeWidget.h"
#include "FavoritesView.h"

class FavoritesWidget : public HomeWidget {
public:
    FavoritesWidget(EntityManager& entityManager, ConfigManager& config, std::function<void(String)> onEntitySelect);

    void draw(DisplayManager& display) override;
    bool handleInput(KeyboardManager& keyboard) override;

private:
    FavoritesView _favoritesView;
};

#endif // FAVORITES_WIDGET_H
