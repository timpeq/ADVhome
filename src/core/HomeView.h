#ifndef HOME_VIEW_H
#define HOME_VIEW_H

#include "View.h"
#include "FavoritesWidget.h"
#include <vector>

class HomeView : public View {
public:
    HomeView(EntityManager& entityManager, ConfigManager& config, std::function<void(String)> onEntitySelect, std::function<void(String)> onEntityToggle = nullptr, std::function<void(String, int)> onEntityAdjust = nullptr);

    void draw(DisplayManager& display) override;
    bool handleInput(KeyboardManager& keyboard) override;

private:
    FavoritesWidget _favoritesWidget;
    std::vector<HomeWidget*> _widgets;
};

#endif // HOME_VIEW_H