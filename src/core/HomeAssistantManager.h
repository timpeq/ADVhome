#ifndef HOME_ASSISTANT_MANAGER_H
#define HOME_ASSISTANT_MANAGER_H

#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "ConfigManager.h"
#include "EntityManager.h"
#include <Arduino.h>
#include <functional>
#include <vector>

class HTTPClient;
class WiFiClient;

class HomeAssistantManager {
public:
    using ConversationCallback = std::function<void(const String&)>;
    using VoiceCallback = std::function<void(const String&)>;
    using PipelinesCallback = std::function<void()>;

    struct VoicePipeline {
        String id;
        String name;
        String ttsEngine;
        String ttsVoice;
        String ttsLanguage;
    };

    HomeAssistantManager(ConfigManager& config, EntityManager& entityManager);
    
    void begin();
    void update();
    
    void fetchInitialStates();
    
    void callService(const String& domain, const String& service, const String& entity_id);
    void callSecureService(const String& domain, const String& service, const String& entity_id, const String& code);
    
    // Media controls
    void setMediaVolume(const String& entity_id, float volume);
    void toggleMute(const String& entity_id, bool is_muted);
    void seekMedia(const String& entity_id, float position);

    // Light controls
    void setLightBrightness(const String& entity_id, int percent);
    
    // Climate controls
    void setClimateTemperature(const String& entity_id, float temp);
    void setClimateTempRange(const String& entity_id, float low, float high);
    void setHvacMode(const String& entity_id, const String& mode);
    
    void adjustEntity(const String& entity_id, int direction);
    void sendConversation(const String& text);
    void setConversationCallback(ConversationCallback callback) { _conversationCallback = callback; }
    bool startVoicePipeline();
    void sendVoiceAudio(const int16_t* samples, size_t sampleCount);
    void finishVoicePipeline();
    bool isVoiceReady() const { return _voiceBinaryHandlerId >= 0; }
    void setVoiceCallback(VoiceCallback callback) { _voiceCallback = callback; }

    void requestPipelineList();
    const std::vector<VoicePipeline>& getPipelines() const { return _pipelines; }
    String getPreferredPipelineId() const { return _preferredPipelineId; }
    void setPipelinesCallback(PipelinesCallback callback) { _pipelinesCallback = callback; }

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
    uint32_t _pipelineListRequestId = 0;
    std::vector<VoicePipeline> _pipelines;
    String _preferredPipelineId;
    PipelinesCallback _pipelinesCallback;
    String _pendingTtsText; // conversation reply awaiting our own WAV synthesis
    bool _ttsTransitioning = false;
    bool _voiceRunFinished = false;
    bool _voiceReceivedText = false;

    // TTS WAV playback: stream the HA tts_proxy WAV over plain HTTP and feed the
    // PCM straight to M5.Speaker (which owns the CardputerADV ES8311 codec).
    static constexpr int kTtsChannel = 7;
    static constexpr size_t kTtsChunkBytes = 6144; // ~140 ms mono @ 22 kHz
    static constexpr size_t kTtsMinSubmit = 2048;  // don't queue fragments smaller than this unless draining
    static constexpr int kTtsBufCount = 5;         // M5.Speaker holds <=3 (2 queued + 1 playing); extra margin
    HTTPClient* _ttsHttp = nullptr;
    WiFiClient* _ttsClient = nullptr;
    uint8_t* _ttsChunks[kTtsBufCount] = {nullptr};
    int _ttsChunkIdx = 0;
    size_t _ttsFillLen = 0; // bytes accumulated in _ttsChunks[_ttsChunkIdx], not yet queued
    bool _ttsPlaying = false;
    uint32_t _ttsRate = 22050;
    bool _ttsStereo = false;
    uint32_t _ttsFedBytes = 0;
    uint32_t _ttsDeadline = 0;
    uint32_t _ttsLastRxMs = 0;
    uint8_t _ttsSniff[16] = {0};
    size_t _ttsSniffLen = 0;

    // Heap low-water marks across one spoken reply, reported when it stops.
    uint32_t _ttsMinFree = UINT32_MAX;
    uint32_t _ttsMinBlock = UINT32_MAX;
    uint32_t _ttsHeapSampleMs = 0;

    void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
    void logTtsHeap(const char* stage);
    const VoicePipeline* activePipeline() const;
    String plainHttpBase() const;
    String requestWavTtsUrl(const String& text);
    void processVoiceResponse();
    void ttsDiag(const String& s); // serial always; chat log only when TTS Debug is on
    bool startTtsWavStream(const String& url);
    bool parseWavHeader();
    void pumpTtsWavStream();
    void stopVoicePlayback();
    void freeTtsResources();
    void resetWebSocket();
};

#endif // HOME_ASSISTANT_MANAGER_H
