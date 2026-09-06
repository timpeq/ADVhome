#include <M5Cardputer.h>
#include <WiFi.h>

M5Canvas canvas(&M5Cardputer.Display);

enum AppState {
    SCANNING,
    SELECT_SSID,
    INPUT_PASSWORD,
    CONNECTING,
    CONNECTED
};

AppState currentState = SCANNING;
String ssid = "";
String password = "";
String currentInput = "";

int numNetworks = 0;
int selectedNetwork = 0;
int scrollOffset = 0;

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    
    M5Cardputer.Display.setRotation(1);
    
    // Create a memory sprite the exact size of the Cardputer screen (240x135)
    canvas.createSprite(M5Cardputer.Display.width(), M5Cardputer.Display.height());
    
    Serial.println("ADVhome Starting...");
    
    // Disconnect any previous connections before scanning
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
}

void loop() {
    M5Cardputer.update();
    
    bool redraw = false;
    
    // --- Keyboard Input Handling ---
    static Keyboard_Class::KeysState last_status;
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
    
    if (currentState == SELECT_SSID || currentState == INPUT_PASSWORD) {
        bool enter_pressed = (status.enter && !last_status.enter) || 
                             (status.space && !last_status.space) || 
                             M5Cardputer.BtnA.wasPressed();
        bool del_pressed = (status.del && !last_status.del);
        
        // Find newly pressed character keys
        std::vector<char> new_chars;
        for (char c : status.word) {
            bool was_pressed = false;
            for (char lc : last_status.word) {
                if (c == lc) { was_pressed = true; break; }
            }
            if (!was_pressed) new_chars.push_back(c);
        }
        
        if (currentState == SELECT_SSID) {
            for (char i : new_chars) {
                if (i == 's' || i == '/' || i == 'j' || i == '.' || i == 'S' || i == 'J') {
                    selectedNetwork++;
                    if (selectedNetwork >= numNetworks) selectedNetwork = numNetworks - 1;
                    if (selectedNetwork >= scrollOffset + 7) scrollOffset++;
                    redraw = true;
                }
                if (i == 'w' || i == ';' || i == 'k' || i == ',' || i == 'W' || i == 'K') {
                    selectedNetwork--;
                    if (selectedNetwork < 0) selectedNetwork = 0;
                    if (selectedNetwork < scrollOffset) scrollOffset--;
                    redraw = true;
                }
            }
            if (enter_pressed) {
                ssid = WiFi.SSID(selectedNetwork);
                currentState = INPUT_PASSWORD;
                currentInput = "";
                redraw = true;
            }
        }
        else if (currentState == INPUT_PASSWORD) {
            for (char i : new_chars) {
                currentInput += i;
                redraw = true;
            }
            if (del_pressed && currentInput.length() > 0) {
                currentInput.remove(currentInput.length() - 1);
                redraw = true;
            }
            if (enter_pressed && currentInput.length() > 0) {
                password = currentInput;
                currentInput = "";
                currentState = CONNECTING;
                WiFi.begin(ssid.c_str(), password.c_str());
                redraw = true;
            }
        }
    }
    last_status = status;
    
    // Always trigger redraw for blinker in password state
    if (currentState == INPUT_PASSWORD) {
        static uint32_t lastBlink = 0;
        if (millis() - lastBlink > 500) {
            redraw = true;
            lastBlink = millis();
        }
    }
    
    // Initial states should draw at least once
    static AppState lastState = (AppState)-1;
    if (currentState != lastState) {
        redraw = true;
        lastState = currentState;
    }

    // --- UI Rendering ---
    if (redraw) {
        if (currentState == SCANNING) {
            canvas.fillSprite(BLACK);
            canvas.setCursor(0, 0);
            canvas.setTextColor(YELLOW);
            canvas.setTextSize(2);
            canvas.println("Scanning WiFi...");
            canvas.pushSprite(0, 0);
            
            numNetworks = WiFi.scanNetworks();
            if (numNetworks > 0) {
                currentState = SELECT_SSID;
            } else {
                canvas.fillSprite(BLACK);
                canvas.setCursor(0, 0);
                canvas.setTextColor(RED);
                canvas.println("No networks found!");
                canvas.pushSprite(0, 0);
                delay(2000);
            }
        }
        else if (currentState == SELECT_SSID) {
            canvas.fillSprite(BLACK);
            canvas.setTextColor(GREEN);
            canvas.setTextSize(2);
            canvas.setCursor(0, 0);
            canvas.println("Select WiFi (W/S):");
            
            canvas.setTextSize(1);
            int maxItems = 7; 
            for (int i = 0; i < maxItems && i + scrollOffset < numNetworks; i++) {
                int idx = i + scrollOffset;
                if (idx == selectedNetwork) {
                    canvas.fillRect(0, 30 + i * 15, canvas.width(), 15, WHITE);
                    canvas.setTextColor(BLACK); 
                } else {
                    canvas.setTextColor(WHITE);
                }
                canvas.setCursor(5, 32 + i * 15);
                canvas.printf("%s (%d dBm)", WiFi.SSID(idx).c_str(), WiFi.RSSI(idx));
            }
            canvas.pushSprite(0, 0);
        }
        else if (currentState == INPUT_PASSWORD) {
            canvas.fillSprite(BLACK);
            canvas.setCursor(0, 0);
            canvas.setTextColor(GREEN);
            canvas.setTextSize(2);
            canvas.println("Enter Password:");
            
            canvas.setTextColor(WHITE);
            canvas.println(ssid);
            canvas.println("");
            
            canvas.setTextColor(YELLOW);
            canvas.print("> ");
            for (int i = 0; i < currentInput.length(); i++) {
                canvas.print("*");
            }
            
            if ((millis() / 500) % 2 == 0) {
                canvas.print("_");
            }
            canvas.pushSprite(0, 0);
        }
        else if (currentState == CONNECTING) {
            canvas.fillSprite(BLACK);
            canvas.setCursor(0, 0);
            canvas.setTextColor(YELLOW);
            canvas.setTextSize(2);
            canvas.println("Connecting to:");
            canvas.setTextColor(WHITE);
            canvas.println(ssid);
            canvas.print("\nWaiting...");
            canvas.pushSprite(0, 0);
        }
        else if (currentState == CONNECTED) {
            canvas.fillSprite(BLACK);
            canvas.setCursor(0, 0);
            canvas.setTextColor(GREEN);
            canvas.setTextSize(2);
            canvas.println("WiFi Connected!");
            canvas.setTextColor(WHITE);
            canvas.println("");
            canvas.print("IP: ");
            canvas.println(WiFi.localIP().toString().c_str());
            canvas.pushSprite(0, 0);
        }
    }
    
    // Handle background connection polling (needs to trigger redraws when status changes)
    if (currentState == CONNECTING) {
        static uint32_t lastDot = 0;
        if (millis() - lastDot > 500) {
            lastDot = millis();
            // Just force a redraw to animate dots
            canvas.fillSprite(BLACK);
            canvas.setCursor(0, 0);
            canvas.setTextColor(YELLOW);
            canvas.setTextSize(2);
            canvas.println("Connecting to:");
            canvas.setTextColor(WHITE);
            canvas.println(ssid);
            
            static int dots = 0;
            dots = (dots + 1) % 4;
            canvas.print("\nWaiting");
            for (int i = 0; i < dots; i++) canvas.print(".");
            canvas.pushSprite(0, 0);
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            currentState = CONNECTED;
        } else if (WiFi.status() == WL_CONNECT_FAILED) {
            canvas.fillSprite(BLACK);
            canvas.setCursor(0, 0);
            canvas.setTextColor(RED);
            canvas.println("Connection Failed!");
            canvas.pushSprite(0, 0);
            delay(2000);
            currentState = SCANNING;
        }
    }
    else if (currentState == CONNECTED) {
        if (WiFi.status() != WL_CONNECTED) {
            currentState = CONNECTING;
            WiFi.disconnect();
            WiFi.reconnect();
        }
    }
    
    // Very small delay to yield CPU to background tasks (like WiFi)
    delay(5); 
}
