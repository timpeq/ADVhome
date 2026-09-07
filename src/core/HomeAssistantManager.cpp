#include "HomeAssistantManager.h"

HomeAssistantManager::HomeAssistantManager(ConfigManager& config, EntityManager& entityManager) : _config(config), _entityManager(entityManager) {}

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
    Serial.println(String("[HA] Attempting connection to ") + (isSecure ? "wss://" : "ws://") + host + ":" + String(port) + "/api/websocket"); _ws.setReconnectInterval(5000);
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
            JsonDocument filter;
            filter["type"] = true;
            filter["id"] = true;
            filter["success"] = true;
            filter["ha_version"] = true;
            
            filter["result"][0]["entity_id"] = true;
            filter["result"][0]["state"] = true;
            filter["result"][0]["attributes"]["friendly_name"] = true;
            filter["result"][0]["attributes"]["media_title"] = true;
            filter["result"][0]["attributes"]["media_artist"] = true;
            filter["result"][0]["attributes"]["media_album_name"] = true;
            filter["result"][0]["attributes"]["media_duration"] = true;
            filter["result"][0]["attributes"]["media_position"] = true;
            filter["result"][0]["attributes"]["volume_level"] = true;
            filter["result"][0]["attributes"]["is_volume_muted"] = true;
            
            filter["event"]["event_type"] = true;
            filter["event"]["data"]["entity_id"] = true;
            filter["event"]["data"]["new_state"]["state"] = true;
            filter["event"]["data"]["new_state"]["attributes"]["friendly_name"] = true;
            filter["event"]["data"]["new_state"]["attributes"]["media_title"] = true;
            filter["event"]["data"]["new_state"]["attributes"]["media_artist"] = true;
            filter["event"]["data"]["new_state"]["attributes"]["media_album_name"] = true;
            filter["event"]["data"]["new_state"]["attributes"]["media_duration"] = true;
            filter["event"]["data"]["new_state"]["attributes"]["media_position"] = true;
            filter["event"]["data"]["new_state"]["attributes"]["volume_level"] = true;
            filter["event"]["data"]["new_state"]["attributes"]["is_volume_muted"] = true;

            filter["error"]["message"] = true;

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
            
            if (!error) {
                String msgType = doc["type"] | "";

                JsonDocument conversationDoc;
                bool isConversationResponse = msgType == "result" && doc["id"] == _conversationRequestId;
                if (isConversationResponse) {
                    deserializeJson(conversationDoc, payload, length);
                }
                
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
                    
                    // Subscribe to state changes (initial states already fetched via HTTP)
                    _ws.sendTXT("{\"id\": 2, \"type\": \"subscribe_events\", \"event_type\": \"state_changed\"}");
                }
                else if (msgType == "auth_invalid") {
                    Serial.println("[HA] Auth Invalid! Clearing config...");
                    _config.clearHAConfig();
                    _ws.disconnect();
                }
                else if (msgType == "result" && doc["id"] == _conversationRequestId) {
                    JsonObject result = conversationDoc["result"].as<JsonObject>();
                    if (doc["success"] && !result["response"]["speech"]["plain"]["speech"].isNull()) {
                        _conversationId = result["conversation_id"] | _conversationId;
                        if (_conversationCallback) {
                            _conversationCallback(result["response"]["speech"]["plain"]["speech"].as<String>());
                        }
                    } else if (_conversationCallback) {
                        String errorMessage = doc["error"]["message"] | "Conversation request failed";
                        _conversationCallback("Error: " + errorMessage);
                    }
                }
                else if (msgType == "result" && doc["id"] == 1 && doc["success"]) {
                    JsonArray result = doc["result"].as<JsonArray>();
                    for (JsonObject stateObj : result) {
                        String entity_id = stateObj["entity_id"].as<String>();
                        String state = stateObj["state"].as<String>();
                        String friendly_name = stateObj["attributes"]["friendly_name"] | "";
                        
                        if (entity_id.startsWith("sensor.")) {
                            String device_class = stateObj["attributes"]["device_class"] | "";
                            if (device_class != "temperature" && device_class != "humidity") {
                                continue;
                            }
                        }
                        
                        _entityManager.updateEntity(entity_id, state, friendly_name);
                        if (entity_id.startsWith("media_player.")) {
                            _entityManager.updateMediaAttributes(entity_id,
                                stateObj["attributes"]["media_title"] | "",
                                stateObj["attributes"]["media_artist"] | "",
                                stateObj["attributes"]["media_album_name"] | "",
                                stateObj["attributes"]["media_duration"] | 0.0f,
                                stateObj["attributes"]["media_position"] | 0.0f,
                                stateObj["attributes"]["volume_level"] | 0.0f,
                                stateObj["attributes"]["is_volume_muted"] | false);
                        }
                    }
                    Serial.println("[HA] Initial states loaded.");
                }
                else if (msgType == "event" && doc["event"]["event_type"] == "state_changed") {
                    JsonObject eventData = doc["event"]["data"];
                    String entity_id = eventData["entity_id"].as<String>();
                    String state = eventData["new_state"]["state"].as<String>();
                    String friendly_name = eventData["new_state"]["attributes"]["friendly_name"] | "";
                    
                    if (entity_id.startsWith("sensor.")) {
                        String device_class = eventData["new_state"]["attributes"]["device_class"] | "";
                        if (device_class != "temperature" && device_class != "humidity") {
                            return; // Stop processing this event
                        }
                    }
                    
                    _entityManager.updateEntity(entity_id, state, friendly_name);
                    if (entity_id.startsWith("media_player.")) {
                        JsonObject attrs = eventData["new_state"]["attributes"];
                        _entityManager.updateMediaAttributes(entity_id,
                            attrs["media_title"] | "",
                            attrs["media_artist"] | "",
                            attrs["media_album_name"] | "",
                            attrs["media_duration"] | 0.0f,
                            attrs["media_position"] | 0.0f,
                            attrs["volume_level"] | 0.0f,
                            attrs["is_volume_muted"] | false);
                    }
                }
            } else {
                Serial.println("[HA] JSON Parse Failed!");
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

void HomeAssistantManager::sendConversation(const String& text) {
    if (text.isEmpty()) return;
    if (!_isConnected || !_isAuthenticated) {
        if (_conversationCallback) _conversationCallback("Error: Home Assistant is not connected");
        return;
    }

    JsonDocument doc;
    _conversationRequestId = _nextMsgId++;
    doc["id"] = _conversationRequestId;
    doc["type"] = "conversation/process";
    doc["text"] = text;
    doc["language"] = "en";
    if (!_conversationId.isEmpty()) doc["conversation_id"] = _conversationId;

    String payload;
    serializeJson(doc, payload);
    _ws.sendTXT(payload);
    Serial.println("[HA] Sent conversation/process");
}

void HomeAssistantManager::callService(const String& domain, const String& service, const String& entity_id) {
    if (!_isConnected || !_isAuthenticated) return;
    
    JsonDocument doc;
    doc["id"] = _nextMsgId++;
    doc["type"] = "call_service";
    doc["domain"] = domain;
    doc["service"] = service;
    
    JsonObject target = doc["target"].to<JsonObject>();
    target["entity_id"] = entity_id;
    
    String payload;
    serializeJson(doc, payload);
    
    _ws.sendTXT(payload);
    Serial.println("[HA] Sent call_service: " + payload);
}

void HomeAssistantManager::setMediaVolume(const String& entity_id, float volume) {
    if (!_isConnected || !_isAuthenticated) return;

    volume = constrain(volume, 0.0f, 1.0f);

    JsonDocument doc;
    doc["id"] = _nextMsgId++;
    doc["type"] = "call_service";
    doc["domain"] = "media_player";
    doc["service"] = "volume_set";
    JsonObject target = doc["target"].to<JsonObject>();
    target["entity_id"] = entity_id;
    JsonObject data = doc["service_data"].to<JsonObject>();
    data["volume_level"] = volume;

    String payload;
    serializeJson(doc, payload);
    _ws.sendTXT(payload);
    Serial.println("[HA] Sent volume_set: " + payload);
}

void HomeAssistantManager::toggleMute(const String& entity_id, bool is_muted) {
    if (!_isConnected || !_isAuthenticated) return;

    JsonDocument doc;
    doc["id"] = _nextMsgId++;
    doc["type"] = "call_service";
    doc["domain"] = "media_player";
    doc["service"] = "volume_mute";
    JsonObject target = doc["target"].to<JsonObject>();
    target["entity_id"] = entity_id;
    JsonObject data = doc["service_data"].to<JsonObject>();
    data["is_volume_muted"] = is_muted;

    String payload;
    serializeJson(doc, payload);
    _ws.sendTXT(payload);
    Serial.println("[HA] Sent volume_mute: " + payload);
}

void HomeAssistantManager::seekMedia(const String& entity_id, float position) {
    if (!_isConnected || !_isAuthenticated) return;

    if (position < 0) position = 0;

    JsonDocument doc;
    doc["id"] = _nextMsgId++;
    doc["type"] = "call_service";
    doc["domain"] = "media_player";
    doc["service"] = "media_seek";
    JsonObject target = doc["target"].to<JsonObject>();
    target["entity_id"] = entity_id;
    JsonObject data = doc["service_data"].to<JsonObject>();
    data["seek_position"] = position;

    String payload;
    serializeJson(doc, payload);
    _ws.sendTXT(payload);
    Serial.println("[HA] Sent media_seek: " + payload);
}

void HomeAssistantManager::adjustEntity(const String& entity_id, int direction) {
    if (!_isAuthenticated) return;
    
    Entity entity = _entityManager.getEntity(entity_id);
    if (entity.id == "") return;

    if (entity.domain == "media_player") {
        float step = 0.05f; // 5%
        float newVol = entity.volumeLevel + (direction > 0 ? step : -step);
        if (newVol < 0.0f) newVol = 0.0f;
        if (newVol > 1.0f) newVol = 1.0f;
        setMediaVolume(entity_id, newVol);
        
        // Optimistically update the entity locally so rapid presses reflect instantly
        _entityManager.updateMediaAttributes(entity_id, entity.mediaTitle, entity.mediaArtist, entity.mediaAlbum, entity.mediaDuration, entity.mediaPosition, newVol, entity.isVolumeMuted);
    } else if (entity.domain == "light") {
        DynamicJsonDocument doc(512);
        doc["id"] = _nextMsgId++;
        doc["type"] = "call_service";
        doc["domain"] = "light";
        doc["service"] = "turn_on";
        
        JsonObject target = doc.createNestedObject("target");
        target["entity_id"] = entity_id;
        
        JsonObject service_data = doc.createNestedObject("service_data");
        service_data["brightness_step_pct"] = direction > 0 ? 10 : -10;
        
        String payload;
        serializeJson(doc, payload);
        _ws.sendTXT(payload);
    }
}

void HomeAssistantManager::callSecureService(const String& domain, const String& service, const String& entity_id, const String& code) {
    if (!_isConnected || !_isAuthenticated) return;

    JsonDocument doc;
    doc["id"] = _nextMsgId++;
    doc["type"] = "call_service";
    doc["domain"] = domain;
    doc["service"] = service;
    JsonObject target = doc["target"].to<JsonObject>();
    target["entity_id"] = entity_id;
    JsonObject data = doc["service_data"].to<JsonObject>();
    data["code"] = code;

    String payload;
    serializeJson(doc, payload);
    _ws.sendTXT(payload);
    Serial.println("[HA] Sent secure call_service: " + payload);
}
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <HTTPClient.h>
#include <WiFiClientSecure.h>

void HomeAssistantManager::fetchInitialStates() {
    String url = _config.getHAUrl();
    if (url.endsWith("/")) url = url.substring(0, url.length() - 1);
    url += "/api/states";
    
    Serial.println("[HA] Fetching initial states via HTTP Chunking: " + url);
    
    HTTPClient http;
    WiFiClientSecure *secureClient = nullptr;
    WiFiClient *client = nullptr;
    
    if (url.startsWith("https")) {
        secureClient = new WiFiClientSecure();
        secureClient->setInsecure();
        http.begin(*secureClient, url);
    } else {
        client = new WiFiClient();
        http.begin(*client, url);
    }
    
    http.addHeader("Authorization", "Bearer " + _config.getHAToken());
    http.addHeader("Content-Type", "application/json");
    
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        WiFiClient* stream = http.getStreamPtr();
        
        // Skip until '['
        while (stream->connected() || stream->available()) {
            if (stream->available() && stream->read() == '[') break;
            delay(1);
        }
        
        int braceCount = 0;
        String objStr = "";
        objStr.reserve(2048);
        bool inString = false;
        bool escape = false;
        
        int count = 0;
        
        while (stream->connected() || stream->available()) {
            if (!stream->available()) {
                delay(1);
                continue;
            }
            
            char c = stream->read();
            
            if (c == ']' && braceCount == 0) break;
            
            if (braceCount > 0 || c == '{') {
                objStr += c;
                
                if (c == '"' && !escape) inString = !inString;
                
                if (!inString) {
                    if (c == '{') braceCount++;
                    else if (c == '}') {
                        braceCount--;
                        if (braceCount == 0) {
                            JsonDocument doc;
                            if (!deserializeJson(doc, objStr)) {
                                String entity_id = doc["entity_id"].as<String>();
                                String state = doc["state"].as<String>();
                                String friendly_name = doc["attributes"]["friendly_name"] | "";
                                
                                bool skip = false;
                                if (entity_id.startsWith("sensor.")) {
                                    String device_class = doc["attributes"]["device_class"] | "";
                                    if (device_class != "temperature" && device_class != "humidity") {
                                        skip = true;
                                    }
                                }
                                
                                if (!skip) {
                                    _entityManager.updateEntity(entity_id, state, friendly_name);
                                    if (entity_id.startsWith("media_player.")) {
                                    _entityManager.updateMediaAttributes(entity_id,
                                        doc["attributes"]["media_title"] | "",
                                        doc["attributes"]["media_artist"] | "",
                                        doc["attributes"]["media_album_name"] | "",
                                        doc["attributes"]["media_duration"] | 0.0f,
                                        doc["attributes"]["media_position"] | 0.0f,
                                        doc["attributes"]["volume_level"] | 0.0f,
                                        doc["attributes"]["is_volume_muted"] | false);
                                    }
                                    count++;
                                }
                            }
                            objStr = "";
                        }
                    }
                }
                
                if (c == '\\' && !escape) escape = true;
                else escape = false;
            }
        }
        Serial.println("[HA] Loaded " + String(count) + " entities via HTTP chunking.");
    } else {
        Serial.println(String("[HA] HTTP GET failed, error: ") + http.errorToString(httpCode).c_str());
    }
    http.end();
    
    if (secureClient) delete secureClient;
    if (client) delete client;
}
