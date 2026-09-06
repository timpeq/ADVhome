#include "DisplayManager.h"

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
    _canvas.setCursor(0, 0);
    _canvas.setTextColor(titleColor);
    _canvas.setTextSize(2);
    _canvas.println(title);
    
    _canvas.setTextColor(TFT_WHITE);
    _canvas.setTextSize(1);
    _canvas.println(message);
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
    push();
}

void DisplayManager::drawDiagPage(const String& ipAddress, const String& haUrl, const String& haVersion) {
    clear();
    _canvas.setCursor(0, 0);
    _canvas.setTextColor(TFT_CYAN);
    _canvas.setTextSize(2);
    _canvas.println("Diagnostic Info");
    
    _canvas.setTextColor(TFT_WHITE);
    _canvas.setTextSize(1);
    _canvas.println("");
    
    _canvas.print("IP: ");
    _canvas.setTextColor(TFT_YELLOW);
    _canvas.println(ipAddress);
    
    _canvas.setTextColor(TFT_WHITE);
    _canvas.print("HA: ");
    _canvas.setTextColor(TFT_YELLOW);
    _canvas.println(haUrl);
    
    _canvas.setTextColor(TFT_WHITE);
    _canvas.print("Version: ");
    _canvas.setTextColor(TFT_GREEN);
    _canvas.println(haVersion);
    
    push();
}
