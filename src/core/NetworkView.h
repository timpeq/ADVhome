#ifndef NETWORK_VIEW_H
#define NETWORK_VIEW_H

#include "View.h"
#include <functional>
#include <vector>

// A read-only summary of one connection (Wi-Fi or Home Assistant) plus a single
// destructive action to reconfigure it. The action is deliberately behind a
// confirm step: on a device with no pointer and a small screen, an accidental
// Enter that forgets your credentials is a bad afternoon.
class NetworkView : public View {
public:
    using InfoProvider = std::function<void(std::vector<String>&)>;

    NetworkView(const char* title,
                const char* actionLabel,
                InfoProvider info,
                std::function<void()> action);

    void onEnter() override;
    void onExit() override;
    void draw(DisplayManager& display) override;
    bool handleInput(KeyboardManager& keyboard) override;

private:
    const char* _title;
    const char* _actionLabel;
    InfoProvider _info;
    std::function<void()> _action;

    std::vector<String> _lines;
    bool _armed = false;
    uint32_t _armedAt = 0;
};

#endif // NETWORK_VIEW_H
