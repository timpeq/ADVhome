#include "AboutView.h"

#ifndef ADVHOME_VERSION
#define ADVHOME_VERSION "dev"
#endif

namespace {

// Colour is carried per line so the page reads as sections without needing a
// second table. 0 = heading, 1 = body, 2 = dim/footnote.
struct AboutLine {
    const char* text;
    uint8_t style;
};

const AboutLine LINES[] = {
    {"ADVhome " ADVHOME_VERSION, 0},
    {"Home Assistant client for the", 1},
    {"M5Stack Cardputer.", 1},
    {"", 1},
    {"(c) 2026 Tim Pequignot", 1},
    {"MIT License", 1},
    {"", 1},
    {"Built on the work of:", 0},
    {" M5Unified / M5GFX      MIT", 1},
    {" M5Cardputer           MIT", 1},
    {" LovyanGFX (via M5GFX) FreeBSD", 1},
    {" ArduinoJson           MIT", 1},
    {" arduinoWebSockets  LGPL-2.1", 1},
    {" ESP32 Arduino core LGPL-2.1", 1},
    {" ESP-IDF          Apache-2.0", 1},
    {" Home Assistant   Apache-2.0", 1},
    {"", 1},
    {"The LGPL-2.1 parts are linked", 2},
    {"statically. Source for this", 2},
    {"exact build is published at", 2},
    {"the revision shown above.", 2},
    {"", 1},
    {"Full terms: LICENSE and", 2},
    {"THIRD-PARTY-NOTICES.md", 2},
    {"", 1},
    {"Not affiliated with M5Stack,", 2},
    {"Nabu Casa, or the Home", 2},
    {"Assistant project.", 2},
};

constexpr int LINE_COUNT = sizeof(LINES) / sizeof(LINES[0]);
constexpr int VISIBLE_LINES = 10;
constexpr int LINE_HEIGHT = 11;

}  // namespace

AboutView::AboutView(ConfigManager& config) : _scrollRepeater(config) {}

void AboutView::onEnter() {
    _scrollOffset = 0;
}

void AboutView::draw(DisplayManager& display) {
    auto canvas = display.getCanvas();
    canvas->setTextSize(1);

    int y = 22;
    for (int i = _scrollOffset; i < LINE_COUNT && (i - _scrollOffset) < VISIBLE_LINES; i++) {
        switch (LINES[i].style) {
            case 0:  canvas->setTextColor(TFT_CYAN); break;
            case 2:  canvas->setTextColor(TFT_DARKGREY); break;
            default: canvas->setTextColor(TFT_LIGHTGREY); break;
        }
        canvas->setCursor(5, y + ((i - _scrollOffset) * LINE_HEIGHT));
        canvas->print(LINES[i].text);
    }

    // Scrollbar, matching the entity list treatment.
    if (LINE_COUNT > VISIBLE_LINES) {
        int trackTop = 20;
        int trackHeight = 112;
        int thumbHeight = (trackHeight * VISIBLE_LINES) / LINE_COUNT;
        if (thumbHeight < 8) thumbHeight = 8;
        int travel = trackHeight - thumbHeight;
        int thumbY = trackTop + (travel * _scrollOffset) / (LINE_COUNT - VISIBLE_LINES);
        canvas->fillRect(236, trackTop, 3, trackHeight, 0x2965);
        canvas->fillRect(236, thumbY, 3, thumbHeight, TFT_DARKCYAN);
    }
}

bool AboutView::handleInput(KeyboardManager& keyboard) {
    int direction = _scrollRepeater.update(keyboard);
    int maxOffset = LINE_COUNT - VISIBLE_LINES;
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
