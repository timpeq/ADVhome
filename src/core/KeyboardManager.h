#ifndef KEYBOARD_MANAGER_H
#define KEYBOARD_MANAGER_H

#include <M5Cardputer.h>
#include <vector>

class KeyboardManager {
public:
    void update();
    
    bool wasEnterPressed() const;
    bool wasSpacePressed() const;
    bool wasBackspacePressed() const;
    bool wasTabPressed() const;
    bool wasUpPressed() const;
    bool wasDownPressed() const;
    bool wasLeftPressed() const;
    bool wasRightPressed() const;
    bool wasPlusPressed() const;
    bool wasMinusPressed() const;
    bool wasLeftReleased() const;
    bool wasRightReleased() const;
    bool isUpHeld() const;
    bool isDownHeld() const;
    bool isLeftHeld() const;
    bool isRightHeld() const;
    bool isEnterHeld() const;
    bool isGoHeld() const;
    bool isPlusHeld() const;
    bool isMinusHeld() const;
    bool isCtrlHeld() const;
    bool isEscHeld() const;
    bool isCharHeld(char character) const;
    
    bool hasActivity() const;
    
    // Returns any new character keys that were pressed this frame
    std::vector<char> getNewChars() const;
    
private:
    Keyboard_Class::KeysState _lastStatus;
    Keyboard_Class::KeysState _currentStatus;
};

#endif // KEYBOARD_MANAGER_H
