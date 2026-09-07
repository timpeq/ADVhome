#ifndef SECURITY_CODE_MODAL_H
#define SECURITY_CODE_MODAL_H

#include "KeyboardManager.h"
#include <functional>

class SecurityCodeModal {
public:
    void open(const String& title);
    bool handleInput(KeyboardManager& keyboard);
    void draw(M5Canvas& canvas) const;

    bool isActive() const { return _active; }
    bool takeSubmittedCode(String& code);

private:
    bool _active = false;
    bool _submitted = false;
    String _title;
    String _code;
};

#endif // SECURITY_CODE_MODAL_H
