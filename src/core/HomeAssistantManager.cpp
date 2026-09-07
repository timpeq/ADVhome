#include "HomeAssistantManager.h"
#include <M5Unified.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <AudioFileSource.h>
#include <AudioGeneratorMP3a.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>
#include <AudioLogger.h>

// Streams the HA TTS audio over HTTP(S) with the auth header ESP8266Audio's own sources don't support.
class AuthenticatedHttpSource : public AudioFileSource {
public:
    AuthenticatedHttpSource(const String& token) : _token(token) {}

    bool open(const char* url) override {
        _url = url;
        bool connected = _url.startsWith("https://")
            ? (_secure.setInsecure(), _http.begin(_secure, _url))
            : _http.begin(_plain, _url);
        if (!connected) return false;

        _http.addHeader("Authorization", "Bearer " + _token);
        _http.addHeader("Accept", "audio/mpeg,audio/wav,application/octet-stream");
        _http.setTimeout(15000);
        int status = _http.GET();
        Serial.println("[HA] TTS stream HTTP status: " + String(status) + ", size=" + String(_http.getSize()));
        if (status < 200 || status >= 300) {
            _http.end();
            return false;
        }
        _size = _http.getSize();
        _position = 0;
        return true;
    }

    uint32_t read(void* data, uint32_t length) override {
        auto* stream = _http.getStreamPtr();
        if (!stream) return 0;
        uint32_t waitStart = millis();
        while (stream->available() == 0 && _http.connected()) {
            if (millis() - waitStart > 3000) return 0; // stalled connection
            yield();
        }
        size_t available = stream->available();
        if (available == 0) return 0;
        if (length > available) length = available;
        if (_size >= 0 && length > (uint32_t)(_size - _position)) length = _size - _position;
        int count = stream->read(reinterpret_cast<uint8_t*>(data), length);
        if (count > 0) _position += count;
        return count > 0 ? count : 0;
    }

    bool close() override { _http.end(); return true; }
    bool isOpen() override { return _http.connected(); }
    uint32_t getSize() override { return _size > 0 ? _size : 0; }
    uint32_t getPos() override { return _position; }

private:
    String _token;
    String _url;
    HTTPClient _http;
    WiFiClientSecure _secure;
    WiFiClient _plain;
    int _size = -1;
    uint32_t _position = 0;
};

HomeAssistantManager::HomeAssistantManager(ConfigManager& config, EntityManager& entityManager) : _config(config), _entityManager(entityManager) {
    audioLogger = &Serial; // ESP8266Audio silences decode/output errors by default
}

void HomeAssistantManager::resetWebSocket() {
    _ws.disconnect();
    _ws.~WebSocketsClient();
    new (&_ws) WebSocketsClient();
}

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
    
    Serial.printf("[HA] Opening %s connection, free heap=%u\n", isSecure ? "wss" : "ws", ESP.getFreeHeap());
    Serial.flush();
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
    if (_ttsGenerator) {
        if (_ttsGenerator->isRunning() && _ttsGenerator->loop()) return;
        Serial.println("[HA] TTS playback finished");
        stopVoicePlayback();
        if (_voiceCallback) _voiceCallback("Voice: finished");
        return;
    }
    processVoiceResponse();
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
            filter["event"]["type"] = true;
            filter["event"]["data"]["entity_id"] = true;
            filter["event"]["data"]["runner_data"]["stt_binary_handler_id"] = true;
            filter["event"]["data"]["stt_output"]["text"] = true;
            filter["event"]["data"]["intent_input"] = true;
            filter["event"]["data"]["intent_output"]["response"]["speech"]["plain"]["speech"] = true;
            filter["event"]["data"]["intent_output"]["response"]["speech"]["ssml"]["speech"] = true;
            filter["event"]["data"]["tts_input"] = true;
            filter["event"]["data"]["tts_output"]["token"] = true;
            filter["event"]["data"]["tts_output"]["url"] = true;
            filter["event"]["data"]["tts_output"]["mime_type"] = true;
            filter["event"]["data"]["tts_output"]["stream_response"] = true;
            filter["event"]["data"]["message"] = true;
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

                    String eventType;
                    if (msgType == "event") {
                        eventType = doc["event"]["type"] | "";
                        if (eventType.isEmpty()) eventType = doc["event"]["event_type"] | "";
                    }
                    // Only accept pipeline events for the run we started; otherwise a stray/foreign
                    // Assist pipeline run (e.g. HA's continue-conversation follow-up, or another
                    // satellite) gets shown as if it belonged to this device's chat.
                    bool isVoiceEvent = msgType == "event" && doc["id"] == _voiceRequestId &&
                        (eventType == "run-start" || eventType == "stt-start" ||
                         eventType == "stt-end" || eventType == "intent-start" ||
                         eventType == "intent-progress" || eventType == "intent-end" ||
                         eventType == "tts-start" || eventType == "tts-end" ||
                         eventType == "run-end" || eventType == "error");
                
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
                    _ttsTransitioning = false;
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
                else if (isVoiceEvent) {
                    JsonObject eventData = doc["event"]["data"].as<JsonObject>();
                        Serial.println("[HA] Voice event: " + eventType + " id=" + String((int)(doc["id"] | 0)));
                    if (eventType == "run-start") {
                        _voiceBinaryHandlerId = eventData["runner_data"]["stt_binary_handler_id"] | -1;
                        Serial.println("[HA] Voice audio handler: " + String(_voiceBinaryHandlerId));
                        if (_voiceCallback) _voiceCallback("Voice: listening");
                    } else if (eventType == "stt-end") {
                        String transcript = eventData["stt_output"]["text"] | "";
                        if (!transcript.isEmpty() && _voiceCallback) {
                            _voiceReceivedText = true;
                            _voiceCallback("You: " + transcript);
                        }
                    } else if (eventType == "intent-end") {
                        String response = eventData["intent_output"]["response"]["speech"]["plain"]["speech"] | "";
                        if (response.isEmpty()) {
                            response = eventData["intent_output"]["response"]["speech"]["ssml"]["speech"] | "";
                        }
                        Serial.println("[HA] Voice intent response: " + response);
                        if (!response.isEmpty() && _voiceCallback) {
                            _voiceCallback("HA: " + response);
                        }
                    } else if (eventType == "intent-start") {
                        String input = eventData["intent_input"] | "";
                        Serial.println("[HA] Voice intent input: " + input);
                        if (_voiceCallback) _voiceCallback("Voice: thinking");
                    } else if (eventType == "tts-start") {
                        String input = eventData["tts_input"] | "";
                        Serial.println("[HA] Voice TTS start: " + input);
                        if (_voiceCallback) _voiceCallback("Voice: speaking");
                    } else if (eventType == "tts-end") {
                        String mimeType = eventData["tts_output"]["mime_type"] | "";
                        String audioUrl = eventData["tts_output"]["url"] | "";
                        bool streaming = eventData["tts_output"]["stream_response"] | false;
                        Serial.println("[HA] Voice TTS end: mime=" + mimeType + ", streaming=" + String(streaming ? "yes" : "no"));
                        Serial.println("[HA] Voice TTS URL: " + audioUrl);
                        queueVoiceResponse(audioUrl, mimeType);
                        if (_voiceCallback) _voiceCallback("Voice: response ready");
                    } else if (eventType == "error") {
                        String errorMessage = eventData["message"] | "Voice pipeline failed";
                        bool staleNoTextError = errorMessage.indexOf("text recognized") >= 0;
                        if (!_voiceReceivedText || !staleNoTextError) {
                            if (_voiceCallback) _voiceCallback("Error: " + errorMessage);
                        }
                    } else if (eventType == "run-end") {
                        _voiceBinaryHandlerId = -1;
                        _voiceRunFinished = true;
                        _voiceRequestId = 0;
                        if (_voiceCallback) _voiceCallback("Voice: finished");
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

bool HomeAssistantManager::startVoicePipeline() {
    // Barge-in: talking again immediately drops any TTS playback in progress or queued.
    if (_ttsGenerator || _ttsSource || _ttsPending) {
        Serial.println("[HA] Barge-in: interrupting TTS playback for new voice request");
        _ttsPending = false;
        _ttsUrl = "";
        stopVoicePlayback();
    }

    if (!_isConnected || !_isAuthenticated) return false;

    JsonDocument doc;
    _voiceRequestId = _nextMsgId++;
    _voiceBinaryHandlerId = -1;
    _voiceRunFinished = false;
    _voiceReceivedText = false;
    doc["id"] = _voiceRequestId;
    doc["type"] = "assist_pipeline/run";
    doc["start_stage"] = "stt";
    doc["end_stage"] = "tts";
    doc["input"]["sample_rate"] = 16000;

    String payload;
    serializeJson(doc, payload);
    _ws.sendTXT(payload);
    if (_voiceCallback) _voiceCallback("Voice: starting");
    return true;
}

void HomeAssistantManager::sendVoiceAudio(const int16_t* samples, size_t sampleCount) {
    if (_voiceBinaryHandlerId < 0 || !samples || sampleCount == 0) return;

    size_t packetSize = 1 + sampleCount * sizeof(int16_t);
    if (_voicePacket.size() < packetSize) _voicePacket.resize(packetSize);
    _voicePacket[0] = static_cast<uint8_t>(_voiceBinaryHandlerId);
    memcpy(_voicePacket.data() + 1, samples, sampleCount * sizeof(int16_t));
    _ws.sendBIN(_voicePacket.data(), packetSize);
}

void HomeAssistantManager::finishVoicePipeline() {
    if (_voiceBinaryHandlerId < 0) return;
    uint8_t endMarker = static_cast<uint8_t>(_voiceBinaryHandlerId);
    _ws.sendBIN(&endMarker, 1);
    _voiceBinaryHandlerId = -1;
}

void HomeAssistantManager::queueVoiceResponse(const String& url, const String& mimeType) {
    if (url.isEmpty()) return;
    _ttsUrl = url;
    _ttsMimeType = mimeType;
    _ttsPending = true;
}

void HomeAssistantManager::processVoiceResponse() {
    if (!_ttsPending || _ttsUrl.isEmpty() || !_voiceRunFinished) return;
    if (!_isConnected || !_isAuthenticated) return;
    if (_ttsGenerator) return;

    if (!_config.getTtsEnabled()) {
        // Leave the websocket/mic pipeline untouched; only the audio download+decode is skipped.
        _ttsPending = false;
        _ttsUrl = "";
        if (_voiceCallback) _voiceCallback("Voice: finished");
        return;
    }

    // HA local engines (e.g. Piper) typically return wav; cloud/Nabu TTS returns mp3.
    // Default to mp3 when the mime type is missing/unrecognized since that's HA's overall default.
    bool isWav = _ttsMimeType.indexOf("wav") >= 0;

    String url = _ttsUrl;
    String baseUrl = _config.getHAUrl();
    if (baseUrl.endsWith("/")) baseUrl.remove(baseUrl.length() - 1);
    if (url.startsWith("/")) {
        url = baseUrl + url;
    } else if (url.startsWith("http://") || url.startsWith("https://")) {
        int authorityStart = url.indexOf("://") + 3;
        int pathStart = url.indexOf('/', authorityStart);
        if (pathStart >= 0) {
            url = baseUrl + url.substring(pathStart);
        }
    }
    url.trim();

    // --- NGINX / TLS BYPASS ---
    // The user confirmed that the same host accepts plain HTTP on port 8123.
    // We rewrite the URL from https://host/api... to http://host:8123/api...
    // This completely bypasses the TLS overhead and Nginx SNI issues!
    if (url.startsWith("https://")) {
        String rewritten = url;
        rewritten.replace("https://", "http://");
        int pathStart = rewritten.indexOf('/', 7);
        if (pathStart != -1) {
            String hostPart = rewritten.substring(7, pathStart);
            if (hostPart.indexOf(':') == -1) {
                rewritten = "http://" + hostPart + ":8123" + rewritten.substring(pathStart);
            }
        }
        url = rewritten;
    }

    Serial.println("[HA] Streaming TTS (" + String(isWav ? "wav" : "mp3") + "): " + url);
    _ttsTransitioning = true;
    resetWebSocket();
    _isConnected = false;
    _isAuthenticated = false;
    delay(100);

    _ttsSource = new AuthenticatedHttpSource(_config.getHAToken());
    if (!_ttsSource->open(url.c_str())) {
        Serial.println("[HA] TTS stream connection failed");
        stopVoicePlayback();
        if (_voiceCallback) _voiceCallback("Error: TTS stream connection failed");
        _ttsPending = false;
        return;
    }

    M5.Speaker.end(); // release I2S_NUM_1 so AudioOutputI2S can drive it directly
    _ttsOutput = new AudioOutputI2S(1);
    _ttsOutput->SetPinout(41, 43, 42);
    _ttsOutput->SetOutputModeMono(true);
    _ttsOutput->SetGain(1.0);

    _ttsGenerator = isWav ? static_cast<AudioGenerator*>(new AudioGeneratorWAV())
                          : static_cast<AudioGenerator*>(new AudioGeneratorMP3a());

    if (_ttsGenerator->begin(_ttsSource, _ttsOutput)) {
        _ttsPending = false;
        Serial.println("[HA] TTS playback started");
        if (_voiceCallback) _voiceCallback("Voice: playing");
    } else {
        Serial.println("[HA] TTS decoder failed to start");
        stopVoicePlayback();
        if (_voiceCallback) _voiceCallback("Error: TTS decoder startup failed");
    }
}

void HomeAssistantManager::stopVoicePlayback() {
    if (_ttsGenerator) {
        _ttsGenerator->stop();
        delete _ttsGenerator;
        _ttsGenerator = nullptr;
    }
    if (_ttsSource) {
        _ttsSource->close();
        delete _ttsSource;
        _ttsSource = nullptr;
    }
    if (_ttsOutput) {
        _ttsOutput->stop();
        delete _ttsOutput;
        _ttsOutput = nullptr;
    }
    if (!M5.Speaker.isRunning()) M5.Speaker.begin();

    if (!_isConnected) {
        Serial.println("[HA] Resuming Home Assistant WebSocket after TTS");
        begin();
    }
    Serial.printf("[HA] TTS playback stopped, free heap=%u\n", ESP.getFreeHeap());
    Serial.flush();
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
