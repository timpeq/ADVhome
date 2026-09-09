#include "MenuView.h"

MenuView::MenuView(ConfigManager& config, const char* version)
    : _config(config), _version(version), _scrollRepeater(config) {}

void MenuView::addItem(const char* name, View* view) {
    _items.push_back({name, view});
}

void MenuView::onEnter() {
    // Returning to the tab always lands on the menu root rather than resuming
    // whatever sub-view was open, so Tab-cycling is predictable.
    if (_activeSubView) {
        _activeSubView->onExit();
        _activeSubView = nullptr;
    }
}

void MenuView::onExit() {
    if (_activeSubView) {
        _activeSubView->onExit();
        _activeSubView = nullptr;
    }
}

void MenuView::draw(DisplayManager& display) {
    if (_activeSubView) {
        _activeSubView->draw(display);
        return;
    }

    auto canvas = display.getCanvas();

    canvas->setTextSize(1);
    canvas->setTextColor(TFT_WHITE);
    canvas->setCursor(5, 21);
    canvas->print("ADVhome");
    canvas->setTextColor(TFT_DARKGREY);
    canvas->setCursor(60, 21);
    canvas->print(_version);
    canvas->drawFastHLine(0, 32, 240, 0x2965);

    int y = 40;
    for (size_t i = 0; i < _items.size(); i++) {
        bool selected = (i == (size_t)_selectedIndex);
        if (selected) {
            canvas->fillRect(0, y + (i * 20) - 3, 240, 19, TFT_BLUE);
            canvas->setTextColor(TFT_WHITE);
        } else {
            canvas->setTextColor(TFT_LIGHTGREY);
        }
        canvas->setCursor(8, y + (i * 20));
        canvas->print(_items[i].name);

        canvas->setTextColor(selected ? TFT_WHITE : TFT_DARKGREY);
        canvas->setCursor(225, y + (i * 20));
        canvas->print(">");
    }
}

bool MenuView::handleInput(KeyboardManager& keyboard) {
    if (_activeSubView) {
        // The sub-view gets first refusal so its own back-handling (the
        // Diagnostics page inside Config, for one) still works.
        if (_activeSubView->handleInput(keyboard)) return true;
        if (keyboard.wasBackspacePressed()) {
            _activeSubView->onExit();
            _activeSubView = nullptr;
            return true;
        }
        return false;
    }

    if (_items.empty()) return false;

    int direction = _scrollRepeater.update(keyboard);
    if (direction < 0 && _selectedIndex > 0) {
        _selectedIndex--;
        return true;
    }
    if (direction > 0 && _selectedIndex < (int)_items.size() - 1) {
        _selectedIndex++;
        return true;
    }

    if (keyboard.wasEnterPressed()) {
        _activeSubView = _items[_selectedIndex].view;
        _activeSubView->onEnter();
        return true;
    }

    return false;
}
