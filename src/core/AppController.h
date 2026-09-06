#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include "KeyboardManager.h"
#include "DisplayManager.h"
#include "WifiConnectionManager.h"
#include "ConfigManager.h"
#include "SetupPortal.h"
#include "HomeAssistantManager.h"
#include <Arduino.h>

enum class AppState {
    SCANNING,
    SELECT_SSID,
    INPUT_PASSWORD,
    CONNECTING,
    CONNECTED,
    HA_SETUP,
    HA_CONNECTING,
    HA_CONNECTED
};

class AppController {
public:
    void begin();
    void update();
    
private:
    KeyboardManager _keyboard;
    DisplayManager _display;
    WifiConnectionManager _wifi;
    ConfigManager _config;
    SetupPortal* _setupPortal = nullptr;
    HomeAssistantManager* _haManager = nullptr;
    
    AppState _currentState = AppState::SCANNING;
    AppState _lastState = (AppState)-1;
    
    bool _redraw = false;
    
    // WiFi state
    int _selectedNetwork = 0;
    int _scrollOffset = 0;
    String _ssid = "";
    String _password = "";
    String _currentInput = "";
    
    // Update handlers
    void updateScanning();
    void updateSelectSSID();
    void updateInputPassword();
    void updateConnecting();
    void updateConnected();
    void updateHASetup();
    void updateHAConnecting();
    void updateHAConnected();
    
    // Draw handlers
    void drawCurrentState();
};

#endif // APP_CONTROLLER_H
