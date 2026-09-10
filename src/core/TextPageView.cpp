#include "TextPageView.h"

namespace {
constexpr int VISIBLE_LINES = 10;
constexpr int LINE_HEIGHT = 11;
}

TextPageView::TextPageView(ConfigManager& config, const TextPageLine* lines, int count)
    : _lines(lines), _count(count), _scrollRepeater(config) {}

void TextPageView::onEnter() {
    _scrollOffset = 0;
}

void TextPageView::draw(DisplayManager& display) {
    auto canvas = display.getCanvas();
    canvas->setTextSize(1);

    int y = 22;
    for (int i = _scrollOffset; i < _count && (i - _scrollOffset) < VISIBLE_LINES; i++) {
        switch (_lines[i].style) {
            case 0:  canvas->setTextColor(TFT_CYAN); break;
            case 2:  canvas->setTextColor(TFT_DARKGREY); break;
            default: canvas->setTextColor(TFT_LIGHTGREY); break;
        }
        canvas->setCursor(5, y + ((i - _scrollOffset) * LINE_HEIGHT));
        canvas->print(_lines[i].text);
    }

    if (_count > VISIBLE_LINES) {
        int trackTop = 20;
        int trackHeight = 112;
        int thumbHeight = (trackHeight * VISIBLE_LINES) / _count;
        if (thumbHeight < 8) thumbHeight = 8;
        int travel = trackHeight - thumbHeight;
        int thumbY = trackTop + (travel * _scrollOffset) / (_count - VISIBLE_LINES);
        canvas->fillRect(236, trackTop, 3, trackHeight, 0x2965);
        canvas->fillRect(236, thumbY, 3, thumbHeight, TFT_DARKCYAN);
    }
}

bool TextPageView::handleInput(KeyboardManager& keyboard) {
    int direction = _scrollRepeater.update(keyboard);
    int maxOffset = _count - VISIBLE_LINES;
    if (maxOffset < 0) maxOffset = 0;

    if (direction < 0 && _scrollOffset > 0) {
        _scrollOffset--;
        return true;
    }
    if (direction > 0 && _scrollOffset < maxOffset) {
        _scrollOffset++;
        return true;
    }
    return false;
}
