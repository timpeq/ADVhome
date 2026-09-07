#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <M5Cardputer.h>

namespace Graphics {
void drawHomeIcon(M5Canvas& canvas, int centerX, int centerY, uint16_t color);
void drawLightIcon(M5Canvas& canvas, int centerX, int centerY, bool isOn, uint16_t color);
void drawToggle(M5Canvas& canvas, int centerX, int centerY, bool isOn, uint16_t color);
void drawAlarmIcon(M5Canvas& canvas, int centerX, int centerY, bool isArmed, uint16_t color);
void drawMusicIcon(M5Canvas& canvas, int centerX, int centerY, uint16_t color);
void drawPlaybackIcon(M5Canvas& canvas, int centerX, int centerY, const String& state, uint16_t color);
}

#endif // GRAPHICS_H
