#include "HomeAssistantManager.h"

HomeAssistantManager::HomeAssistantManager(ConfigManager& config) : _config(config) {}

void HomeAssistantManager::begin() {
    String url = _config.getHAUrl();
    
    // Parse URL (e.g., http://192.168.1.100:8123 or https://ha.domain.com)
    bool isSecure = url.startsWith("https");
    
    // Remove protocol
    int protocolEnd = url.indexOf("://");
    if (protocolEnd != -1) {
        url = url.substring(protocolEnd + 3);
    }
    
    // Extract host and port
    String host = url;
    int port = isSecure ? 443 : 80;
    
    int colonIdx = url.indexOf(':');
    int slashIdx = url.indexOf('/');
    
    if (colonIdx != -1 && (slashIdx == -1 || colonIdx < slashIdx)) {
        host = url.substring(0, colonIdx);
        String portStr = url.substring(colonIdx + 1, slashIdx != -1 ? slashIdx : url.length());
        port = portStr.toInt();
    } else if (slashIdx != -1) {
        host = url.substring(0, slashIdx);
    }
    
    _ws.onEvent(std::bind(&HomeAssistantManager::webSocketEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    
    if (isSecure) {
        _ws.beginSSL(host, port, "/api/websocket");
    } else {
        _ws.begin(host, port, "/api/websocket");
    }
    
    // Fast reconnect
    _ws.setReconnectInterval(5000);
}

void HomeAssistantManager::update() {
    _ws.loop();
}

void HomeAssistantManager::webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            _isConnected = false;
            _isAuthenticated = false;
            Serial.println("[HA] Disconnected!");
            break;
            
        case WStype_CONNECTED:
            _isConnected = true;
            Serial.println("[HA] Connected to websocket");
            break;
            
        case WStype_TEXT: {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);
            
            if (!error) {
                String msgType = doc["type"] | "";
                
                if (msgType == "auth_required") {
                    Serial.println("[HA] Auth required, sending token...");
                    String token = _config.getHAToken();
                    
                    JsonDocument authDoc;
                    authDoc["type"] = "auth";
                    authDoc["access_token"] = token;
                    
                    String authStr;
                    serializeJson(authDoc, authStr);
                    _ws.sendTXT(authStr);
                } 
                else if (msgType == "auth_ok") {
                    _isAuthenticated = true;
                    _haVersion = doc["ha_version"] | "Unknown";
                    Serial.println("[HA] Auth OK! Version: " + _haVersion);
                }
                else if (msgType == "auth_invalid") {
                    Serial.println("[HA] Auth Invalid! Clearing config...");
                    _config.clearHAConfig();
                    _ws.disconnect();
                }
            }
            break;
        }
        case WStype_BIN:
        case WStype_ERROR:
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
        case WStype_PING:
        case WStype_PONG:
            break;
    }
}
