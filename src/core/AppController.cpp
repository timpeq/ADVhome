#include "AppController.h"

void AppController::begin() {
    _display.begin();
    _config.begin();
    _wifi.begin();
    
    if (_config.hasWifiConfig()) {
        _ssid = _config.getWifiSSID();
        _password = _config.getWifiPassword();
        _wifi.connectTo(_ssid, _password);
        _currentState = AppState::CONNECTING;
    } else {
        _currentState = AppState::SCANNING;
    }
    
    _redraw = true;
}

void AppController::update() {
    _keyboard.update();
    _wifi.update();
    
    if (_setupPortal) {
        _setupPortal->update();
    }
    
    if (_haManager) {
        _haManager->update();
    }
    
    _redraw = false;
    
    if (_currentState != _lastState) {
        _redraw = true;
        _lastState = _currentState;
    }
    
    switch (_currentState) {
        case AppState::SCANNING:
            updateScanning();
            break;
        case AppState::SELECT_SSID:
            updateSelectSSID();
            break;
        case AppState::INPUT_PASSWORD:
            updateInputPassword();
            break;
        case AppState::CONNECTING:
            updateConnecting();
            break;
        case AppState::CONNECTED:
            updateConnected();
            break;
        case AppState::HA_SETUP:
            updateHASetup();
            break;
        case AppState::HA_CONNECTING:
            updateHAConnecting();
            break;
        case AppState::HA_CONNECTED:
            updateHAConnected();
            break;
    }
    
    if (_redraw) {
        drawCurrentState();
    }
}

void AppController::updateScanning() {
    if (_redraw) {
        _display.drawMessage("Scanning WiFi...", "Please wait...", TFT_YELLOW);
        
        int numNetworks = _wifi.scanNetworks();
        if (numNetworks > 0) {
            _currentState = AppState::SELECT_SSID;
        } else {
            _display.drawMessage("Error", "No networks found!", TFT_RED);
            delay(2000);
            _currentState = AppState::SCANNING;
        }
    }
}

void AppController::updateSelectSSID() {
    auto new_chars = _keyboard.getNewChars();
    for (char i : new_chars) {
        if (i == 's' || i == '/' || i == 'j' || i == '.' || i == 'S' || i == 'J') {
            _selectedNetwork++;
            int max_net = _wifi.getNetworkList().size();
            if (_selectedNetwork >= max_net) _selectedNetwork = max_net - 1;
            if (_selectedNetwork >= _scrollOffset + 7) _scrollOffset++;
            _redraw = true;
        }
        if (i == 'w' || i == ';' || i == 'k' || i == ',' || i == 'W' || i == 'K') {
            _selectedNetwork--;
            if (_selectedNetwork < 0) _selectedNetwork = 0;
            if (_selectedNetwork < _scrollOffset) _scrollOffset--;
            _redraw = true;
        }
    }
    
    if (_keyboard.wasEnterPressed()) {
        _ssid = _wifi.getNetworkName(_selectedNetwork);
        _currentState = AppState::INPUT_PASSWORD;
        _currentInput = "";
        _redraw = true;
    }
}

void AppController::updateInputPassword() {
    auto new_chars = _keyboard.getNewChars();
    for (char i : new_chars) {
        _currentInput += i;
        _redraw = true;
    }
    
    if (_keyboard.wasBackspacePressed() && _currentInput.length() > 0) {
        _currentInput.remove(_currentInput.length() - 1);
        _redraw = true;
    }
    
    if (_keyboard.wasEnterPressed() && _currentInput.length() > 0) {
        _password = _currentInput;
        _currentInput = "";
        _currentState = AppState::CONNECTING;
        _wifi.connectTo(_ssid, _password);
        _redraw = true;
    }
    
    static uint32_t lastBlink = 0;
    if (millis() - lastBlink > 500) {
        _redraw = true;
        lastBlink = millis();
    }
}

void AppController::updateConnecting() {
    if (_wifi.isConnected()) {
        _config.saveWifiConfig(_ssid, _password);
        _currentState = AppState::CONNECTED;
    } else if (_wifi.hasConnectionFailed()) {
        _display.drawMessage("Connection Failed!", "Please check password.", TFT_RED);
        _config.clearWifiConfig();
        delay(2000);
        _currentState = AppState::SCANNING;
    }
    
    static uint32_t lastDot = 0;
    if (millis() - lastDot > 500) {
        lastDot = millis();
        _redraw = true;
    }
}

void AppController::updateConnected() {
    if (!_wifi.isConnected()) {
        _currentState = AppState::CONNECTING;
        return;
    }
    
    if (!_config.hasHAConfig()) {
        _currentState = AppState::HA_SETUP;
    } else {
        _currentState = AppState::HA_CONNECTING;
    }
}

void AppController::updateHASetup() {
    if (!_setupPortal) {
        _setupPortal = new SetupPortal(_config);
        _setupPortal->begin();
    }
    
    if (_setupPortal->isComplete()) {
        _setupPortal->stop();
        delete _setupPortal;
        _setupPortal = nullptr;
        
        _currentState = AppState::HA_CONNECTING;
    }
}

void AppController::updateHAConnecting() {
    if (!_wifi.isConnected()) {
        _currentState = AppState::CONNECTING;
        return;
    }

    if (!_haManager) {
        _haManager = new HomeAssistantManager(_config, _entityManager);
        _haManager->fetchInitialStates();
        _haManager->begin();
    }
    
    if (_haManager->isAuthenticated()) {
        _currentState = AppState::HA_CONNECTED;
    }
    
    static uint32_t lastDot = 0;
    if (millis() - lastDot > 500) {
        lastDot = millis();
        _redraw = true;
    }
}

void AppController::updateHAConnected() {
    if (!_wifi.isConnected()) {
        _currentState = AppState::CONNECTING;
        return;
    }
    
    if (_haManager && !_haManager->isConnected()) {
        _currentState = AppState::HA_CONNECTING;
        return;
    }
    
    if (!_diagView) {
        _diagView = new DiagnosticView(_wifi, *_haManager);
        
        auto onSelect = [this](String entityId) {
            _detailView->setEntityId(entityId);
            _isDetailViewActive = true;
            _redraw = true;
        };
        
        _entitiesView = new EntitiesView(_entityManager, _config, onSelect);
        _favoritesView = new FavoritesView(_entityManager, _config, onSelect);
        _configView = new ConfigView(_config);
        
        auto onBack = [this]() {
            _isDetailViewActive = false;
            _redraw = true;
        };
        
        auto onCallService = [this](String domain, String service) {
            _haManager->callService(domain, service, _detailView->getEntityId());
        };
        
        _detailView = new EntityDetailView(_entityManager, _config, onBack, onCallService);
        
        _tabController.addView(_favoritesView, "Favs");
        _tabController.addView(_entitiesView, "Entities");
        _tabController.addView(_configView, "Config");
        _tabController.addView(_diagView, "Diag");
    }
    
    bool wasDetailActive = _isDetailViewActive;
    
    static uint32_t lastMarquee = 0;
    if (millis() - lastMarquee > 250) {
        lastMarquee = millis();
        _redraw = true;
    }
    
    if (wasDetailActive) {
        if (_detailView->handleInput(_keyboard)) {
            _redraw = true;
        }
    } else {
        _tabController.update(_keyboard, _display, _redraw, _config.getShowBattery());
        // We only clear _redraw if we stayed in tab view. 
        // If an input caused us to switch to detail view, we leave _redraw = true.
        if (!_isDetailViewActive) {
            _redraw = false;
        }
    }
    
    // Draw Detail View if active and needs redraw
    if (_isDetailViewActive) {
        if (_redraw) {
            _display.clear();
            _detailView->draw(_display);
            _display.push();
            _redraw = false;
        }
    } else if (wasDetailActive) {
        // We just exited detail view. Force a redraw of tabs on the NEXT frame.
        _redraw = true;
    }
}

void AppController::drawCurrentState() {
    switch (_currentState) {
        case AppState::SCANNING:
            break;
            
        case AppState::SELECT_SSID:
            _display.drawMenu("Select WiFi (W/S):", _wifi.getNetworkList(), _selectedNetwork, _scrollOffset);
            break;
            
        case AppState::INPUT_PASSWORD:
            _display.drawPasswordInput("Enter Password:", _ssid, _currentInput);
            break;
            
        case AppState::CONNECTING: {
            static int dots = 0;
            if (_redraw) { 
                dots = (dots + 1) % 4;
            }
            String waiting = "Waiting";
            for (int i = 0; i < dots; i++) waiting += ".";
            _display.drawMessage("Connecting to:", _ssid + "\n\n" + waiting, TFT_YELLOW);
            break;
        }
            
        case AppState::CONNECTED:
            // Transient state
            break;
            
        case AppState::HA_SETUP:
            if (_redraw) {
                _display.drawHASetup(_wifi.getIPAddress());
            }
            break;
            
        case AppState::HA_CONNECTING: {
            static int hadots = 0;
            if (_redraw) { 
                hadots = (hadots + 1) % 4;
            }
            String hawaiting = "Connecting HA";
            for (int i = 0; i < hadots; i++) hawaiting += ".";
            _display.drawMessage("Home Assistant", hawaiting, TFT_CYAN);
            break;
        }
            
        case AppState::HA_CONNECTED:
            // Handled by TabController inside updateHAConnected()
            break;
    }
}
