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
    bool wasEscPressed() const;
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
    bool wasGoPressed() const;
    // When false, a short press of the GO (top) button no longer aliases Enter.
    void setGoActsAsEnter(bool v) { _goActsAsEnter = v; }
    bool isPlusHeld() const;
    bool isMinusHeld() const;
    bool isCtrlHeld() const;
    bool isEscHeld() const;
    bool isCharHeld(char character) const;
    
    bool hasActivity() const;
    
    // Returns any new character keys that were pressed this frame
    std::vector<char> getNewChars() const;
    
private:
    // The driver substitutes a key's shifted character whenever Ctrl, Shift or
    // caps lock is active, so ';' arrives as ':' under Ctrl. Every alias below
    // therefore has to be matched in both forms.
    bool charHeldNow(char plain, char shifted) const;
    bool charWasHeld(char plain, char shifted) const;
    bool charPressed(char plain, char shifted) const;
    bool charReleased(char plain, char shifted) const;

    Keyboard_Class::KeysState _lastStatus;
    Keyboard_Class::KeysState _currentStatus;
    bool _goActsAsEnter = true;
};

#endif // KEYBOARD_MANAGER_H
