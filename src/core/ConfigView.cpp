#include "ConfigView.h"

ConfigView::ConfigView(ConfigManager& config, DiagnosticView& diagnosticView)
    : _config(config), _diagnosticView(diagnosticView), _scrollRepeater(config) {
    _settings.push_back({"Battery % in Tab Bar", 0});
    _settings.push_back({"Hide Unavailable on Home", 9});
    _settings.push_back({"Favorites Sort", 2});
    _settings.push_back({"Reconnect Interval", 1});
    _settings.push_back({"Scroll Start Delay", 5});
    _settings.push_back({"Scroll Repeat", 6});
    _settings.push_back({"Seek Step (Min)", 7});
    _settings.push_back({"Seek Step (Max)", 8});
    _settings.push_back({"Brightness", 10});
    _settings.push_back({"Dim T/O", 11});
    _settings.push_back({"Disp Off T/O", 12});
    _settings.push_back({"Soft Sleep T/O", 13});
    _settings.push_back({"Deep Sleep T/O", 14});
    _settings.push_back({"ESC for Sleep", 15});
    _settings.push_back({"Diagnostics", 4});
}

void ConfigView::refreshValues() {
    _showBattery = _config.getShowBattery();
    _reconInt = _config.getReconnectInterval();
    _scrollDelay = _config.getScrollDelay();
    _scrollSpeed = _config.getScrollSpeed();
    _seekStep = _config.getSeekStep();
    _seekStepMax = _config.getSeekStepMax();
    _favoritesSort = _config.getFavoritesSort();
    _hideUnavailable = _config.getHideUnavailable();
    _brightness = _config.getDisplayBrightness();
    _dimTO = _config.getDimTimeout();
    _dispOffTO = _config.getDisplayOffTimeout();
    _softSleepTO = _config.getSoftSleepTimeout();
    _deepSleepTO = _config.getDeepSleepTimeout();
    _escDeepSleep = _config.getEscDeepSleep();
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
    for (size_t i = _scrollOffset; i < _settings.size(); i++) {
        int displayIndex = i - _scrollOffset;
        if (displayIndex >= 5) break; // max 5 items visible
        
        if (i == (size_t)_selectedIndex) {
            canvas->fillRect(0, y + (displayIndex * 20) - 2, 240, 20, TFT_BLUE);
            canvas->setTextColor(TFT_WHITE);
        } else {
            canvas->setTextColor(TFT_LIGHTGREY);
        }
        
        canvas->setTextSize(1);
        canvas->setCursor(5, y + (displayIndex * 20));
        canvas->print(_settings[i].name);
        
        // Draw Value
        canvas->setCursor(150, y + (displayIndex * 20));
        if (_settings[i].type == 0) {
            canvas->print(_showBattery ? "ON" : "OFF");
            if (_showBattery) canvas->setTextColor(TFT_GREEN);
            else canvas->setTextColor(TFT_RED);
        } else if (_settings[i].type == 1) {
            canvas->print(String(_reconInt / 1000) + " sec");
            canvas->setTextColor(TFT_GREEN);
        } else if (_settings[i].type == 9) {
            canvas->print(_hideUnavailable ? "YES" : "NO");
            canvas->setTextColor(_hideUnavailable ? TFT_GREEN : TFT_LIGHTGREY);
        } else if (_settings[i].type == 2) {
            canvas->print(_favoritesSort == 0 ? "ORDER" : "NAME");
            canvas->setTextColor(TFT_CYAN);
        } else if (_settings[i].type == 5) {
            canvas->print(String(_scrollDelay) + " ms");
            canvas->setTextColor(TFT_YELLOW);
        } else if (_settings[i].type == 6) {
            canvas->print(String(_scrollSpeed) + " ms");
            canvas->setTextColor(TFT_YELLOW);
        } else if (_settings[i].type == 7) {
            canvas->print(String(_seekStep) + " s");
            canvas->setTextColor(TFT_ORANGE);
        } else if (_settings[i].type == 8) {
            canvas->print(String(_seekStepMax) + " s");
            canvas->setTextColor(TFT_RED);
        } else if (_settings[i].type == 10) {
            canvas->print(String(_brightness));
            canvas->setTextColor(TFT_WHITE);
        } else if (_settings[i].type == 11) {
            canvas->print(String(_dimTO) + " s");
            canvas->setTextColor(TFT_YELLOW);
        } else if (_settings[i].type == 12) {
            canvas->print(String(_dispOffTO) + " s");
            canvas->setTextColor(TFT_ORANGE);
        } else if (_settings[i].type == 13) {
            canvas->print(String(_softSleepTO) + " s");
            canvas->setTextColor(TFT_GREEN);
        } else if (_settings[i].type == 14) {
            canvas->print(String(_deepSleepTO) + " s");
            canvas->setTextColor(TFT_PURPLE);
        } else if (_settings[i].type == 15) {
            canvas->print(_escDeepSleep ? "YES" : "NO");
            canvas->setTextColor(_escDeepSleep ? TFT_GREEN : TFT_LIGHTGREY);
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
        _favoritesSort = _favoritesSort == 0 ? 1 : 0;
        _config.setFavoritesSort(_favoritesSort);
    } else if (_settings[_selectedIndex].type == 5) {
        _scrollDelay += 100;
        if (_scrollDelay > 1000) _scrollDelay = 200;
        _config.setScrollDelay(_scrollDelay);
    } else if (_settings[_selectedIndex].type == 6) {
        _scrollSpeed -= 20;
        if (_scrollSpeed < 40) _scrollSpeed = 200;
        _config.setScrollSpeed(_scrollSpeed);
    } else if (_settings[_selectedIndex].type == 7) {
        if (_seekStep == 5) _seekStep = 10;
        else if (_seekStep == 10) _seekStep = 15;
        else if (_seekStep == 15) _seekStep = 30;
        else _seekStep = 5;
        if (_seekStep > _seekStepMax) _seekStepMax = _seekStep;
        _config.setSeekStep(_seekStep);
        _config.setSeekStepMax(_seekStepMax);
    } else if (_settings[_selectedIndex].type == 8) {
        if (_seekStepMax == 5) _seekStepMax = 10;
        else if (_seekStepMax == 10) _seekStepMax = 15;
        else if (_seekStepMax == 15) _seekStepMax = 30;
        else if (_seekStepMax == 30) _seekStepMax = 60;
        else _seekStepMax = 5;
        if (_seekStepMax < _seekStep) _seekStep = _seekStepMax;
        _config.setSeekStep(_seekStep);
        _config.setSeekStepMax(_seekStepMax);
    } else if (_settings[_selectedIndex].type == 9) {
        _hideUnavailable = !_hideUnavailable;
        _config.setHideUnavailable(_hideUnavailable);
    } else if (_settings[_selectedIndex].type == 10) {
        _brightness += 25;
        if (_brightness > 255) _brightness = 25;
        _config.setDisplayBrightness(_brightness);
    } else if (_settings[_selectedIndex].type == 11) {
        _dimTO += 10;
        if (_dimTO > 120) _dimTO = 10;
        _config.setDimTimeout(_dimTO);
    } else if (_settings[_selectedIndex].type == 12) {
        _dispOffTO += 30;
        if (_dispOffTO > 300) _dispOffTO = 30;
        _config.setDisplayOffTimeout(_dispOffTO);
    } else if (_settings[_selectedIndex].type == 13) {
        _softSleepTO += 30;
        if (_softSleepTO > 600) _softSleepTO = 60;
        _config.setSoftSleepTimeout(_softSleepTO);
    } else if (_settings[_selectedIndex].type == 14) {
        _deepSleepTO += 300; // 5 min steps
        if (_deepSleepTO > 7200) _deepSleepTO = 300;
        _config.setDeepSleepTimeout(_deepSleepTO);
    } else if (_settings[_selectedIndex].type == 15) {
        _escDeepSleep = !_escDeepSleep;
        _config.setEscDeepSleep(_escDeepSleep);
    }
}

bool ConfigView::handleInput(KeyboardManager& keyboard) {
    bool handled = false;

    if (_showDiagnostics) {
        if (keyboard.wasBackspacePressed()) {
            _showDiagnostics = false;
            refreshValues();
            handled = true;
        }
        return handled;
    }
    
    int direction = _scrollRepeater.update(keyboard);
    if (direction < 0 && _selectedIndex > 0) {
        _selectedIndex--;
        if (_selectedIndex < _scrollOffset) _scrollOffset = _selectedIndex;
        handled = true;
    } else if (direction > 0 && _selectedIndex < (int)_settings.size() - 1) {
        _selectedIndex++;
        if (_selectedIndex >= _scrollOffset + 5) _scrollOffset = _selectedIndex - 4;
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
