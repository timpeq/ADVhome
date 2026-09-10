#include "AppController.h"
#include "Graphics.h"
#include "TextScroller.h"
#include <M5Cardputer.h>

#ifndef ADVHOME_VERSION
#define ADVHOME_VERSION "dev"
#endif

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

    _config.flushPendingFavorites();
    
    _redraw = false;
    
    if (_currentState != _lastState) {
        _redraw = true;
        _lastState = _currentState;
    }
    
    checkPowerManagement();
    if (_powerState == PowerState::SOFT_SLEEP) {
        return; // Don't run the rest of the update loop
    }

    // Freeze the view while the sleep hold is counting. Letting it run means two
    // pushes per frame -- the view without the overlay, then the overlay -- and
    // the first of those is visible as a glitch. The canvas keeps the last frame
    // beneath, so the overlay just redraws over it.
    if (_escHoldActive) {
        drawSleepHoldOverlay();
        return;
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
    // Nothing is initialised yet at this point, so this is a plain state change
    // rather than the reboot the later screens need.
    if (_keyboard.wasEscPressed()) {
        _config.clearWifiConfig();
        WiFi.disconnect(true);
        _ssid = "";
        _password = "";
        _currentInput = "";
        _selectedNetwork = 0;
        _scrollOffset = 0;
        _currentState = AppState::SCANNING;
        _redraw = true;
        return;
    }

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

    if (handleConnectionRecoveryKeys(true)) return;

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
                MediaPlayerState ms;
                if (_entityManager.getMediaPlayerState(eId, ms)) {
                    _haManager->toggleMute(eId, !ms.isVolumeMuted);
                }
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
        
        auto onSetClimateTemp = [this](String entityId, float temp) {
            _haManager->setClimateTemperature(entityId, temp);
        };
        auto onSetClimateRange = [this](String entityId, float low, float high) {
            _haManager->setClimateTempRange(entityId, low, high);
        };
        auto onSetHvacMode = [this](String entityId, String mode) {
            _haManager->setHvacMode(entityId, mode);
        };
        
        _detailView = new EntityDetailView(_entityManager, _config, onBack, onCallService, onSetVolume, onSeekMedia, onSecureService, onSetClimateTemp, onSetClimateRange, onSetHvacMode);
        
        _aboutView = new AboutView(_config);
        _helpView = new HelpView(_config);

        _wifiView = new NetworkView(
            "Wi-Fi", "Forget network and rescan",
            [this](std::vector<String>& out) {
                out.push_back("SSID: " + _config.getWifiSSID());
                out.push_back("IP:   " + _wifi.getIPAddress());
                out.push_back("RSSI: " + String(WiFi.RSSI()) + " dBm");
            },
            [this]() {
                _config.clearWifiConfig();
                rebootWithMessage("Forgetting network...");
            });

        _haView = new NetworkView(
            "Home Assistant", "Clear server and token",
            [this](std::vector<String>& out) {
                out.push_back("URL: " + _config.getHAUrl());
                out.push_back("Ver: " + _haManager->getVersion());
                out.push_back("Setup runs again on reboot.");
            },
            [this]() {
                _config.clearHAConfig();
                rebootWithMessage("Clearing HA setup...");
            });

        // The connection pages are settings, not a separate destination, so
        // they sit in the Settings list next to brightness and volume rather
        // than as their own Menu entries.
        _configView->setConnectionViews(_wifiView, _haView);

        _menuView = new MenuView(_config, ADVHOME_VERSION);
        _menuView->addItem("Settings", _configView);
        _menuView->addItem("Help & Shortcuts", _helpView);
        _menuView->addItem("About & License", _aboutView);

        _tabController.addView(_homeView, "Home");
        _tabController.addView(_chatView, "Chat");
        _tabController.setViewVisible(_chatView, _config.getShowChat());
        _tabController.addView(_entitiesView, "Entities");
        _tabController.addView(_menuView, "Menu");
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
        // Input is otherwise ignored while disconnected, so this is the only way
        // back out if the network or the server has moved.
        if (handleConnectionRecoveryKeys(true)) return;

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
            
            String waiting = "";
            for (int i = 0; i < dots; i++) waiting += ".";
            
            if (isWifiDisc) {
                String msg = "Connecting to " + _config.getWifiSSID() + waiting;
                String line1 = TextScroller::visible(msg, 32);
                _display.drawModalMessage("ADVhome", line1, "", TFT_YELLOW, TFT_CYAN,
                                          recoveryHint());
            } else {
                String line1 = "Connected to " + _config.getWifiSSID();
                String msg2 = "Connecting to " + _config.getHAUrl() + waiting;
                String line2 = TextScroller::visible(msg2, 32);
                _display.drawModalMessage("ADVhome", line1, line2, TFT_GREEN, TFT_CYAN,
                                          recoveryHint());
            }
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
            String waiting = "";
            for (int i = 0; i < dots; i++) waiting += ".";
            _display.clear();
            String displaySsid = _ssid.isEmpty() ? _config.getWifiSSID() : _ssid;
            String msg = "Connecting to " + displaySsid + waiting;
            String line1 = TextScroller::visible(msg, 32);
            _display.drawModalMessage("ADVhome", line1, "", TFT_YELLOW, TFT_CYAN,
                                      "ESC: pick another network");
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
            String hawaiting = "";
            for (int i = 0; i < hadots; i++) hawaiting += ".";
            _display.clear();
            String line1 = "Connected to " + _config.getWifiSSID();
            String msg2 = "Connecting to " + _config.getHAUrl() + hawaiting;
            String line2 = TextScroller::visible(msg2, 32);
            _display.drawModalMessage("ADVhome", line1, line2, TFT_GREEN, TFT_CYAN,
                                      recoveryHint());
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
            // setBrightness(0) alone leaves the panel controller running; the
            // deeper states put it into sleep-in, so it has to be woken.
            M5.Display.wakeup();
            WiFi.setSleep(WIFI_PS_MIN_MODEM);
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
    static bool escHintShown = false;
    bool forceDeepSleep = false;
    
    if (_config.getEscDeepSleep()) {
        if (_keyboard.isEscHeld()) {
            if (escHoldStartTime == 0) {
                escHoldStartTime = now;
                escHintShown = false;
            } else if (now - escHoldStartTime > 1000) {
                forceDeepSleep = true;
            } else if (now - escHoldStartTime > 300) {
                _escHoldActive = true;
                _escHoldMs = now - escHoldStartTime;
                escHintShown = true;
            }
        } else {
            if (escHintShown) _redraw = true;  // repaint over an abandoned hold
            escHoldStartTime = 0;
            escHintShown = false;
            _escHoldActive = false;
        }
    }
    
    bool autoSleep = _config.getDeepSleepMode() != 0 &&
                     idleTime >= (uint32_t)_config.getDeepSleepTimeout();
    
    if (forceDeepSleep || autoSleep) {
        escHintShown = false;

        // Wait for all keys to be released before sleeping to prevent immediate
        // wakeup, keeping the same overlay on screen and switching it to its
        // completed state rather than swapping in another full-screen message.
        if (forceDeepSleep) {
            _escHoldActive = true;
            _escHoldMs = 1000;
        }
        while (M5Cardputer.Keyboard.isPressed()) {
            M5Cardputer.update();
            if (forceDeepSleep) drawSleepHoldOverlay();
            delay(10);
        }
        _escHoldActive = false;

        // Releasing the key is not enough on the ADV. The TCA8418 holds its INT
        // line low until its event FIFO is empty, and the reader drains exactly
        // one event per update(), only clearing INT_STAT once nothing is left.
        // The release event is therefore still queued here, so arming a
        // level-triggered wake on GPIO11 would fire on an already-low pin and
        // wake the device immediately. Drain until the line actually rises.
        if (M5.getBoard() == m5::board_t::board_M5CardputerADV) {
            uint32_t drainStart = millis();
            while (digitalRead(11) == LOW && millis() - drainStart < 500) {
                M5Cardputer.update();
                delay(5);
            }
        }

        // Deep sleep - CPU halts until keypress
        WiFi.disconnect(true);
        releaseAudio();
        M5.Display.sleep();
        delay(100);

        const bool isAdv = (M5.getBoard() == m5::board_t::board_M5CardputerADV);
        // "Deep Sleep" chooses the depth, "Wake On GO Only" the wake source.
        // Off still honours a held ESC, as a light sleep, so the manual gesture
        // never becomes a no-op.
        int mode = _config.getDeepSleepMode();
        if (mode == 0) mode = 1;
        const bool keyboardWakes = !_config.getWakeOnGoOnly();

        // GO is GPIO0, active low, and RTC-capable, so it is a valid wake source
        // at either sleep depth.
        uint64_t wakeMask = (1ULL << 0);

        const int input_list[] = {13, 15, 3, 4, 5, 6, 7};
        const int output_list[] = {8, 9, 11};

#if defined(CONFIG_IDF_TARGET_ESP32S3)
        if (keyboardWakes) {
            if (isAdv) {
                // The ADV's TCA8418 raises one interrupt line for any key.
                pinMode(11, INPUT_PULLUP);
                gpio_wakeup_enable((gpio_num_t)11, GPIO_INTR_LOW_LEVEL);
                wakeMask |= (1ULL << 11);
            } else {
                // The original Cardputer is a matrix: hold the rows low so a
                // press pulls a column down.
                for (int i = 0; i < 3; i++) {
                    pinMode(output_list[i], OUTPUT);
                    digitalWrite(output_list[i], LOW);
                }
                for (int i = 0; i < 7; i++) {
                    pinMode(input_list[i], INPUT_PULLUP);
                    gpio_wakeup_enable((gpio_num_t)input_list[i], GPIO_INTR_LOW_LEVEL);
                    wakeMask |= (1ULL << input_list[i]);
                }
            }
        }
#endif

        if (mode == 2) {
            // True deep sleep: the digital core powers down and the chip resets
            // on wake, so this costs a full boot and Home Assistant reconnect.
            // Every pin used here is within GPIO0-21 and therefore RTC-capable,
            // which is what ext1 requires.
            while (M5Cardputer.BtnA.isPressed()) {
                M5Cardputer.update();
                delay(10);
            }
#if defined(CONFIG_IDF_TARGET_ESP32S3)
            if (keyboardWakes && !isAdv) {
                // Matrix rows are driven by the digital core, which is about to
                // lose power. Latch them low for the duration of the sleep.
                for (int i = 0; i < 3; i++) {
                    gpio_hold_en((gpio_num_t)output_list[i]);
                }
                gpio_deep_sleep_hold_en();
            }
            esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_LOW);
#endif
            esp_deep_sleep_start();
            // Never returns; the device reboots into setup().
        }

#if defined(CONFIG_IDF_TARGET_ESP32S3)
        esp_sleep_enable_gpio_wakeup();
#endif

        M5.Power.lightSleep(M5.Power.sleep_no_timer, true); // true = wake from wakeup pin

        // On wake:
        _lastActivityTime = millis();
        _powerState = PowerState::NORMAL;
        M5.Display.wakeup();
        M5.Display.setBrightness(_config.getDisplayBrightness());
        _wifi.connectTo(_ssid, _password);
        return;
    }
    
    if (_powerState != PowerState::SOFT_SLEEP && idleTime >= (uint32_t)_config.getSoftSleepTimeout()) {
        _powerState = PowerState::SOFT_SLEEP;
        M5.Display.sleep();
        releaseAudio();
        WiFi.disconnect(true); // Drop connection
        setCpuFrequencyMhz(80); // Save power
        return;
    }
    
    if (_powerState != PowerState::DISPLAY_OFF && _powerState != PowerState::SOFT_SLEEP && idleTime >= (uint32_t)_config.getDisplayOffTimeout()) {
        _powerState = PowerState::DISPLAY_OFF;
        // Nobody is looking, so let the radio doze longer between beacons. The
        // websocket stays up; state pushes just arrive less promptly.
        M5.Display.sleep();
        releaseAudio();
        WiFi.setSleep(WIFI_PS_MAX_MODEM);
    } else if (_powerState == PowerState::NORMAL && idleTime >= (uint32_t)_config.getDimTimeout()) {
        _powerState = PowerState::DIM;
        M5.Display.setBrightness(10);
    }
}

void AppController::rebootWithMessage(const char* message) {
    _display.drawMessage("ADVhome", message, TFT_YELLOW);
    delay(1200);
    ESP.restart();
}

bool AppController::handleConnectionRecoveryKeys(bool allowHaReset) {
    // Reachable from any screen that can strand the user: if the saved network
    // is gone or the Home Assistant URL is wrong, the device would otherwise
    // retry forever with no way in. Both actions are destructive, so the first
    // press only arms them.
    if (_recoveryArmed != 0 && millis() - _recoveryArmedAt > 8000) {
        _recoveryArmed = 0;
        _redraw = true;
    }

    uint8_t requested = 0;
    if (_keyboard.wasEscPressed()) {
        requested = 1;
    } else if (allowHaReset) {
        for (char ch : _keyboard.getNewChars()) {
            if (ch == 'h' || ch == 'H') {
                requested = 2;
                break;
            }
        }
    }

    if (requested == 0) return false;

    if (_recoveryArmed != requested) {
        _recoveryArmed = requested;
        _recoveryArmedAt = millis();
        _redraw = true;
        return false;
    }

    _recoveryArmed = 0;
    if (requested == 1) {
        _config.clearWifiConfig();
        rebootWithMessage("Forgetting network...");
    } else {
        _config.clearHAConfig();
        rebootWithMessage("Clearing HA setup...");
    }
    return true;
}

String AppController::recoveryHint() const {
    if (_recoveryArmed == 1) return "ESC again: forget Wi-Fi";
    if (_recoveryArmed == 2) return "H again: clear HA setup";
    return "ESC: Wi-Fi   H: HA setup";
}

void AppController::releaseAudio() {
    // The ES8311 stays powered for as long as the speaker task runs, and the
    // TTS volume preview in Settings can leave it running. Never touch the mic
    // here: ending it while the speaker runs hangs the shared I2S bus.
    if (M5.Speaker.isRunning() && M5.Speaker.isPlaying() == 0) {
        M5.Speaker.end();
    }
}

void AppController::drawSleepHoldOverlay() {
    // A held key produces no edges, so without this the hold is silent until it
    // takes effect. Drawn as the last thing each frame so the active view's own
    // repaint cannot land on top of it.
    auto canvas = _display.getCanvas();
    canvas->fillRect(30, 45, 180, 45, TFT_BLACK);
    canvas->drawRect(30, 45, 180, 45, TFT_YELLOW);

    canvas->setTextSize(1);
    bool ready = _escHoldMs >= 1000;
    canvas->setTextColor(TFT_YELLOW);
    canvas->setCursor(ready ? 60 : 42, 56);
    canvas->print(ready ? "Release to sleep" : "Keep holding to sleep");

    int pct = (int)((_escHoldMs * 100) / 1000);
    if (pct > 100) pct = 100;
    canvas->fillRect(42, 72, 156, 6, 0x2124);
    canvas->fillRect(42, 72, (156 * pct) / 100, 6, TFT_YELLOW);
    _display.push();
}
