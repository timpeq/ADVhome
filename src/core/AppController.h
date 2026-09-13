#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include "KeyboardManager.h"
#include "DisplayManager.h"
#include "WifiConnectionManager.h"
#include "ConfigManager.h"
#include "SetupPortal.h"
#include "HomeAssistantManager.h"
#include "EntityManager.h"
#include "TabController.h"
#include "ConfigView.h"
#include "DiagnosticView.h"
#include "EntitiesView.h"
#include "HomeView.h"
#include "EntityDetailView.h"
#include "ChatView.h"
#include "MenuView.h"
#include "AboutView.h"
#include "HelpView.h"
#include "NetworkView.h"
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
    EntityManager _entityManager;
    SetupPortal* _setupPortal = nullptr;
    HomeAssistantManager* _haManager = nullptr;
    
    TabController _tabController;
    DiagnosticView* _diagView = nullptr;
    EntitiesView* _entitiesView = nullptr;
    HomeView* _homeView = nullptr;
    ConfigView* _configView = nullptr;
    ChatView* _chatView = nullptr;
    MenuView* _menuView = nullptr;
    AboutView* _aboutView = nullptr;
    HelpView* _helpView = nullptr;
    NetworkView* _wifiView = nullptr;
    NetworkView* _haView = nullptr;
    EntityDetailView* _detailView = nullptr;
    
    bool _isDetailViewActive = false;
    
    AppState _currentState = AppState::SCANNING;
    AppState _lastState = (AppState)-1;
    
    bool _redraw = false;
    
    // WiFi state
    int _selectedNetwork = 0;
    int _scrollOffset = 0;
    String _ssid = "";
    String _password = "";
    String _currentInput = "";
    
    uint32_t _lastActivityTime = 0;
    enum class PowerState {
        NORMAL,
        DIM,
        DISPLAY_OFF,
        SOFT_SLEEP
    };
    PowerState _powerState = PowerState::NORMAL;
    
    // Update handlers
    void updateScanning();
    void updateSelectSSID();
    void updateInputPassword();
    void updateConnecting();
    void updateConnected();
    void updateHASetup();
    void updateHAConnecting();
    void buildViews();
    void updateHAConnected();
    void checkPowerManagement();
    void releaseAudio();
    void drawSleepHoldOverlay();

    // Set while ESC is held toward the sleep threshold. The overlay is painted
    // after the active view, which would otherwise repaint over it.
    bool _escHoldActive = false;
    uint32_t _escHoldMs = 0;

    // Credentials are re-read only at boot, so every reconfiguration action
    // clears the relevant keys and restarts rather than trying to tear down a
    // live HomeAssistantManager that the views still hold references to.
    void rebootWithMessage(const char* message);
    bool handleConnectionRecoveryKeys(bool allowHaReset);
    String recoveryHint() const;

    // Recovery keys are armed by the first press and act on the second, so a
    // stray keystroke on a reconnect screen cannot wipe credentials.
    uint8_t _recoveryArmed = 0;  // 0 = none, 1 = Wi-Fi, 2 = Home Assistant
    uint32_t _recoveryArmedAt = 0;
    
    // When the WebSocket was opened; 0 until the state download has finished.
    uint32_t _haSocketStartedMs = 0;

    // Draw handlers
    void drawCurrentState();
    void drawLoadingHome();
};

#endif // APP_CONTROLLER_H
