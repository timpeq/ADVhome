#include <Arduino.h>
#include <M5Cardputer.h>
#include "core/AppController.h"

AppController app;

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    
    // Setting rotation is required for Cardputer Display
    M5Cardputer.Display.setRotation(1);
    
    Serial.println("ADVhome Starting...");
    app.begin();
}

void loop() {
    M5Cardputer.update();
    app.update();
    
    // Very small delay to yield CPU to background tasks (like WiFi)
    delay(5); 
}
