#ifndef SETUP_PORTAL_H
#define SETUP_PORTAL_H

#include <WebServer.h>
#include "ConfigManager.h"
#include <Arduino.h>

class SetupPortal {
public:
    SetupPortal(ConfigManager& config);
    
    void begin();
    void update();
    void stop();
    
    bool isComplete() const { return _setupComplete; }
    
private:
    ConfigManager& _config;
    WebServer _server;
    bool _setupComplete = false;
    
    void handleRoot();
    void handleSave();
};

#endif // SETUP_PORTAL_H
