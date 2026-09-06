#include "KeyboardManager.h"

void KeyboardManager::update() {
    _lastStatus = _currentStatus;
    _currentStatus = M5Cardputer.Keyboard.keysState();
}

bool KeyboardManager::wasEnterPressed() const {
    return (_currentStatus.enter && !_lastStatus.enter) || 
           (_currentStatus.space && !_lastStatus.space) ||
           M5Cardputer.BtnA.wasPressed();
}

bool KeyboardManager::wasBackspacePressed() const {
    return _currentStatus.del && !_lastStatus.del;
}

std::vector<char> KeyboardManager::getNewChars() const {
    std::vector<char> new_chars;
    for (char c : _currentStatus.word) {
        bool was_pressed = false;
        for (char lc : _lastStatus.word) {
            if (c == lc) { 
                was_pressed = true; 
                break; 
            }
        }
        if (!was_pressed) {
            new_chars.push_back(c);
        }
    }
    return new_chars;
}
