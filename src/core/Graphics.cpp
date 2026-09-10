#include "Graphics.h"

namespace Graphics {

void drawHomeIcon(M5Canvas& canvas, int centerX, int centerY, uint16_t color) {
    canvas.fillTriangle(centerX - 9, centerY - 2, centerX, centerY - 10, centerX + 9, centerY - 2, color);
    canvas.fillRect(centerX - 7, centerY - 2, 14, 11, color);
    canvas.fillRect(centerX - 2, centerY + 2, 4, 7, TFT_BLACK);
}

void drawMenuIcon(M5Canvas& canvas, int centerX, int centerY, uint16_t color) {
    for (int i = -1; i <= 1; i++) {
        canvas.fillRect(centerX - 7, centerY + (i * 4) - 1, 14, 2, color);
    }
}

void drawLightIcon(M5Canvas& canvas, int centerX, int centerY, bool isOn, uint16_t color) {
    // A bulb, not a balloon: glass on top, a short neck, then a screw base.
    // The whole glyph stays within centerY-10..centerY+8 so it cannot reach up
    // into the detail window's title bar.
    uint16_t glass = isOn ? color : TFT_DARKGREY;

    canvas.fillCircle(centerX, centerY - 3, 5, glass);
    canvas.fillRect(centerX - 3, centerY + 1, 6, 2, glass);

    canvas.fillRect(centerX - 3, centerY + 3, 6, 5, TFT_LIGHTGREY);
    canvas.drawFastHLine(centerX - 3, centerY + 5, 6, TFT_DARKGREY);

    if (isOn) {
        canvas.drawFastVLine(centerX, centerY - 10, 2, color);
        canvas.drawFastHLine(centerX - 10, centerY - 3, 2, color);
        canvas.drawFastHLine(centerX + 9, centerY - 3, 2, color);
        canvas.drawLine(centerX - 8, centerY - 8, centerX - 7, centerY - 7, color);
        canvas.drawLine(centerX + 8, centerY - 8, centerX + 7, centerY - 7, color);
    }
}

void drawToggle(M5Canvas& canvas, int centerX, int centerY, bool isOn, uint16_t color) {
    uint16_t trackColor = isOn ? color : TFT_DARKGREY;
    canvas.fillRoundRect(centerX - 12, centerY - 6, 24, 12, 6, trackColor);
    canvas.fillCircle(centerX + (isOn ? 6 : -6), centerY, 4, TFT_WHITE);
}

void drawAlarmIcon(M5Canvas& canvas, int centerX, int centerY, bool isArmed, uint16_t color) {
    uint16_t iconColor = isArmed ? color : TFT_DARKGREY;
    canvas.fillTriangle(centerX, centerY - 9, centerX - 8, centerY - 3, centerX - 6, centerY + 8, iconColor);
    canvas.fillTriangle(centerX, centerY - 9, centerX + 8, centerY - 3, centerX + 6, centerY + 8, iconColor);
    canvas.fillRect(centerX - 4, centerY - 1, 8, 8, iconColor);
    canvas.drawLine(centerX - 10, centerY - 8, centerX - 7, centerY - 11, iconColor);
    canvas.drawLine(centerX + 10, centerY - 8, centerX + 7, centerY - 11, iconColor);
    canvas.setTextColor(TFT_BLACK);
    canvas.setTextSize(1);
    canvas.setCursor(centerX - 2, centerY - 3);
    canvas.print(isArmed ? "!" : "-");
}

void drawMusicIcon(M5Canvas& canvas, int centerX, int centerY, uint16_t color) {
    canvas.fillCircle(centerX - 5, centerY + 7, 4, color);
    canvas.fillCircle(centerX + 6, centerY + 3, 4, color);
    canvas.fillRect(centerX - 2, centerY - 10, 3, 17, color);
    canvas.fillRect(centerX + 9, centerY - 14, 3, 17, color);
    canvas.fillRect(centerX, centerY - 14, 12, 4, color);
}

void drawPlaybackIcon(M5Canvas& canvas, int centerX, int centerY, const String& state, uint16_t color) {
    if (state == "playing") {
        canvas.fillTriangle(centerX - 5, centerY - 7, centerX + 6, centerY, centerX - 5, centerY + 7, color);
    } else if (state == "paused") {
        canvas.fillRect(centerX - 6, centerY - 7, 4, 14, color);
        canvas.fillRect(centerX + 2, centerY - 7, 4, 14, color);
    } else if (state == "idle" || state == "off" || state == "stopped") {
        canvas.fillRect(centerX - 6, centerY - 6, 12, 12, color);
    } else {
        canvas.drawCircle(centerX, centerY, 7, color);
    }
}

}
