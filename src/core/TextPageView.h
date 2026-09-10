#ifndef TEXT_PAGE_VIEW_H
#define TEXT_PAGE_VIEW_H

#include "View.h"
#include "ConfigManager.h"
#include "ScrollRepeater.h"

// A scrolling page of static text. Style is carried per line so a page reads as
// sections without needing a second table: 0 = heading, 1 = body, 2 = footnote.
struct TextPageLine {
    const char* text;
    uint8_t style;
};

class TextPageView : public View {
public:
    TextPageView(ConfigManager& config, const TextPageLine* lines, int count);

    void onEnter() override;
    void draw(DisplayManager& display) override;
    bool handleInput(KeyboardManager& keyboard) override;

private:
    const TextPageLine* _lines;
    int _count;
    int _scrollOffset = 0;
    ScrollRepeater _scrollRepeater;
};

#endif // TEXT_PAGE_VIEW_H
