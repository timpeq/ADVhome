#ifndef KEYBOARD_MANAGER_H
#define KEYBOARD_MANAGER_H

#include <M5Cardputer.h>
#include <vector>

class KeyboardManager {
public:
    void update();
    
    bool wasEnterPressed() const;
    bool wasBackspacePressed() const;
    bool wasTabPressed() const;
    bool wasUpPressed() const;
    bool wasDownPressed() const;
    bool wasLeftPressed() const;
    bool wasRightPressed() const;
    
    // Returns any new character keys that were pressed this frame
    std::vector<char> getNewChars() const;
    
private:
    Keyboard_Class::KeysState _lastStatus;
    Keyboard_Class::KeysState _currentStatus;
};

#endif // KEYBOARD_MANAGER_H
