#ifndef SCROLL_REPEATER_H
#define SCROLL_REPEATER_H

#include "ConfigManager.h"
#include "KeyboardManager.h"

class ScrollRepeater {
public:
    explicit ScrollRepeater(ConfigManager& config);

    // Returns -1 for up, 1 for down, and 0 when no movement is due.
    int update(KeyboardManager& keyboard);
    
    // Returns -1 for left, 1 for right, and 0 when no movement is due.
    int updateLeftRight(KeyboardManager& keyboard);

private:
    ConfigManager& _config;
    uint32_t _holdStarted = 0;
    uint32_t _lastMove = 0;
};

#endif // SCROLL_REPEATER_H
