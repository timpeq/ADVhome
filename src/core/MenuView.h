#ifndef MENU_VIEW_H
#define MENU_VIEW_H

#include "View.h"
#include "ConfigManager.h"
#include "ScrollRepeater.h"
#include <vector>

// The Menu tab. It owns no settings of its own: it is a launcher for the views
// that used to be reachable only as a flat Config list, plus the connection and
// attribution pages that have no other home. Sub-views are drawn full-screen
// beneath the tab bar and return here on Backspace/ESC.
class MenuView : public View {
public:
    MenuView(ConfigManager& config, const char* version);

    void addItem(const char* name, View* view);

    void onEnter() override;
    void onExit() override;
    void draw(DisplayManager& display) override;
    bool handleInput(KeyboardManager& keyboard) override;

private:
    struct Item {
        const char* name;
        View* view;
    };

    ConfigManager& _config;
    const char* _version;
    std::vector<Item> _items;
    View* _activeSubView = nullptr;
    int _selectedIndex = 0;
    ScrollRepeater _scrollRepeater;
};

#endif // MENU_VIEW_H
