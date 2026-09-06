#include "TextScroller.h"

String TextScroller::visible(const String& text, int maxCharacters, bool scroll) {
    if (maxCharacters <= 0 || text.length() <= maxCharacters) return text;
    if (!scroll || maxCharacters <= 3) {
        return text.substring(0, maxCharacters > 3 ? maxCharacters - 3 : maxCharacters) +
               (maxCharacters > 3 ? "..." : "");
    }

    int overflow = text.length() - maxCharacters;
    int cycle = (millis() / 250) % ((overflow + 4) * 2);
    int offset = 0;

    if (cycle < 4) offset = 0;
    else if (cycle < overflow + 4) offset = cycle - 4;
    else if (cycle < overflow + 8) offset = overflow;
    else offset = overflow - (cycle - (overflow + 8));

    return text.substring(offset, offset + maxCharacters);
}