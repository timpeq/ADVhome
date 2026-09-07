#include "ScrollRepeater.h"

ScrollRepeater::ScrollRepeater(ConfigManager& config) : _config(config) {}

int ScrollRepeater::update(KeyboardManager& keyboard) {
    uint32_t now = millis();
    bool upHeld = keyboard.isUpHeld();
    bool downHeld = keyboard.isDownHeld();

    if (keyboard.wasUpPressed()) {
        _holdStarted = now;
        _lastMove = now;
        return -1;
    }
    if (keyboard.wasDownPressed()) {
        _holdStarted = now;
        _lastMove = now;
        return 1;
    }

    if (!upHeld && !downHeld) {
        _holdStarted = 0;
        return 0;
    }

    if (_holdStarted != 0 &&
        now - _holdStarted >= (uint32_t)_config.getScrollDelay() &&
        now - _lastMove >= (uint32_t)_config.getScrollSpeed()) {
        _lastMove = now;
        return upHeld ? -1 : 1;
    }

    return 0;
}
