#include "ScrollRepeater.h"

ScrollRepeater::ScrollRepeater(ConfigManager& config) : _config(config) {}

int ScrollRepeater::update(KeyboardManager& keyboard) {
    return update(keyboard.wasUpPressed(), keyboard.isUpHeld(), keyboard.wasDownPressed(), keyboard.isDownHeld());
}

int ScrollRepeater::updateLeftRight(KeyboardManager& keyboard) {
    return update(keyboard.wasLeftPressed(), keyboard.isLeftHeld(), keyboard.wasRightPressed(), keyboard.isRightHeld());
}

int ScrollRepeater::updatePlusMinus(KeyboardManager& keyboard) {
    return update(keyboard.wasMinusPressed(), keyboard.isMinusHeld(), keyboard.wasPlusPressed(), keyboard.isPlusHeld());
}

int ScrollRepeater::update(bool wasDecPressed, bool isDecHeld, bool wasIncPressed, bool isIncHeld) {
    uint32_t now = millis();

    if (wasDecPressed) {
        _holdStarted = now;
        _lastMove = now;
        return -1;
    }
    if (wasIncPressed) {
        _holdStarted = now;
        _lastMove = now;
        return 1;
    }

    if (!isDecHeld && !isIncHeld) {
        _holdStarted = 0;
        return 0;
    }

    if (_holdStarted != 0 &&
        now - _holdStarted >= (uint32_t)_config.getScrollDelay() &&
        now - _lastMove >= (uint32_t)_config.getScrollSpeed()) {
        _lastMove = now;
        return isDecHeld ? -1 : 1;
    }

    return 0;
}
