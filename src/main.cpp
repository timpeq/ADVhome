#include <Arduino.h>
#include <M5Cardputer.h>
#include <esp_sleep.h>
#include "core/AppController.h"

AppController app;

void setup() {
    // When launched from the M5Launcher OTA slot there is no USB bus reset, so
    // the host stays bound to the launcher's now-dead CDC endpoint. Re-init the
    // CDC and give the host a moment to re-enumerate before anything logs. A
    // wake from deep sleep is a clean reset with no hand-off, so it skips the wait.
    Serial.begin(115200);
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED) {
        delay(1500);
    }

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
