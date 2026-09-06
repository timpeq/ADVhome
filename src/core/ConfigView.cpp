#include "ConfigView.h"

ConfigView::ConfigView(ConfigManager& config) : _config(config) {
    _settings.push_back({"Battery % in Tab Bar", 0});
    _settings.push_back({"Reconnect Interval", 1});
    _settings.push_back({"Back Button", 2});
    _settings.push_back({"Scroll Style", 3});
}

void ConfigView::refreshValues() {
    _showBattery = _config.getShowBattery();
    _reconInt = _config.getReconnectInterval();
    _backStyle = _config.getBackButtonStyle();
    _scrollStyle = _config.getScrollStyle();
}

void ConfigView::onEnter() {
    refreshValues();
    _selectedIndex = 0;
}

void ConfigView::draw(DisplayManager& display) {
    auto canvas = display.getCanvas();
    
    int y = 25;
    for (size_t i = 0; i < _settings.size(); i++) {
        if (i == (size_t)_selectedIndex) {
            canvas->fillRect(0, y + (i * 20) - 2, 240, 20, TFT_BLUE);
            canvas->setTextColor(TFT_WHITE);
        } else {
            canvas->setTextColor(TFT_LIGHTGREY);
        }
        
        canvas->setTextSize(1);
        canvas->setCursor(5, y + (i * 20));
        canvas->print(_settings[i].name);
        
        // Draw Value
        canvas->setCursor(150, y + (i * 20));
        if (_settings[i].type == 0) {
            canvas->print(_showBattery ? "ON" : "OFF");
            if (_showBattery) canvas->setTextColor(TFT_GREEN);
            else canvas->setTextColor(TFT_RED);
        } else if (_settings[i].type == 1) {
            canvas->print(String(_reconInt / 1000) + " sec");
            canvas->setTextColor(TFT_CYAN);
        } else if (_settings[i].type == 2) {
            if (_backStyle == 0) canvas->print("Both");
            else if (_backStyle == 1) canvas->print("<- Arrow");
            else if (_backStyle == 2) canvas->print("ESC / `");
            canvas->setTextColor(TFT_YELLOW);
        } else if (_settings[i].type == 3) {
            if (_scrollStyle == 0) canvas->print("1 Item");
            else if (_scrollStyle == 1) canvas->print("Page Jump");
            canvas->setTextColor(TFT_YELLOW);
        }
    }
}

void ConfigView::toggleCurrent() {
    if (_selectedIndex < 0 || _selectedIndex >= _settings.size()) return;
    
    if (_settings[_selectedIndex].type == 0) {
        _showBattery = !_showBattery;
        _config.setShowBattery(_showBattery);
    } else if (_settings[_selectedIndex].type == 1) {
        _reconInt += 1000;
        if (_reconInt > 30000) _reconInt = 1000;
        _config.setReconnectInterval(_reconInt);
    } else if (_settings[_selectedIndex].type == 2) {
        _backStyle++;
        if (_backStyle > 2) _backStyle = 0;
        _config.setBackButtonStyle(_backStyle);
    } else if (_settings[_selectedIndex].type == 3) {
        _scrollStyle++;
        if (_scrollStyle > 1) _scrollStyle = 0;
        _config.setScrollStyle(_scrollStyle);
    }
}

bool ConfigView::handleInput(KeyboardManager& keyboard) {
    bool handled = false;
    
    if (keyboard.wasUpPressed()) {
        if (_selectedIndex > 0) _selectedIndex--;
        handled = true;
    }
    
    if (keyboard.wasDownPressed()) {
        if (_selectedIndex < _settings.size() - 1) _selectedIndex++;
        handled = true;
    }
    
    if (keyboard.wasEnterPressed() || keyboard.wasRightPressed()) {
        toggleCurrent();
        handled = true;
    }
    
    return handled;
}
