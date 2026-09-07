#include "ConfigView.h"

ConfigView::ConfigView(ConfigManager& config, DiagnosticView& diagnosticView)
    : _config(config), _diagnosticView(diagnosticView), _scrollRepeater(config) {
    _settings.push_back({"Battery % in Tab Bar", 0});
    _settings.push_back({"Reconnect Interval", 1});
    _settings.push_back({"Back Button", 2});
    _settings.push_back({"Scroll Start Delay", 5});
    _settings.push_back({"Scroll Repeat", 6});
    _settings.push_back({"Diagnostics", 4});
}

void ConfigView::refreshValues() {
    _showBattery = _config.getShowBattery();
    _reconInt = _config.getReconnectInterval();
    _backStyle = _config.getBackButtonStyle();
    _scrollDelay = _config.getScrollDelay();
    _scrollSpeed = _config.getScrollSpeed();
}

void ConfigView::onEnter() {
    refreshValues();
    _selectedIndex = 0;
}

void ConfigView::draw(DisplayManager& display) {
    auto canvas = display.getCanvas();

    if (_showDiagnostics) {
        _diagnosticView.draw(display);
        return;
    }
    
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
        } else if (_settings[i].type == 5) {
            canvas->print(String(_scrollDelay) + " ms");
            canvas->setTextColor(TFT_YELLOW);
        } else if (_settings[i].type == 6) {
            canvas->print(String(_scrollSpeed) + " ms");
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
    } else if (_settings[_selectedIndex].type == 5) {
        _scrollDelay += 100;
        if (_scrollDelay > 1000) _scrollDelay = 200;
        _config.setScrollDelay(_scrollDelay);
    } else if (_settings[_selectedIndex].type == 6) {
        _scrollSpeed -= 20;
        if (_scrollSpeed < 40) _scrollSpeed = 200;
        _config.setScrollSpeed(_scrollSpeed);
    }
}

bool ConfigView::handleInput(KeyboardManager& keyboard) {
    bool handled = false;

    if (_showDiagnostics) {
        if (keyboard.wasBackspacePressed() || keyboard.wasLeftPressed()) {
            _showDiagnostics = false;
            refreshValues();
            handled = true;
        }
        return handled;
    }
    
    int direction = _scrollRepeater.update(keyboard);
    if (direction < 0 && _selectedIndex > 0) {
        _selectedIndex--;
        handled = true;
    } else if (direction > 0 && _selectedIndex < (int)_settings.size() - 1) {
        _selectedIndex++;
        handled = true;
    }
    
    if (keyboard.wasEnterPressed() || keyboard.wasRightPressed()) {
        if (_settings[_selectedIndex].type == 4) {
            _showDiagnostics = true;
        } else {
            toggleCurrent();
        }
        handled = true;
    }
    
    return handled;
}
