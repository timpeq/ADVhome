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
    _keyboard.setGoActsAsEnter(!_config.getGoButtonToChat());
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
    
    checkPowerManagement();
    if (_powerState == PowerState::SOFT_SLEEP) {
        return; // Don't run the rest of the update loop
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
    bool isWifiDisc = !_wifi.isConnected();
    bool isHADisc = !isWifiDisc && (_haManager && !_haManager->isConnected() && !_haManager->isTtsTransitioning());
    bool isDisconnected = isWifiDisc || isHADisc;
    
    
    if (!_diagView) {
        _diagView = new DiagnosticView(_wifi, *_haManager);
        
        auto onSelect = [this](String entityId) {
            _detailView->setEntityId(entityId);
            _isDetailViewActive = true;
            _redraw = true;
        };
        auto onToggle = [this](String entityId) {
            Entity entity = _entityManager.getEntity(entityId);
            if (entity.id != "") {
                _haManager->callService(entity.domain, "toggle", entityId);
            }
        };
        
        auto onAdjust = [this](String entityId, int dir) {
            _haManager->adjustEntity(entityId, dir);
            _redraw = true;
        };
        
        _entitiesView = new EntitiesView(_entityManager, _config, onSelect, onToggle, onAdjust);
        _homeView = new HomeView(_entityManager, _config, onSelect, onToggle, onAdjust);
        _configView = new ConfigView(_config, *_diagView, *_haManager, [this]() {
            _tabController.setViewVisible(_chatView, _config.getShowChat());
            _redraw = true;
        });
        _chatView = new ChatView(*_haManager);
        _haManager->setConversationCallback([this](const String& response) {
            _chatView->receiveResponse(response);
            _redraw = true;
        });
        _haManager->setVoiceCallback([this](const String& event) {
            _chatView->receiveVoiceEvent(event);
            _redraw = true;
        });
        _haManager->setPipelinesCallback([this]() {
            _redraw = true;
        });
        
        auto onBack = [this]() {
            _isDetailViewActive = false;
            _redraw = true;
        };
        
        auto onCallService = [this](String domain, String service) {
            String eId = _detailView->getEntityId();
            if (service == "volume_mute" && domain == "media_player") {
                Entity e = _entityManager.getEntity(eId);
                _haManager->toggleMute(eId, !e.isVolumeMuted);
            } else {
                _haManager->callService(domain, service, eId);
            }
        };

        auto onSetVolume = [this](String entityId, float volume) {
            _haManager->setMediaVolume(entityId, volume);
        };

        auto onSecureService = [this](String domain, String service, String entityId, String code) {
            _haManager->callSecureService(domain, service, entityId, code);
        };
        
        auto onSeekMedia = [this](String entityId, float position) {
            _haManager->seekMedia(entityId, position);
        };
        
        _detailView = new EntityDetailView(_entityManager, _config, onBack, onCallService, onSetVolume, onSeekMedia, onSecureService);
        
        _tabController.addView(_homeView, "Home");
        _tabController.addView(_chatView, "Chat");
        _tabController.setViewVisible(_chatView, _config.getShowChat());
        _tabController.addView(_entitiesView, "Entities");
        _tabController.addView(_configView, "Config");
    }
    
    bool wasDetailActive = _isDetailViewActive;

    // The GO (top) button jumps straight to the Chat tab from anywhere.
    if (_config.getGoButtonToChat() && _config.getShowChat() && _keyboard.wasGoPressed()) {
        _isDetailViewActive = false;
        wasDetailActive = false;
        _tabController.showView(_chatView);
        _redraw = true;
    }

    static uint32_t lastMarquee = 0;
    if (millis() - lastMarquee > 250) {
        lastMarquee = millis();
        _redraw = true;
    }
    
    if (!isDisconnected) {
        if (wasDetailActive) {
            if (_detailView->handleInput(_keyboard)) {
                _redraw = true;
            }
        } else {
            _tabController.update(_keyboard, _display, _redraw, _config.getShowBattery());
            if (!_isDetailViewActive) {
                _redraw = false;
            }
        }
    }
    
    if (isDisconnected) {
        static uint32_t lastDot = 0;
        static int dots = 0;
        if (millis() - lastDot > 500) {
            lastDot = millis();
            dots = (dots + 1) % 4;
            _redraw = true;
        }
        if (_redraw) {
            _display.clear();
            _tabController.drawTabBar(_display, _config.getShowBattery());
            _tabController.drawActiveView(_display);
            if (_isDetailViewActive) _detailView->draw(_display);
            
            String waiting = "Waiting";
            for (int i = 0; i < dots; i++) waiting += ".";
            
            if (isWifiDisc) {
                _display.drawMessage("WiFi Disconnected", waiting, TFT_YELLOW);
            } else {
                _display.drawMessage("HA Disconnected", waiting, TFT_CYAN);
            }
            _display.push();
            _redraw = false;
        }
    } else {
        // Draw Detail View if active and needs redraw
        if (_isDetailViewActive) {
            if (_redraw) {
                _display.clear();
                _tabController.drawTabBar(_display, _config.getShowBattery());
                _tabController.drawActiveView(_display); // Render the list beneath
                _detailView->draw(_display);
                _display.push();
                _redraw = false;
            }
        } else if (wasDetailActive) {
            // We just exited detail view. Force a redraw of tabs on the NEXT frame.
            _redraw = true;
        }
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

void AppController::checkPowerManagement() {
    bool activity = _keyboard.hasActivity();
    
    // Check IMU for movement
    if (!activity && M5.Imu.isEnabled()) {
        float gx, gy, gz;
        M5.Imu.update();
        M5.Imu.getGyroData(&gx, &gy, &gz);
        if (abs(gx) + abs(gy) + abs(gz) > 100.0f) { // Threshold for movement
            activity = true;
        }
    }
    
    uint32_t now = millis();
    
    if (activity) {
        _lastActivityTime = now;
        if (_powerState != PowerState::NORMAL) {
            _powerState = PowerState::NORMAL;
            setCpuFrequencyMhz(240);
            if (!_wifi.isConnected()) {
                _wifi.connectTo(_ssid, _password);
            }
        }
    }
    if (_powerState == PowerState::NORMAL) {
        // Always set brightness on activity in case the user changed it in the menu
        M5.Display.setBrightness(_config.getDisplayBrightness());
    }
    
    uint32_t idleTime = (now - _lastActivityTime) / 1000; // in seconds
    
    static uint32_t escHoldStartTime = 0;
    bool forceDeepSleep = false;
    
    if (_config.getEscDeepSleep()) {
        if (_keyboard.isEscHeld()) {
            if (escHoldStartTime == 0) escHoldStartTime = now;
            else if (now - escHoldStartTime > 1000) forceDeepSleep = true;
        } else {
            escHoldStartTime = 0;
        }
    }
    
    if (forceDeepSleep || idleTime >= (uint32_t)_config.getDeepSleepTimeout()) {
        // Wait for all keys to be released before sleeping to prevent immediate wakeup
        while (M5Cardputer.Keyboard.isPressed()) {
            M5Cardputer.update();
            delay(10);
        }

        // Deep sleep - CPU halts until keypress
        WiFi.disconnect(true);
        M5.Display.setBrightness(0);
        delay(100);

#if defined(CONFIG_IDF_TARGET_ESP32S3)
        if (M5.getBoard() == m5::board_t::board_M5CardputerADV) {
            // Setup Cardputer ADV wakeup (TCA8418 INT pin)
            pinMode(11, INPUT_PULLUP);
            gpio_wakeup_enable((gpio_num_t)11, GPIO_INTR_LOW_LEVEL);
        } else {
            // Setup standard Cardputer matrix wakeup
            const int input_list[] = {13, 15, 3, 4, 5, 6, 7};
            const int output_list[] = {8, 9, 11};
            for (int i = 0; i < 3; i++) {
                pinMode(output_list[i], OUTPUT);
                digitalWrite(output_list[i], LOW);
            }
            for (int i = 0; i < 7; i++) {
                pinMode(input_list[i], INPUT_PULLUP);
                gpio_wakeup_enable((gpio_num_t)input_list[i], GPIO_INTR_LOW_LEVEL);
            }
        }
        esp_sleep_enable_gpio_wakeup();
#endif

        M5.Power.lightSleep(M5.Power.sleep_no_timer, true); // true = wake from wakeup pin
        // On wake:
        _lastActivityTime = millis();
        _powerState = PowerState::NORMAL;
        M5.Display.setBrightness(_config.getDisplayBrightness());
        _wifi.connectTo(_ssid, _password);
        return;
    }
    
    if (_powerState != PowerState::SOFT_SLEEP && idleTime >= (uint32_t)_config.getSoftSleepTimeout()) {
        _powerState = PowerState::SOFT_SLEEP;
        M5.Display.setBrightness(0);
        WiFi.disconnect(true); // Drop connection
        setCpuFrequencyMhz(80); // Save power
        return;
    }
    
    if (_powerState != PowerState::DISPLAY_OFF && _powerState != PowerState::SOFT_SLEEP && idleTime >= (uint32_t)_config.getDisplayOffTimeout()) {
        _powerState = PowerState::DISPLAY_OFF;
        M5.Display.setBrightness(0);
    } else if (_powerState == PowerState::NORMAL && idleTime >= (uint32_t)_config.getDimTimeout()) {
        _powerState = PowerState::DIM;
        M5.Display.setBrightness(10);
    }
}
