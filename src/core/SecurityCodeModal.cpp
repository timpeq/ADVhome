#include "SecurityCodeModal.h"

void SecurityCodeModal::open(const String& title) {
    _active = true;
    _submitted = false;
    _title = title;
    _code = "";
}

bool SecurityCodeModal::handleInput(KeyboardManager& keyboard) {
    if (!_active) return false;

    bool handled = false;
    if (keyboard.wasBackspacePressed()) {
        if (_code.length() > 0) _code.remove(_code.length() - 1);
        else _active = false;
        handled = true;
    }

    for (char character : keyboard.getNewChars()) {
        if (character >= '0' && character <= '9') {
            _code += character;
            handled = true;
        }
    }

    if (keyboard.wasEnterPressed() && _code.length() > 0) {
        _submitted = true;
        _active = false;
        handled = true;
    }

    return handled;
}

void SecurityCodeModal::draw(M5Canvas& canvas) const {
    if (!_active) return;

    canvas.fillRect(24, 44, 192, 58, TFT_BLACK);
    canvas.drawRect(24, 44, 192, 58, TFT_WHITE);
    canvas.setTextColor(TFT_CYAN);
    canvas.setTextSize(1);
    canvas.setCursor(34, 52);
    canvas.print(_title);

    canvas.setTextColor(TFT_YELLOW);
    canvas.setCursor(34, 72);
    canvas.print("> ");
    for (unsigned int i = 0; i < _code.length(); i++) canvas.print("*");
    if ((millis() / 500) % 2 == 0) canvas.print("_");

    canvas.setTextColor(TFT_LIGHTGREY);
    canvas.setCursor(34, 92);
    canvas.print("Enter: submit  Esc: cancel");
}

bool SecurityCodeModal::takeSubmittedCode(String& code) {
    if (!_submitted) return false;
    code = _code;
    _submitted = false;
    _code = "";
    return true;
}
