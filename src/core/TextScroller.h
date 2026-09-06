#ifndef TEXT_SCROLLER_H
#define TEXT_SCROLLER_H

#include <Arduino.h>

class TextScroller {
public:
    static String visible(const String& text, int maxCharacters, bool scroll = true);
};

#endif // TEXT_SCROLLER_H