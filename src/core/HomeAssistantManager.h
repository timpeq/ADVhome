#ifndef HOME_ASSISTANT_MANAGER_H
#define HOME_ASSISTANT_MANAGER_H

#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "ConfigManager.h"
#include "EntityManager.h"
#include <Arduino.h>
#include <functional>
#include <vector>

class AudioFileSource;
class AudioGenerator;
class AudioOutputI2S;

class HomeAssistantManager {
public:
    using ConversationCallback = std::function<void(const String&)>;
    using VoiceCallback = std::function<void(const String&)>;

    HomeAssistantManager(ConfigManager& config, EntityManager& entityManager);
    
    void begin();
    void update();
    
    void fetchInitialStates();
    
    void callService(const String& domain, const String& service, const String& entity_id);
    void callSecureService(const String& domain, const String& service, const String& entity_id, const String& code);
    void setMediaVolume(const String& entity_id, float volume);
    void seekMedia(const String& entity_id, float position);
    void toggleMute(const String& entity_id, bool is_muted);
    void adjustEntity(const String& entity_id, int direction);
    void sendConversation(const String& text);
    void setConversationCallback(ConversationCallback callback) { _conversationCallback = callback; }
    bool startVoicePipeline();
    void sendVoiceAudio(const int16_t* samples, size_t sampleCount);
    void finishVoicePipeline();
    void queueVoiceResponse(const String& url, const String& mimeType);
    bool isVoiceReady() const { return _voiceBinaryHandlerId >= 0; }
    void setVoiceCallback(VoiceCallback callback) { _voiceCallback = callback; }
    
    bool isConnected() const { return _isConnected; }
    bool isAuthenticated() const { return _isAuthenticated; }
    bool isTtsTransitioning() const { return _ttsTransitioning; }
    String getVersion() const { return _haVersion; }
    
private:
    ConfigManager& _config;
    EntityManager& _entityManager;
    WebSocketsClient _ws;
    
    bool _isConnected = false;
    bool _isAuthenticated = false;
    String _haVersion = "Unknown";
    uint32_t _nextMsgId = 100;
    uint32_t _conversationRequestId = 0;
    String _conversationId;
    ConversationCallback _conversationCallback;
    uint32_t _voiceRequestId = 0;
    int _voiceBinaryHandlerId = -1;
    std::vector<uint8_t> _voicePacket;
    VoiceCallback _voiceCallback;
    String _ttsUrl;
    String _ttsMimeType;
    bool _ttsPending = false;
    bool _ttsTransitioning = false;
    bool _voiceRunFinished = false;
    bool _voiceReceivedText = false;
    AudioFileSource* _ttsSource = nullptr;
    AudioGenerator* _ttsGenerator = nullptr;
    AudioOutputI2S* _ttsOutput = nullptr;
    
    void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
    void processVoiceResponse();
    void stopVoicePlayback();
    void resetWebSocket();
};

#endif // HOME_ASSISTANT_MANAGER_H
