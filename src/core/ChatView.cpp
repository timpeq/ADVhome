#include "ChatView.h"

ChatView::ChatView(HomeAssistantManager& homeAssistant) : _homeAssistant(homeAssistant) {}

void ChatView::addMessage(const String& message) {
    _messages.push_back(message);
    while (_messages.size() > 8) _messages.erase(_messages.begin());
}

void ChatView::receiveResponse(const String& response) {
    addMessage("HA: " + response);
}

void ChatView::draw(DisplayManager& display) {
    auto canvas = display.getCanvas();
    canvas->setTextSize(1);
    canvas->setTextColor(TFT_LIGHTGREY);

    int firstMessage = _messages.size() > 6 ? _messages.size() - 6 : 0;
    int y = 23;
    for (size_t i = firstMessage; i < _messages.size(); i++) {
        String message = _messages[i];
        if (message.length() > 38) message = message.substring(0, 35) + "...";
        canvas->setCursor(5, y);
        canvas->setTextColor(message.startsWith("You:") ? TFT_CYAN : TFT_WHITE);
        canvas->print(message);
        y += 14;
    }

    canvas->drawLine(0, 114, 239, 114, TFT_DARKGREY);
    canvas->setCursor(5, 120);
    canvas->setTextColor(TFT_GREEN);
    canvas->print("> ");
    String visibleInput = _input;
    if (visibleInput.length() > 36) visibleInput = visibleInput.substring(visibleInput.length() - 36);
    canvas->setTextColor(TFT_WHITE);
    canvas->print(visibleInput);
}

bool ChatView::handleInput(KeyboardManager& keyboard) {
    bool handled = false;

    for (char character : keyboard.getNewChars()) {
        if (_input.length() < 80 && character >= 32) {
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