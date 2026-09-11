#include "DisplayManager.h"
#include "Battery.h"

DisplayManager::DisplayManager() : _canvas(&M5Cardputer.Display) {
}

void DisplayManager::begin() {
    _canvas.createSprite(M5Cardputer.Display.width(), M5Cardputer.Display.height());
}

void DisplayManager::push() {
    _canvas.pushSprite(0, 0);
}

void DisplayManager::clear(uint16_t color) {
    _canvas.fillSprite(color);
}

void DisplayManager::drawMessage(const String& title, const String& message, uint16_t titleColor) {
    clear();
    _canvas.setTextColor(titleColor);
    _canvas.setTextSize(2);
    _canvas.setCursor(10, 40);
    _canvas.print(title);
    
    _canvas.setTextColor(TFT_WHITE);
    _canvas.setTextSize(1);
    _canvas.setCursor(10, 80);
    _canvas.print(message);
    
    drawBatteryIndicator();
    push();
}

void DisplayManager::drawModalMessage(const String& title, const String& line1, const String& line2, uint16_t color1, uint16_t color2, const String& hint) {
    _canvas.fillRect(20, 30, 200, 75, TFT_BLACK); 
    _canvas.drawRect(19, 29, 202, 77, TFT_DARKGREY); 
    _canvas.drawRect(20, 30, 200, 75, TFT_LIGHTGREY); 
    _canvas.fillRect(21, 31, 198, 14, TFT_DARKCYAN); 
    
    _canvas.setTextColor(TFT_WHITE);
    _canvas.setTextSize(1);
    _canvas.setCursor(24, 34);
    _canvas.print(title);
    
    _canvas.setTextColor(color1);
    _canvas.setTextSize(1);
    _canvas.setCursor(24, 55);
    _canvas.print(line1);
    
    if (!line2.isEmpty()) {
        _canvas.setTextColor(color2);
        _canvas.setCursor(24, 75);
        _canvas.print(line2);
    }
    
    // Recovery keys live outside the modal frame: they are always available on a
    // connecting screen, but they should not read as part of the status message.
    if (!hint.isEmpty()) {
        _canvas.setTextColor(TFT_DARKGREY);
        _canvas.setTextSize(1);
        _canvas.setCursor(24, 114);
        _canvas.print(hint);
    }
    
    drawBatteryIndicator();
    push();
}

void DisplayManager::drawMenu(const String& title, const std::vector<String>& items, int selectedIndex, int scrollOffset) {
    clear();
    _canvas.setTextColor(TFT_GREEN);
    _canvas.setTextSize(2);
    _canvas.setCursor(0, 0);
    _canvas.println(title);
    
    _canvas.setTextSize(1);
    int maxItems = 7; 
    for (int i = 0; i < maxItems && i + scrollOffset < items.size(); i++) {
        int idx = i + scrollOffset;
        if (idx == selectedIndex) {
            _canvas.fillRect(0, 30 + i * 15, _canvas.width(), 15, TFT_WHITE);
            _canvas.setTextColor(TFT_BLACK); 
        } else {
            _canvas.setTextColor(TFT_WHITE);
        }
        _canvas.setCursor(5, 32 + i * 15);
        _canvas.println(items[idx]);
    }
    drawBatteryIndicator();
    push();
}

void DisplayManager::drawPasswordInput(const String& title, const String& subtitle, const String& currentInput) {
    clear();
    _canvas.setCursor(0, 0);
    _canvas.setTextColor(TFT_GREEN);
    _canvas.setTextSize(2);
    _canvas.println(title);
    
    _canvas.setTextColor(TFT_WHITE);
    _canvas.setTextSize(1);
    _canvas.println(subtitle);
    _canvas.println("");
    
    _canvas.setTextColor(TFT_YELLOW);
    _canvas.print("> ");
    for (int i = 0; i < currentInput.length(); i++) {
        _canvas.print("*");
    }
    
    if ((millis() / 500) % 2 == 0) {
        _canvas.print("_");
    }
    drawBatteryIndicator();
    push();
}

void DisplayManager::drawHASetup(const String& ipAddress) {
    clear();
    _canvas.setCursor(0, 0);
    _canvas.setTextColor(TFT_GREEN);
    _canvas.setTextSize(2);
    _canvas.println("HA Setup");
    
    _canvas.setTextColor(TFT_WHITE);
    _canvas.setTextSize(1);
    _canvas.println("Scan QR code");
    _canvas.println("or go to:");
    _canvas.setTextColor(TFT_YELLOW);
    _canvas.print("http://");
    _canvas.println(ipAddress);
    
    String url = "http://" + ipAddress;
    // Draw QR on the right side of the screen
    _canvas.qrcode(url.c_str(), 140, 20, 90, 2);
    drawBatteryIndicator();
    push();
}

void DisplayManager::drawBatteryIndicator() {
    int batteryLevel = Battery::level();
    _canvas.fillRect(200, 0, 40, 16, TFT_BLACK);
    _canvas.setTextColor(TFT_GREEN);
    _canvas.setTextSize(1);
    _canvas.setCursor(205, 4);
    _canvas.print(String(batteryLevel) + "%");
}
