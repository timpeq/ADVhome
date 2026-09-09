#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <M5Cardputer.h>
#include <vector>
#include <Arduino.h>

class DisplayManager {
public:
    DisplayManager();
    void begin();
    
    // Core drawing methods
    void push();
    void clear(uint16_t color = TFT_BLACK);
    
    // Screens
    void drawMessage(const String& title, const String& message, uint16_t titleColor = TFT_YELLOW);
    void drawModalMessage(const String& title, const String& line1, const String& line2 = "", uint16_t color1 = TFT_YELLOW, uint16_t color2 = TFT_CYAN, const String& hint = "");
    void drawMenu(const String& title, const std::vector<String>& items, int selectedIndex, int scrollOffset);
    void drawPasswordInput(const String& title, const String& subtitle, const String& currentInput);
    void drawHASetup(const String& ipAddress);
    void drawBatteryIndicator();
    
    // Direct canvas access for custom drawing if needed
    M5Canvas* getCanvas() { return &_canvas; }
    
private:
    M5Canvas _canvas;
};

#endif // DISPLAY_MANAGER_H
