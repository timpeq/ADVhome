#ifndef CHAT_VIEW_H
#define CHAT_VIEW_H

#include "View.h"
#include "HomeAssistantManager.h"
#include <vector>

class ChatView : public View {
public:
    explicit ChatView(HomeAssistantManager& homeAssistant);

    void draw(DisplayManager& display) override;
    bool handleInput(KeyboardManager& keyboard) override;
    void onEnter() override;
    void receiveResponse(const String& response);
    void receiveVoiceEvent(const String& event);

private:
    HomeAssistantManager& _homeAssistant;
    std::vector<String> _messages;
    String _input;
    bool _voiceRecording = false;
    String _voiceStatus;
    uint32_t _lastVoiceCapture = 0;
    int16_t _voiceSamples[320] = {};

    void addMessage(const String& message);
    void startVoiceRecording();
    void stopVoiceRecording();
    void captureVoiceAudio();
};

#endif // CHAT_VIEW_H