#include "TabController.h"
#include "Graphics.h"

void TabController::addView(View* view, const String& name) {
    _tabs.push_back({view, name, true});
}

void TabController::setViewVisible(View* view, bool visible) {
    for (auto& tab : _tabs) {
        if (tab.view == view) {
            tab.visible = visible;
            return;
        }
    }
}

void TabController::update(KeyboardManager& keyboard, DisplayManager& display, bool forceRedraw, bool showBattery) {
    if (_tabs.empty()) return;
    
    bool redraw = forceRedraw;
    
    if (keyboard.wasTabPressed()) {
        nextTab();
        redraw = true;
    }
    
    // Pass input to the active view
    if (_tabs[_currentTabIndex].view->handleInput(keyboard)) {
        redraw = true; // The view handled it and probably needs a redraw
    }
    
    if (redraw) {
        display.clear();
        drawTabBar(display, showBattery);
        _tabs[_currentTabIndex].view->draw(display);
        display.push();
    }
}

void TabController::nextTab() {
    if (_tabs.empty()) return;
    _tabs[_currentTabIndex].view->onExit();
    size_t startIndex = _currentTabIndex;
    do {
        _currentTabIndex = (_currentTabIndex + 1) % _tabs.size();
    } while (!_tabs[_currentTabIndex].visible && _currentTabIndex != startIndex);
    _tabs[_currentTabIndex].view->onEnter();
}

void TabController::showView(View* target) {
    for (size_t i = 0; i < _tabs.size(); i++) {
        if (_tabs[i].view != target) continue;
        if (!_tabs[i].visible || i == (size_t)_currentTabIndex) return;
        _tabs[_currentTabIndex].view->onExit();
        _currentTabIndex = i;
        _tabs[_currentTabIndex].view->onEnter();
        return;
    }
}

void TabController::drawActiveView(DisplayManager& display) {
    if (_tabs.empty()) return;
    _tabs[_currentTabIndex].view->draw(display);
}

void TabController::drawTabBar(DisplayManager& display, bool showBattery) {
    auto canvas = display.getCanvas();
    canvas->fillRect(0, 0, 240, 16, 0x18E3); // Dark greyish blue
    
    if (_tabs.size() == 0) return;
    
    size_t visibleCount = 0;
    for (const auto& tab : _tabs) if (tab.visible) visibleCount++;
    if (visibleCount == 0) return;

    int tabAreaWidth = showBattery ? 200 : 240;
    int tabWidth = tabAreaWidth / visibleCount;
    size_t visibleIndex = 0;
    
    for (size_t i = 0; i < _tabs.size(); i++) {
        if (!_tabs[i].visible) continue;
        if (i == (size_t)_currentTabIndex) {
            canvas->fillRect(visibleIndex * tabWidth, 0, tabWidth, 16, TFT_BLUE);
            canvas->setTextColor(TFT_WHITE);
        } else {
            canvas->setTextColor(TFT_LIGHTGREY);
        }
        canvas->setTextSize(1);
        
        if (_tabs[i].name == "Home") {
            Graphics::drawHomeIcon(*canvas, (visibleIndex * tabWidth) + (tabWidth / 2), 8, TFT_WHITE);
        } else if (_tabs[i].name == "Menu") {
            Graphics::drawMenuIcon(*canvas, (visibleIndex * tabWidth) + (tabWidth / 2), 8,
                                   i == (size_t)_currentTabIndex ? TFT_WHITE : TFT_LIGHTGREY);
        } else {
            // Center text roughly
            int textX = (visibleIndex * tabWidth) + (tabWidth / 2) - (_tabs[i].name.length() * 3);
            canvas->setCursor(textX, 4);
            canvas->print(_tabs[i].name);
        }
        visibleIndex++;
    }
    
    if (showBattery) {
        int batLevel = M5.Power.getBatteryLevel();
        canvas->fillRect(200, 0, 40, 16, 0x18E3);
        canvas->setTextColor(TFT_GREEN);
        canvas->setTextSize(1);
        canvas->setCursor(205, 4);
        canvas->print(String(batLevel) + "%");
    }
}
