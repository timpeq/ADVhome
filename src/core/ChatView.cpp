#include "ChatView.h"
#include "TextScroller.h"
#include <M5Unified.h>

static constexpr uint32_t voiceSampleRate = 16000;
static constexpr size_t voiceChunkSamples = 320;

ChatView::ChatView(HomeAssistantManager& homeAssistant) : _homeAssistant(homeAssistant) {}

void ChatView::addMessage(const String& message) {
    _messages.push_back(message);
    while (_messages.size() > 8) _messages.erase(_messages.begin());
}

void ChatView::receiveResponse(const String& response) {
    addMessage("HA: " + response);
}

void ChatView::receiveVoiceEvent(const String& event) {
    // Empty event = clear the transient status line (turn finished).
    _voiceStatus = event;
    // Conversation turns and errors are kept in the log; the "TTS:" diagnostic
    // lines only arrive at all when the TTS Debug setting is on.
    if (event.startsWith("You:") || event.startsWith("HA:") || event.startsWith("Error:") ||
        event.startsWith("TTS:")) {
        addMessage(event);
    }
}

void ChatView::startVoiceRecording() {
    if (_voiceRecording) return;

    Serial.printf("[VOICE] GO hold detected, free heap: %u\n", ESP.getFreeHeap());
    if (!_homeAssistant.startVoicePipeline()) return;

    Serial.println("[VOICE] Starting microphone");
    if (!M5.Speaker.isRunning()) {
        Serial.println("[VOICE] Starting speaker for audio handoff");
        M5.Speaker.begin();
    }
    
    if (!M5.Mic.begin()) {
        Serial.println("[VOICE] Microphone begin failed");
        _homeAssistant.finishVoicePipeline();
        receiveVoiceEvent("Error: microphone unavailable");
        return;
    }
    Serial.println("[VOICE] Microphone started");
    _voiceRecording = true;
    _lastVoiceCapture = 0;
}

void ChatView::stopVoiceRecording() {
    if (!_voiceRecording) return;

    Serial.println("[VOICE] Finishing microphone capture");
    _homeAssistant.finishVoicePipeline();
    _voiceRecording = false;
    // Release the mic: on CardputerADV it shares the I2S BCK/WS pins and the
    // ES8311 codec with the speaker, so TTS playback needs it fully stopped.
    M5.Mic.end();
    Serial.println("[VOICE] Microphone stopped");
}

void ChatView::captureVoiceAudio() {
    if (!_voiceRecording || !_homeAssistant.isVoiceReady()) return;
    if (millis() - _lastVoiceCapture < 20) return;

    if (M5.Mic.record(_voiceSamples, voiceChunkSamples, voiceSampleRate)) {
        _lastVoiceCapture = millis();
        static bool loggedFirstChunk = false;
        if (!loggedFirstChunk) {
            Serial.println("[VOICE] Captured first audio chunk");
            loggedFirstChunk = true;
        }
        _homeAssistant.sendVoiceAudio(_voiceSamples, voiceChunkSamples);
    }
}

void ChatView::draw(DisplayManager& display) {
    auto canvas = display.getCanvas();
    canvas->setTextSize(1);
    canvas->setTextColor(TFT_LIGHTGREY);

    if (_messages.empty()) {
        // A hint, not a message: nothing in _messages is anything but a real
        // turn, and this is short enough that it never has to scroll.
        canvas->setCursor(5, 23);
        canvas->setTextColor(TFT_DARKGREY);
        canvas->print("Type, or hold GO to speak.");
    }

    int firstMessage = _messages.size() > 6 ? _messages.size() - 6 : 0;
    int y = 23;
    for (size_t i = firstMessage; i < _messages.size(); i++) {
        String message = _messages[i];
        canvas->setCursor(5, y);
        canvas->setTextColor(message.startsWith("You:") ? TFT_CYAN : TFT_WHITE);
        canvas->print(TextScroller::visible(message, 38));
        y += 14;
    }

    if (!_voiceStatus.isEmpty()) {
        canvas->setCursor(5, 105);
        canvas->setTextColor(TFT_YELLOW);
        canvas->print(TextScroller::visible(_voiceStatus, 38));
    }
    canvas->drawLine(0, 114, 239, 114, TFT_DARKGREY);
    canvas->setCursor(5, 120);
    canvas->setTextColor(TFT_GREEN);
    canvas->print("> ");
    canvas->setTextColor(TFT_WHITE);
    canvas->print(TextScroller::visible(_input, 36));
}

bool ChatView::handleInput(KeyboardManager& keyboard) {
    bool handled = false;

    if (keyboard.isGoHeld()) {
        if (!(_voiceRecording)) {
            startVoiceRecording();
            return true;
        }
        captureVoiceAudio();
        return false;
    }
    if (_voiceRecording) {
        stopVoiceRecording();
        return true;
    }

    for (char character : keyboard.getNewChars()) {
        if (_input.length() < 80 && character >= 32 && character != 127) {
            _input += character;
            handled = true;
        }
    }

    if (keyboard.wasBackspacePressed() && !_input.isEmpty()) {
        _input.remove(_input.length() - 1);
        handled = true;
    }

    if (keyboard.wasEnterPressed() && !_input.isEmpty()) {
        String message = _input;
        _input = "";
        addMessage("You: " + message);
        _homeAssistant.sendConversation(message);
        handled = true;
    }

    return handled;
}