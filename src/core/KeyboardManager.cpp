#include <algorithm>
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
    return (_currentStatus.del && !_lastStatus.del) || 
           (_currentStatus.esc && !_lastStatus.esc) ||
           (std::find(_currentStatus.word.begin(), _currentStatus.word.end(), '`') != _currentStatus.word.end() &&
            std::find(_lastStatus.word.begin(), _lastStatus.word.end(), '`') == _lastStatus.word.end()) ||
           (std::find(_currentStatus.word.begin(), _currentStatus.word.end(), '~') != _currentStatus.word.end() &&
            std::find(_lastStatus.word.begin(), _lastStatus.word.end(), '~') == _lastStatus.word.end());
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

bool KeyboardManager::wasTabPressed() const {
    return _currentStatus.tab && !_lastStatus.tab;
}

bool KeyboardManager::wasUpPressed() const {
    return (_currentStatus.up && !_lastStatus.up) || 
           (std::find(_currentStatus.word.begin(), _currentStatus.word.end(), ';') != _currentStatus.word.end() &&
            std::find(_lastStatus.word.begin(), _lastStatus.word.end(), ';') == _lastStatus.word.end());
}

bool KeyboardManager::wasDownPressed() const {
    return (_currentStatus.down && !_lastStatus.down) || 
           (std::find(_currentStatus.word.begin(), _currentStatus.word.end(), '.') != _currentStatus.word.end() &&
            std::find(_lastStatus.word.begin(), _lastStatus.word.end(), '.') == _lastStatus.word.end());
}

bool KeyboardManager::wasLeftPressed() const {
    return (_currentStatus.left && !_lastStatus.left) || 
           (std::find(_currentStatus.word.begin(), _currentStatus.word.end(), ',') != _currentStatus.word.end() &&
            std::find(_lastStatus.word.begin(), _lastStatus.word.end(), ',') == _lastStatus.word.end());
}

bool KeyboardManager::wasRightPressed() const {
    return (_currentStatus.right && !_lastStatus.right) || 
           (std::find(_currentStatus.word.begin(), _currentStatus.word.end(), '/') != _currentStatus.word.end() &&
            std::find(_lastStatus.word.begin(), _lastStatus.word.end(), '/') == _lastStatus.word.end());
}

bool KeyboardManager::isUpHeld() const {
    return _currentStatus.up ||
           std::find(_currentStatus.word.begin(), _currentStatus.word.end(), ';') != _currentStatus.word.end();
}

bool KeyboardManager::isDownHeld() const {
    return _currentStatus.down ||
           std::find(_currentStatus.word.begin(), _currentStatus.word.end(), '.') != _currentStatus.word.end();
}
