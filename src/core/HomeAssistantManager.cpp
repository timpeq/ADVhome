#include "HomeAssistantManager.h"
#include <M5Unified.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <string.h>
#include <math.h>

namespace {

// Pull the climate attributes we care about out of a state object's
// "attributes". Used by applyEntityState(), which both the initial download
// and the WebSocket listener go through, so a thermostat is populated at boot
// instead of staying empty until Home Assistant pushes a state_changed for it.
void readClimateAttributes(JsonVariantConst attrs, ClimateState& climate) {
    if (!attrs["current_temperature"].isNull()) climate.currentTemperature = attrs["current_temperature"].as<float>();
    if (!attrs["temperature"].isNull())         climate.targetTemperature  = attrs["temperature"].as<float>();
    if (!attrs["target_temp_high"].isNull())    climate.targetTempHigh     = attrs["target_temp_high"].as<float>();
    if (!attrs["target_temp_low"].isNull())     climate.targetTempLow      = attrs["target_temp_low"].as<float>();
    if (!attrs["current_humidity"].isNull())    climate.currentHumidity    = attrs["current_humidity"].as<float>();
    climate.minTemp        = attrs["min_temp"] | 7.0f;
    climate.maxTemp        = attrs["max_temp"] | 35.0f;
    climate.targetTempStep = attrs["target_temp_step"] | 0.5f;
    climate.hvacAction     = attrs["hvac_action"] | "";

    JsonArrayConst modes = attrs["hvac_modes"].as<JsonArrayConst>();
    if (!modes.isNull()) {
        String joined;
        for (JsonVariantConst mode : modes) {
            if (joined.length() > 0) joined += ",";
            joined += mode.as<String>();
        }
        climate.hvacModes = joined;
    }
}

// Light brightness, and whether the light can dim at all. Home Assistant
// reports brightness as null while a light is off, which reads back as -1.
void readLightAttributes(JsonVariantConst attrs, LightState& light) {
    light.brightness = attrs["brightness"].isNull() ? -1 : (int)attrs["brightness"].as<float>();
    light.dimmable = false;
    JsonArrayConst modes = attrs["supported_color_modes"].as<JsonArrayConst>();
    if (modes.isNull()) {
        // No colour modes reported (an unavailable light, or an integration
        // that predates them): trust a reported brightness.
        light.dimmable = light.brightness >= 0;
        return;
    }
    for (JsonVariantConst mode : modes) {
        if (strcmp(mode | "", "onoff") != 0) {
            light.dimmable = true;
            break;
        }
    }
}

} // namespace


HomeAssistantManager::HomeAssistantManager(ConfigManager& config, EntityManager& entityManager) : _config(config), _entityManager(entityManager) {
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
    Serial.println(String("[HA] Attempting connection to ") + (isSecure ? "wss://" : "ws://") + host + ":" + String(port) + "/api/websocket"); _ws.setReconnectInterval(_config.getReconnectInterval());
}

void HomeAssistantManager::update() {
    _ws.loop();
    if (_ttsPlaying) {
        pumpTtsWavStream();
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
            // Built once: the same projection is applied to every frame, and
            // constructing it key-by-key inlines a large amount of code here.
            static JsonDocument filter;
            if (filter.isNull()) {
                deserializeJson(filter, F(
                    "{"
                        "\"type\":true,"
                        "\"id\":true,"
                        "\"success\":true,"
                        "\"ha_version\":true,"
                        "\"result\":["
                            "{"
                                "\"entity_id\":true,"
                                "\"state\":true,"
                                "\"attributes\":{"
                                    "\"friendly_name\":true,"
                                    "\"media_title\":true,"
                                    "\"media_artist\":true,"
                                    "\"media_album_name\":true,"
                                    "\"media_duration\":true,"
                                    "\"media_position\":true,"
                                    "\"volume_level\":true,"
                                    "\"is_volume_muted\":true,"
                                    "\"device_class\":true,"
                                    "\"current_temperature\":true,"
                                    "\"temperature\":true,"
                                    "\"target_temp_high\":true,"
                                    "\"target_temp_low\":true,"
                                    "\"current_humidity\":true,"
                                    "\"min_temp\":true,"
                                    "\"max_temp\":true,"
                                    "\"target_temp_step\":true,"
                                    "\"hvac_action\":true,"
                                    "\"hvac_modes\":true,"
                                    "\"brightness\":true,"
                                    "\"supported_color_modes\":true"
                                "}"
                            "}"
                        "],"
                        "\"event\":{"
                            "\"event_type\":true,"
                            "\"type\":true,"
                            "\"data\":{"
                                "\"entity_id\":true,"
                                "\"runner_data\":{"
                                    "\"stt_binary_handler_id\":true"
                                "},"
                                "\"stt_output\":{"
                                    "\"text\":true"
                                "},"
                                "\"intent_input\":true,"
                                "\"intent_output\":{"
                                    "\"response\":{"
                                        "\"speech\":{"
                                            "\"plain\":{"
                                                "\"speech\":true"
                                            "},"
                                            "\"ssml\":{"
                                                "\"speech\":true"
                                            "}"
                                        "}"
                                    "}"
                                "},"
                                "\"tts_input\":true,"
                                "\"tts_output\":{"
                                    "\"token\":true,"
                                    "\"url\":true,"
                                    "\"mime_type\":true,"
                                    "\"stream_response\":true"
                                "},"
                                "\"message\":true,"
                                "\"new_state\":{"
                                    "\"state\":true,"
                                    "\"attributes\":{"
                                        "\"friendly_name\":true,"
                                        "\"media_title\":true,"
                                        "\"media_artist\":true,"
                                        "\"media_album_name\":true,"
                                        "\"media_duration\":true,"
                                        "\"media_position\":true,"
                                        "\"volume_level\":true,"
                                        "\"is_volume_muted\":true,"
                                        "\"device_class\":true,"
                                        "\"current_temperature\":true,"
                                        "\"temperature\":true,"
                                        "\"target_temp_high\":true,"
                                        "\"target_temp_low\":true,"
                                        "\"current_humidity\":true,"
                                        "\"min_temp\":true,"
                                        "\"max_temp\":true,"
                                        "\"target_temp_step\":true,"
                                        "\"hvac_action\":true,"
                                        "\"hvac_modes\":true,"
                                        "\"brightness\":true,"
                                        "\"supported_color_modes\":true"
                                    "}"
                                "}"
                            "}"
                        "},"
                        "\"error\":{"
                            "\"message\":true"
                        "}"
                    "}"));
            }

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
            
            if (!error) {
                String msgType = doc["type"] | "";

                JsonDocument conversationDoc;
                bool isConversationResponse = msgType == "result" && doc["id"] == _conversationRequestId;
                if (isConversationResponse) {
                    deserializeJson(conversationDoc, payload, length);
                }

                JsonDocument pipelineDoc;
                bool isPipelineListResponse = msgType == "result" && _pipelineListRequestId != 0 &&
                                              doc["id"] == _pipelineListRequestId;
                if (isPipelineListResponse) {
                    deserializeJson(pipelineDoc, payload, length);
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
                    requestPipelineList();
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
                else if (isPipelineListResponse) {
                    if (doc["success"]) {
                        _pipelines.clear();
                        JsonObject result = pipelineDoc["result"].as<JsonObject>();
                        _preferredPipelineId = result["preferred_pipeline"] | "";
                        for (JsonObject p : result["pipelines"].as<JsonArray>()) {
                            VoicePipeline vp;
                            vp.id = p["id"] | "";
                            vp.name = p["name"] | "";
                            vp.ttsEngine = p["tts_engine"] | "";
                            vp.ttsVoice = p["tts_voice"] | "";
                            vp.ttsLanguage = p["tts_language"] | "";
                            if (!vp.id.isEmpty()) _pipelines.push_back(vp);
                        }
                        Serial.println("[HA] Pipelines: " + String(_pipelines.size()) +
                                       ", preferred=" + _preferredPipelineId);
                        if (_pipelinesCallback) _pipelinesCallback();
                    }
                }
                else if (isVoiceEvent) {
                    JsonObject eventData = doc["event"]["data"].as<JsonObject>();
                        Serial.println("[HA] Voice event: " + eventType + " id=" + String((int)(doc["id"] | 0)));
                    if (eventType == "run-start") {
                        _voiceBinaryHandlerId = eventData["runner_data"]["stt_binary_handler_id"] | -1;
                        Serial.println("[HA] Voice audio handler: " + String(_voiceBinaryHandlerId));
                        if (_voiceCallback) _voiceCallback("Listening");
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
                        // The pipeline ends at "intent"; we synthesise this reply
                        // ourselves via /api/tts_get_url so we can force a WAV.
                        if (!response.isEmpty() && _config.getTtsEnabled()) {
                            _pendingTtsText = response;
                        }
                    } else if (eventType == "intent-start") {
                        String input = eventData["intent_input"] | "";
                        Serial.println("[HA] Voice intent input: " + input);
                        if (_voiceCallback) _voiceCallback("Thinking");
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
                        // Clear the status line unless TTS playback is about to start.
                        if (_voiceCallback && _pendingTtsText.isEmpty()) _voiceCallback("");
                    }
                }
                else if (msgType == "event" && doc["event"]["event_type"] == "state_changed") {
                    JsonObject eventData = doc["event"]["data"];
                    String entity_id = eventData["entity_id"].as<String>();
                    applyEntityState(entity_id, eventData["new_state"]);
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
    if (_ttsPlaying || !_pendingTtsText.isEmpty()) {
        Serial.println("[HA] Barge-in: interrupting TTS playback for new voice request");
        _pendingTtsText = "";
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
    // Stop at intent: the pipeline's own TTS stage only produces MP3 (HA's
    // default), which this firmware can't decode. We synthesise the reply
    // ourselves via /api/tts_get_url with preferred_format=wav.
    doc["end_stage"] = "intent";
    doc["input"]["sample_rate"] = 16000;

    // Pin the pipeline the user picked in Config; otherwise HA uses its preferred one.
    String pipelineId = _config.getVoicePipelineId();
    if (!pipelineId.isEmpty()) doc["pipeline"] = pipelineId;

    String payload;
    serializeJson(doc, payload);
    _ws.sendTXT(payload);
    if (_voiceCallback) _voiceCallback("Listening");
    return true;
}

void HomeAssistantManager::logTtsHeap(const char* stage) {
    ttsDiag(String("heap ") + stage +
            " free=" + String(ESP.getFreeHeap() / 1024) +
            "K blk=" + String(ESP.getMaxAllocHeap() / 1024) +
            "K min=" + String(ESP.getMinFreeHeap() / 1024) +
            "K ws=" + (_isAuthenticated ? "up" : "down"));
}

void HomeAssistantManager::ttsDiag(const String& s) {
    Serial.println("[HA] TTS " + s);
    if (_voiceCallback && _config.getTtsDebug()) _voiceCallback("TTS: " + s);
}

void HomeAssistantManager::requestPipelineList() {
    if (!_isConnected || !_isAuthenticated) return;
    _pipelineListRequestId = _nextMsgId++;
    JsonDocument doc;
    doc["id"] = _pipelineListRequestId;
    doc["type"] = "assist_pipeline/pipeline/list";
    String payload;
    serializeJson(doc, payload);
    _ws.sendTXT(payload);
    Serial.println("[HA] Requested pipeline list");
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

const HomeAssistantManager::VoicePipeline* HomeAssistantManager::activePipeline() const {
    String id = _config.getVoicePipelineId();
    if (id.isEmpty()) id = _preferredPipelineId;
    for (const auto& p : _pipelines) {
        if (p.id == id) return &p;
    }
    if (!_pipelines.empty()) return &_pipelines.front();
    return nullptr;
}

String HomeAssistantManager::plainHttpBase() const {
    String base = _config.getHAUrl();
    base.trim();
    if (base.endsWith("/")) base.remove(base.length() - 1);
    // The ESP32-S3 has no headroom for a second TLS session next to the wss
    // WebSocket, so audio/TTS HTTP must be plain. Rewrite https to http on :8123.
    if (base.startsWith("https://")) {
        base.replace("https://", "http://");
        if (base.indexOf(':', 7) == -1) base += ":8123";
    }
    return base;
}

String HomeAssistantManager::requestWavTtsUrl(const String& text) {
    const VoicePipeline* pipe = activePipeline();
    String engine = pipe ? pipe->ttsEngine : String();
    if (engine.isEmpty()) {
        if (_voiceCallback) _voiceCallback("Error: no TTS engine (open Config)");
        return String();
    }

    String base = plainHttpBase();
    if (!base.startsWith("http://")) {
        if (_voiceCallback) _voiceCallback("Error: HA URL not reachable over http");
        return String();
    }

    JsonDocument body;
    body["engine_id"] = engine;
    body["message"] = text;
    if (pipe && !pipe->ttsLanguage.isEmpty()) body["language"] = pipe->ttsLanguage;
    body["options"]["preferred_format"] = "wav";
    if (pipe && !pipe->ttsVoice.isEmpty()) body["options"]["voice"] = pipe->ttsVoice;
    String payload;
    serializeJson(body, payload);

    WiFiClient client;
    HTTPClient http;
    http.setReuse(false);
    if (!http.begin(client, base + "/api/tts_get_url")) {
        if (_voiceCallback) _voiceCallback("Error: tts_get_url begin failed");
        return String();
    }
    http.addHeader("Authorization", "Bearer " + _config.getHAToken());
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(8000);
    int code = http.POST(payload);
    String resp = (code > 0) ? http.getString() : String();
    http.end();
    Serial.printf("[HA] tts_get_url HTTP %d\n", code);
    if (code != HTTP_CODE_OK) {
        if (_voiceCallback) _voiceCallback("Error: tts_get_url HTTP " + String(code));
        return String();
    }

    JsonDocument doc;
    if (deserializeJson(doc, resp)) {
        if (_voiceCallback) _voiceCallback("Error: tts_get_url bad JSON");
        return String();
    }
    String path = doc["path"] | "";
    if (path.isEmpty()) {
        String full = doc["url"] | "";
        int p = full.indexOf('/', full.indexOf("://") + 3);
        if (p >= 0) path = full.substring(p);
    }
    if (path.isEmpty()) {
        if (_voiceCallback) _voiceCallback("Error: tts_get_url no path");
        return String();
    }
    Serial.println("[HA] TTS engine=" + engine + " url=" + base + path);
    ttsDiag(engine + " -> " + path);
    return base + path;
}

void HomeAssistantManager::processVoiceResponse() {
    if (_pendingTtsText.isEmpty() || !_voiceRunFinished) return;
    if (!_isConnected || !_isAuthenticated) return;
    if (_ttsPlaying) return;

    String text = _pendingTtsText;
    _pendingTtsText = "";

    if (!_config.getTtsEnabled()) {
        if (_voiceCallback) _voiceCallback("");
        return;
    }

    if (_pipelines.empty()) {
        // list not back yet; try once more shortly
        _pendingTtsText = text;
        return;
    }

    if (_voiceCallback) _voiceCallback("Speaking");
    logTtsHeap("pre");

    // Over wss the TLS session and the playback buffers do not fit in RAM
    // together: once the WAV fetch opens there is ~33K left against 30K of PCM
    // buffers. So the socket is closed for the reply and reopened in
    // stopVoicePlayback(), which also defeats barge-in, because the next run has
    // no socket to start on. Plain ws has no TLS to free, so it stays up.
    if (_config.getHAUrl().startsWith("https")) {
        _ttsTransitioning = true;
        resetWebSocket();
        _isConnected = false;
        _isAuthenticated = false;
        delay(100);
    }

    String wavUrl = requestWavTtsUrl(text);
    if (wavUrl.isEmpty() || !startTtsWavStream(wavUrl)) {
        stopVoicePlayback();
    }
}

bool HomeAssistantManager::startTtsWavStream(const String& url) {
    if (!url.startsWith("http://")) {
        if (_voiceCallback) _voiceCallback("Error: TTS URL not http");
        return false;
    }

    uint32_t heap = ESP.getFreeHeap();
    Serial.printf("[HA] TTS fetch, free heap=%u\n", heap);
    if (heap < 45000) {
        if (_voiceCallback) _voiceCallback("Error: low RAM for TTS (" + String(heap / 1024) + "K)");
        return false;
    }

    _ttsClient = new WiFiClient();
    _ttsHttp = new HTTPClient();
    _ttsHttp->setReuse(false);
    if (!_ttsHttp->begin(*_ttsClient, url)) {
        if (_voiceCallback) _voiceCallback("Error: TTS begin() failed");
        return false;
    }
    _ttsHttp->addHeader("Authorization", "Bearer " + _config.getHAToken());
    const char* wantHeaders[] = {"Content-Type"};
    _ttsHttp->collectHeaders(wantHeaders, 1);
    // HTTP/1.0 so HA can't use Transfer-Encoding: chunked -- Arduino HTTPClient
    // does not de-chunk a stream we read directly, it would hand us the chunk
    // size lines mixed into the audio. 1.0 gives a plain close-delimited body.
    _ttsHttp->useHTTP10(true);
    _ttsHttp->setTimeout(8000);
    int code = _ttsHttp->GET();
    int contentLen = _ttsHttp->getSize();
    String ctype = _ttsHttp->header("Content-Type");
    ttsDiag("HTTP " + String(code) + " " + ctype + " len " + String(contentLen));
    if (code != HTTP_CODE_OK) {
        if (_voiceCallback) _voiceCallback("Error: TTS fetch HTTP " + String(code));
        return false;
    }

    if (!parseWavHeader()) {
        String hex;
        for (size_t i = 0; i < _ttsSniffLen; ++i) {
            char b[4];
            snprintf(b, sizeof(b), "%02X ", _ttsSniff[i]);
            hex += b;
        }
        Serial.println("[HA] Bad WAV, first bytes: " + hex);
        if (_voiceCallback) _voiceCallback("Error: not WAV [" + hex + "]");
        return false;
    }
    Serial.printf("[HA] WAV %u Hz, %s\n", _ttsRate, _ttsStereo ? "stereo" : "mono");
    logTtsHeap("fetched");

    // On CardputerADV the mic and speaker share the I2S BCK/WS pins and one
    // ES8311 codec. M5.Mic.end() powers the codec state machine and analog
    // section DOWN over I2C; only M5.Speaker.begin()'s enable callback powers the
    // DAC path back UP. It must run even if the speaker task was already started
    // for the mic handoff, so force a full end()/begin() cycle here.
    M5.Mic.end();
    M5.Speaker.end();
    M5.Speaker.begin();
    // M5Unified squares the volume in its gain math, so map the 0-100 setting
    // onto 0-90 -- past ~90 full-scale PCM clips into a fizz.
    uint8_t master = (uint8_t)(constrain(_config.getTtsVolume(), 0, 100) * 90 / 100);
    M5.Speaker.setVolume(master);
    M5.Speaker.setChannelVolume(kTtsChannel, 255);
    M5.Speaker.stop(kTtsChannel);

    for (int i = 0; i < kTtsBufCount; ++i) {
        _ttsChunks[i] = (uint8_t*)malloc(kTtsChunkBytes);
        if (!_ttsChunks[i]) {
            if (_voiceCallback) _voiceCallback("Error: TTS buffer alloc");
            return false;
        }
    }
    logTtsHeap("buffers");
    _ttsMinFree = ESP.getFreeHeap();
    _ttsMinBlock = ESP.getMaxAllocHeap();
    _ttsHeapSampleMs = millis();
    _ttsChunkIdx = 0;
    _ttsFillLen = 0;
    _ttsFedBytes = 0;
    _ttsPlaying = true;
    _ttsLastRxMs = millis();
    _ttsDeadline = millis() + 60000;
    ttsDiag("playing " + String(_ttsRate) + "Hz");
    return true;
}

bool HomeAssistantManager::parseWavHeader() {
    WiFiClient* stream = _ttsHttp ? _ttsHttp->getStreamPtr() : nullptr;
    if (!stream) return false;

    auto readExact = [&](uint8_t* dst, size_t n) -> bool {
        size_t got = 0;
        uint32_t start = millis();
        while (got < n) {
            if (stream->available()) {
                int r = stream->read(dst + got, n - got);
                if (r > 0) { got += r; start = millis(); continue; }
            }
            if (!stream->connected() && !stream->available()) return false;
            if (millis() - start > 5000) return false;
            delay(2);
        }
        return true;
    };

    uint8_t riff[12];
    _ttsSniffLen = 0;
    if (!readExact(riff, 12)) return false;
    memcpy(_ttsSniff, riff, 12);
    _ttsSniffLen = 12;
    if (memcmp(riff, "RIFF", 4) != 0 || memcmp(riff + 8, "WAVE", 4) != 0) return false;

    for (int guard = 0; guard < 12; ++guard) {
        uint8_t hdr[8];
        if (!readExact(hdr, 8)) return false;
        uint32_t sz = (uint32_t)hdr[4] | (hdr[5] << 8) | (hdr[6] << 16) | ((uint32_t)hdr[7] << 24);

        if (memcmp(hdr, "fmt ", 4) == 0) {
            uint8_t fmt[40];
            uint32_t take = sz > sizeof(fmt) ? sizeof(fmt) : sz;
            if (!readExact(fmt, take)) return false;
            uint16_t channels = (uint16_t)fmt[2] | (fmt[3] << 8);
            uint32_t rate = (uint32_t)fmt[4] | (fmt[5] << 8) | (fmt[6] << 16) | ((uint32_t)fmt[7] << 24);
            uint16_t bits = (uint16_t)fmt[14] | (fmt[15] << 8);
            if (bits != 16 || channels == 0 || channels > 2 || rate < 8000 || rate > 48000) return false;
            _ttsRate = rate;
            _ttsStereo = channels > 1;
            for (uint32_t skip = take; skip < sz; ++skip) { uint8_t b; if (!readExact(&b, 1)) return false; }
        } else if (memcmp(hdr, "data", 4) == 0) {
            return true; // PCM samples follow immediately
        } else {
            for (uint32_t skip = 0; skip < sz; ++skip) { uint8_t b; if (!readExact(&b, 1)) return false; }
        }
    }
    return false;
}

void HomeAssistantManager::pumpTtsWavStream() {
    if (millis() > _ttsDeadline) {
        Serial.println("[HA] TTS deadline hit");
        if (_voiceCallback) _voiceCallback("Error: TTS timed out");
        stopVoicePlayback();
        return;
    }

    // Free heap is a counter read; the largest block walks the heap, so sample
    // that one only a few times a second.
    uint32_t freeNow = ESP.getFreeHeap();
    if (freeNow < _ttsMinFree) _ttsMinFree = freeNow;
    if (millis() - _ttsHeapSampleMs > 250) {
        _ttsHeapSampleMs = millis();
        uint32_t block = ESP.getMaxAllocHeap();
        if (block < _ttsMinBlock) _ttsMinBlock = block;
    }

    WiFiClient* stream = _ttsHttp ? _ttsHttp->getStreamPtr() : nullptr;

    // Coalesce whatever bytes are buffered now into the current chunk.
    if (stream) {
        while (_ttsFillLen < kTtsChunkBytes) {
            size_t avail = stream->available();
            if (avail == 0) break;
            size_t room = kTtsChunkBytes - _ttsFillLen;
            int got = stream->read(_ttsChunks[_ttsChunkIdx] + _ttsFillLen, avail < room ? avail : room);
            if (got <= 0) break;
            _ttsFillLen += got;
            _ttsLastRxMs = millis();
        }
    }

    // HTTP/1.0 close-delimited: connected() drops as soon as the server finishes,
    // often with bytes still in flight, so hold the "done" call for a grace period.
    bool rxIdle = !stream || (!stream->connected() && stream->available() == 0);
    bool streamDone = rxIdle && (millis() - _ttsLastRxMs > 700);

    // Queue the chunk once it is full, or once the stream has drained.
    bool haveEnough = _ttsFillLen >= kTtsChunkBytes ||
                      (_ttsFillLen >= kTtsMinSubmit && rxIdle) ||
                      (_ttsFillLen > 0 && streamDone);
    if (haveEnough && M5.Speaker.isPlaying(kTtsChannel) < 2) {
        size_t n = _ttsFillLen & ~size_t{1};
        if (n && M5.Speaker.playRaw((const int16_t*)_ttsChunks[_ttsChunkIdx], n / 2,
                                    _ttsRate, _ttsStereo, 1, kTtsChannel, false)) {
            _ttsFedBytes += n;
            uint8_t odd = (_ttsFillLen & 1) ? _ttsChunks[_ttsChunkIdx][n] : 0;
            _ttsChunkIdx = (_ttsChunkIdx + 1) % kTtsBufCount;
            _ttsChunks[_ttsChunkIdx][0] = odd;
            _ttsFillLen = (_ttsFillLen & 1) ? 1 : 0;
        }
    }

    // isPlaying(channel) only counts queued slots; the no-arg bitmask also covers
    // the chunk currently playing out, so the tail is not cut.
    bool channelActive = (M5.Speaker.isPlaying() & (1u << kTtsChannel)) != 0;
    if (streamDone && _ttsFillLen < 2 && !channelActive) {
        Serial.printf("[HA] TTS finished, fed %u PCM bytes\n", _ttsFedBytes);
        ttsDiag("done " + String(_ttsFedBytes / 1024) + "K");
        if (_voiceCallback) _voiceCallback("");
        stopVoicePlayback();
    }
}

void HomeAssistantManager::freeTtsResources() {
    _ttsPlaying = false;
    _ttsFillLen = 0;
    _ttsChunkIdx = 0;
    if (_ttsHttp) { _ttsHttp->end(); delete _ttsHttp; _ttsHttp = nullptr; }
    if (_ttsClient) { delete _ttsClient; _ttsClient = nullptr; }
    for (int i = 0; i < kTtsBufCount; ++i) {
        if (_ttsChunks[i]) { free(_ttsChunks[i]); _ttsChunks[i] = nullptr; }
    }
}

void HomeAssistantManager::stopVoicePlayback() {
    if (_ttsMinFree != UINT32_MAX) {
        ttsDiag("heap play-low free=" + String(_ttsMinFree / 1024) + "K blk=" +
                String(_ttsMinBlock / 1024) + "K");
        _ttsMinFree = UINT32_MAX;
        _ttsMinBlock = UINT32_MAX;
    }
    M5.Speaker.stop(kTtsChannel);
    // Fully release the codec/I2S so the next mic session (shared pins on
    // CardputerADV) can reconfigure the ES8311 from a clean state.
    M5.Speaker.end();
    freeTtsResources();
    logTtsHeap("after");

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

void HomeAssistantManager::setLightBrightness(const String& entity_id, int percent) {
    if (!_isConnected || !_isAuthenticated) return;

    // light.turn_on with brightness_pct 0 turns the light off, so the bottom
    // of the range needs no separate service.
    JsonDocument doc;
    doc["id"] = _nextMsgId++;
    doc["type"] = "call_service";
    doc["domain"] = "light";
    doc["service"] = "turn_on";
    JsonObject target = doc["target"].to<JsonObject>();
    target["entity_id"] = entity_id;
    JsonObject data = doc["service_data"].to<JsonObject>();
    data["brightness_pct"] = constrain(percent, 0, 100);

    String payload;
    serializeJson(doc, payload);
    _ws.sendTXT(payload);
    Serial.println("[HA] Sent light brightness: " + payload);
}

void HomeAssistantManager::adjustEntity(const String& entity_id, int direction) {
    if (!_isAuthenticated) return;
    
    Entity entity = _entityManager.getEntity(entity_id);
    if (entity.id == "") return;

    if (entity.domain == "media_player") {
        MediaPlayerState ms;
        if (!_entityManager.getMediaPlayerState(entity_id, ms)) return;
        
        float step = 0.05f; // 5%
        float newVol = ms.volumeLevel + (direction > 0 ? step : -step);
        if (newVol < 0.0f) newVol = 0.0f;
        if (newVol > 1.0f) newVol = 1.0f;
        setMediaVolume(entity_id, newVol);
        
        // Optimistically update the entity locally so rapid presses reflect instantly
        _entityManager.updateMediaAttributes(entity_id, ms.title, ms.artist, ms.album, ms.duration, ms.position, newVol, ms.isVolumeMuted);
    } else if (entity.domain == "light") {
        JsonDocument doc;
        doc["id"] = _nextMsgId++;
        doc["type"] = "call_service";
        doc["domain"] = "light";
        doc["service"] = "turn_on";
        
        JsonObject target = doc["target"].to<JsonObject>();
        target["entity_id"] = entity_id;
        
        JsonObject service_data = doc["service_data"].to<JsonObject>();
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
    // Not the payload: it carries the alarm or lock code, and serial logs get pasted into issues.
    Serial.println("[HA] Sent secure call_service: " + domain + "." + service + " on " + entity_id);
}

// One entity from a state object, whether it came from the initial download or
// a state_changed event. Returns false when the entity is filtered out.
bool HomeAssistantManager::applyEntityState(const String& entity_id, JsonVariantConst stateObj) {
    int dot = entity_id.indexOf('.');
    if (dot < 0 || !_entityManager.isSupportedDomain(entity_id.substring(0, dot))) return false;

    JsonVariantConst attrs = stateObj["attributes"];
    if (entity_id.startsWith("sensor.")) {
        String device_class = attrs["device_class"] | "";
        if (device_class != "temperature" && device_class != "humidity") return false;
    }

    String state = stateObj["state"].as<String>();
    String friendly_name = attrs["friendly_name"] | "";
    _entityManager.updateEntity(entity_id, state, friendly_name);

    if (entity_id.startsWith("media_player.")) {
        _entityManager.updateMediaAttributes(entity_id,
            attrs["media_title"] | "",
            attrs["media_artist"] | "",
            attrs["media_album_name"] | "",
            attrs["media_duration"] | 0.0f,
            attrs["media_position"] | 0.0f,
            attrs["volume_level"] | 0.0f,
            attrs["is_volume_muted"] | false);
    } else if (entity_id.startsWith("climate.")) {
        ClimateState climate;
        readClimateAttributes(attrs, climate);
        _entityManager.updateClimateAttributes(entity_id, climate);
    } else if (entity_id.startsWith("light.")) {
        LightState light;
        readLightAttributes(attrs, light);
        _entityManager.updateLightAttributes(entity_id, light);
    }
    return true;
}

void HomeAssistantManager::startInitialStates() {
    if (_statesPhase != StatesPhase::Idle) return;

    String url = _config.getHAUrl();
    if (url.endsWith("/")) url = url.substring(0, url.length() - 1);
    url += "/api/states";

    _statesPhase = StatesPhase::Loading;
    _statesStartMs = millis();
    _statesDeadline = _statesStartMs + kStatesTimeoutMs;
    _statesCount = 0;
    _statesBrace = 0;
    _statesInArray = false;
    _statesInString = false;
    _statesEscape = false;
    _statesObj = "";
    _statesObj.reserve(2048);

    _statesHttp = new HTTPClient();
    if (url.startsWith("https")) {
        _statesSecureClient = new WiFiClientSecure();
        _statesSecureClient->setInsecure();
        _statesHttp->begin(*_statesSecureClient, url);
    } else {
        _statesPlainClient = new WiFiClient();
        _statesHttp->begin(*_statesPlainClient, url);
    }
    _statesHttp->addHeader("Authorization", "Bearer " + _config.getHAToken());
    _statesHttp->addHeader("Content-Type", "application/json");

    Serial.println("[HA] Fetching initial states: " + url);
    // Blocks only until the response headers arrive; pumpInitialStates() reads the body.
    int httpCode = _statesHttp->GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.println("[HA] Initial states GET failed: HTTP " + String(httpCode) + " " +
                       _statesHttp->errorToString(httpCode));
        finishInitialStates("HTTP error");
        return;
    }
    _statesStream = _statesHttp->getStreamPtr();
}

void HomeAssistantManager::pumpInitialStates() {
    if (_statesPhase != StatesPhase::Loading) return;
    if (millis() > _statesDeadline) {
        finishInitialStates("timed out");
        return;
    }

    uint32_t budgetEnd = millis() + kStatesPumpMs;
    uint8_t buf[256];
    while (millis() < budgetEnd) {
        int avail = _statesStream->available();
        if (avail <= 0) {
            if (!_statesStream->connected()) finishInitialStates("stream closed early");
            return; // nothing waiting: hand the loop back rather than spin here
        }
        int n = _statesStream->read(buf, avail < (int)sizeof(buf) ? avail : (int)sizeof(buf));
        for (int i = 0; i < n; i++) {
            if (feedInitialStatesByte((char)buf[i])) {
                finishInitialStates("complete");
                return;
            }
        }
    }
}

// Tracks the JSON array one byte at a time. Returns true on the ']' that closes it.
bool HomeAssistantManager::feedInitialStatesByte(char c) {
    if (!_statesInArray) {
        if (c == '[') _statesInArray = true;
        return false;
    }
    if (_statesBrace == 0) {
        if (c == ']') return true;
        if (c != '{') return false; // separators between objects
        _statesObj = "{";
        _statesBrace = 1;
        _statesInString = false;
        _statesEscape = false;
        _statesDecided = false;
        _statesSkipObject = false;
        return false;
    }

    bool escaped = _statesEscape;
    _statesEscape = (c == '\\' && !escaped);
    if (c == '"' && !escaped) _statesInString = !_statesInString;
    if (!_statesInString) {
        if (c == '{') _statesBrace++;
        else if (c == '}') _statesBrace--;
    }
    if (!_statesSkipObject) {
        _statesObj += c;
        if (!_statesDecided && !_statesInString) decideInitialStatesObject();
    }
    if (_statesBrace == 0) {
        if (!_statesSkipObject) parseInitialStatesObject();
        _statesObj = "";
    }
    return false;
}

// Settles keep-or-skip as soon as the entity_id value is complete, so an
// unsupported entity is never accumulated or parsed. Home Assistant writes
// entity_id first; if it ever does not, the object is simply parsed in full.
void HomeAssistantManager::decideInitialStatesObject() {
    int key = _statesObj.indexOf("\"entity_id\"");
    if (key < 0) {
        if (_statesObj.length() > 48) _statesDecided = true;
        return;
    }
    int open = _statesObj.indexOf('"', key + 11);
    if (open < 0) return;
    int close = _statesObj.indexOf('"', open + 1);
    if (close < 0) return;
    _statesDecided = true;
    String id = _statesObj.substring(open + 1, close);
    int dot = id.indexOf('.');
    _statesSkipObject = dot < 0 || !_entityManager.isSupportedDomain(id.substring(0, dot));
}

void HomeAssistantManager::parseInitialStatesObject() {
    JsonDocument doc;
    if (deserializeJson(doc, _statesObj)) return;
    String entity_id = doc["entity_id"] | "";
    if (applyEntityState(entity_id, doc.as<JsonVariantConst>())) _statesCount++;
}

void HomeAssistantManager::finishInitialStates(const char* why) {
    Serial.printf("[HA] Initial states: %d entities kept, %lu ms (%s)\n",
                  _statesCount, (unsigned long)(millis() - _statesStartMs), why);
    if (_statesHttp) {
        _statesHttp->end();
        delete _statesHttp;
        _statesHttp = nullptr;
    }
    delete _statesSecureClient;
    _statesSecureClient = nullptr;
    delete _statesPlainClient;
    _statesPlainClient = nullptr;
    _statesStream = nullptr;
    _statesObj = String();
    _statesPhase = StatesPhase::Done;
}

void HomeAssistantManager::setClimateTemperature(const String& entity_id, float temp) {
    if (!_isConnected || !_isAuthenticated) return;
    JsonDocument doc;
    doc["id"] = _nextMsgId++;
    doc["type"] = "call_service";
    doc["domain"] = "climate";
    doc["service"] = "set_temperature";
    doc["target"]["entity_id"] = entity_id;
    doc["service_data"]["temperature"] = temp;
    String payload;
    serializeJson(doc, payload);
    _ws.sendTXT(payload);
}

void HomeAssistantManager::setClimateTempRange(const String& entity_id, float low, float high) {
    if (!_isConnected || !_isAuthenticated) return;
    JsonDocument doc;
    doc["id"] = _nextMsgId++;
    doc["type"] = "call_service";
    doc["domain"] = "climate";
    doc["service"] = "set_temperature";
    doc["target"]["entity_id"] = entity_id;
    doc["service_data"]["target_temp_low"] = low;
    doc["service_data"]["target_temp_high"] = high;
    String payload;
    serializeJson(doc, payload);
    _ws.sendTXT(payload);
}

void HomeAssistantManager::setHvacMode(const String& entity_id, const String& mode) {
    if (!_isConnected || !_isAuthenticated) return;
    JsonDocument doc;
    doc["id"] = _nextMsgId++;
    doc["type"] = "call_service";
    doc["domain"] = "climate";
    doc["service"] = "set_hvac_mode";
    doc["target"]["entity_id"] = entity_id;
    doc["service_data"]["hvac_mode"] = mode;
    String payload;
    serializeJson(doc, payload);
    _ws.sendTXT(payload);
}
