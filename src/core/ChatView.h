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
    void receiveResponse(const String& response);

private:
    HomeAssistantManager& _homeAssistant;
    std::vector<String> _messages;
    String _input;

    void addMessage(const String& message);
};

#endif // CHAT_VIEW_H