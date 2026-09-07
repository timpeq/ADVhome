#ifndef HOME_ASSISTANT_MANAGER_H
#define HOME_ASSISTANT_MANAGER_H

#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "ConfigManager.h"
#include "EntityManager.h"
#include <Arduino.h>

class HomeAssistantManager {
public:
    HomeAssistantManager(ConfigManager& config, EntityManager& entityManager);
    
    void begin();
    void update();
    
    void fetchInitialStates();
    
    void callService(const String& domain, const String& service, const String& entity_id);
    void setMediaVolume(const String& entity_id, float volume);
    
    bool isConnected() const { return _isConnected; }
    bool isAuthenticated() const { return _isAuthenticated; }
    String getVersion() const { return _haVersion; }
    
private:
    ConfigManager& _config;
    EntityManager& _entityManager;
    WebSocketsClient _ws;
    
    bool _isConnected = false;
    bool _isAuthenticated = false;
    String _haVersion = "Unknown";
    
    void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
};

#endif // HOME_ASSISTANT_MANAGER_H
