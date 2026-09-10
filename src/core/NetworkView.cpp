#include "NetworkView.h"
#include "TextScroller.h"

namespace {
// An armed confirmation disarms itself so the device is never left one keypress
// away from wiping its credentials while it sits on a desk.
constexpr uint32_t ARM_TIMEOUT_MS = 8000;
}

NetworkView::NetworkView(const char* title,
                         const char* actionLabel,
                         InfoProvider info,
                         std::function<void()> action)
    : _title(title), _actionLabel(actionLabel), _info(info), _action(action) {}

void NetworkView::onEnter() {
    _armed = false;
    _lines.clear();
    if (_info) _info(_lines);
}

void NetworkView::onExit() {
    _armed = false;
}

void NetworkView::draw(DisplayManager& display) {
    auto canvas = display.getCanvas();
    canvas->setTextSize(1);

    canvas->setTextColor(TFT_CYAN);
    canvas->setCursor(5, 22);
    canvas->print(_title);

    canvas->setTextColor(TFT_LIGHTGREY);
    int y = 40;
    for (size_t i = 0; i < _lines.size() && i < 4; i++) {
        canvas->setCursor(5, y + (i * 12));
        canvas->print(TextScroller::visible(_lines[i], 38));
    }

    // Action row, drawn as a button so it reads as distinct from the info above.
    int buttonY = 100;
    canvas->drawRect(4, buttonY, 232, 18, _armed ? TFT_RED : TFT_DARKGREY);
    canvas->setTextColor(_armed ? TFT_RED : TFT_WHITE);
    canvas->setCursor(10, buttonY + 5);
    canvas->print(_armed ? "ENTER again to confirm" : _actionLabel);

    canvas->setTextColor(0x6B6D);
    canvas->setCursor(10, 123);
    canvas->print(_armed ? "ESC cancels. Device reboots." : "ENTER to change");
}

bool NetworkView::handleInput(KeyboardManager& keyboard) {
    if (_armed && millis() - _armedAt > ARM_TIMEOUT_MS) {
        _armed = false;
        return true;
    }

    if (_armed && keyboard.wasEscPressed()) {
        _armed = false;
        return true;
    }

    if (keyboard.wasEnterPressed()) {
        if (_armed) {
            _armed = false;
            if (_action) _action();
        } else {
            _armed = true;
            _armedAt = millis();
        }
        return true;
    }

    return false;
}
