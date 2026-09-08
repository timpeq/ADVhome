#include <Arduino.h>
#include <M5Cardputer.h>
#include "core/AppController.h"

AppController app;

void setup() {
    // When launched from the M5Launcher OTA slot there is no USB bus reset, so
    // the host stays bound to the launcher's now-dead CDC endpoint. Re-init the
    // CDC and give the host a moment to re-enumerate before anything logs.
    Serial.begin(115200);
    delay(1500);

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
