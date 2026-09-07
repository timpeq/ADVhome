#include "DiagnosticView.h"

DiagnosticView::DiagnosticView(WifiConnectionManager& wifi, HomeAssistantManager& ha) : _wifi(wifi), _ha(ha) {}

void DiagnosticView::draw(DisplayManager& display) {
    auto canvas = display.getCanvas();
    
    canvas->setCursor(5, 30);
    canvas->setTextColor(TFT_CYAN);
    canvas->setTextSize(2);
    canvas->println("Diagnostic Info");
    
    canvas->setTextColor(TFT_WHITE);
    canvas->setTextSize(1);
    canvas->setCursor(5, 55);
    canvas->println("");
    
    canvas->setCursor(5, canvas->getCursorY());
    canvas->print("IP: ");
    canvas->setTextColor(TFT_YELLOW);
    canvas->println(_wifi.getIPAddress());
    
    canvas->setTextColor(TFT_WHITE);
    canvas->setCursor(5, canvas->getCursorY());
    canvas->print("HA: ");
    canvas->setTextColor(TFT_YELLOW);
    // Might be long, so let it wrap
    canvas->println(_ha.getVersion()); // Or we can fetch HA URL if we passed config, but version is enough
    
    canvas->setTextColor(TFT_WHITE);
    canvas->setCursor(5, canvas->getCursorY());
    canvas->print("WS Connected: ");
    if (_ha.isConnected()) {
        canvas->setTextColor(TFT_GREEN);
        canvas->println("Yes");
    } else {
        canvas->setTextColor(TFT_RED);
        canvas->println("No");
    }

    canvas->setTextColor(TFT_WHITE);
    canvas->setCursor(5, canvas->getCursorY());
    canvas->print("WiFi RSSI: ");
    canvas->setTextColor(TFT_YELLOW);
    canvas->print(WiFi.RSSI());
    canvas->println(" dBm");

    canvas->setTextColor(TFT_WHITE);
    canvas->setCursor(5, canvas->getCursorY());
    canvas->print("Free Heap: ");
    canvas->setTextColor(TFT_YELLOW);
    canvas->print(ESP.getFreeHeap() / 1024);
    canvas->println(" KB");
    
    canvas->setTextColor(TFT_WHITE);
    canvas->setCursor(5, canvas->getCursorY());
    canvas->print("Build: ");
    canvas->setTextColor(TFT_YELLOW);
    canvas->println(__DATE__);
}

bool DiagnosticView::handleInput(KeyboardManager& keyboard) {
    // Diagnostic view doesn't handle inputs
    return false;
}
