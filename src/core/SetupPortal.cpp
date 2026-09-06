#include "SetupPortal.h"

SetupPortal::SetupPortal(ConfigManager& config) : _config(config), _server(80) {}

void SetupPortal::begin() {
    _setupComplete = false;
    
    _server.on("/", HTTP_GET, std::bind(&SetupPortal::handleRoot, this));
    _server.on("/save", HTTP_POST, std::bind(&SetupPortal::handleSave, this));
    
    _server.begin();
}

void SetupPortal::update() {
    _server.handleClient();
}

void SetupPortal::stop() {
    _server.stop();
}

void SetupPortal::handleRoot() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ADVhome Setup</title>
    <style>
        body { font-family: -apple-system, system-ui, sans-serif; background: #121212; color: #fff; margin: 0; padding: 20px; }
        .container { max-width: 400px; margin: 0 auto; background: #1e1e1e; padding: 20px; border-radius: 12px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); }
        h2 { color: #4CAF50; text-align: center; margin-top: 0; }
        label { display: block; margin: 15px 0 5px; color: #bbb; }
        input[type="text"], input[type="password"] { width: 100%; padding: 12px; margin-bottom: 10px; border: 1px solid #333; border-radius: 6px; background: #2d2d2d; color: #fff; box-sizing: border-box; font-size: 16px; }
        input[type="submit"] { width: 100%; padding: 14px; background: #4CAF50; color: white; border: none; border-radius: 6px; font-size: 16px; font-weight: bold; cursor: pointer; margin-top: 20px; }
        input[type="submit"]:hover { background: #45a049; }
        .hint { font-size: 12px; color: #888; margin-top: -5px; margin-bottom: 10px; display: block; }
    </style>
</head>
<body>
    <div class="container">
        <h2>ADVhome Setup</h2>
        <form action="/save" method="POST">
            <label for="url">Home Assistant URL</label>
            <input type="text" id="url" name="url" placeholder="http://homeassistant.local:8123" required>
            
            <label for="token">Long-Lived Access Token</label>
            <span class="hint">Create this in your Home Assistant profile page at the bottom.</span>
            <input type="password" id="token" name="token" placeholder="eyJhbG..." required>
            
            <input type="submit" value="Connect">
        </form>
    </div>
</body>
</html>
)rawliteral";
    
    _server.send(200, "text/html", html);
}

void SetupPortal::handleSave() {
    if (_server.hasArg("url") && _server.hasArg("token")) {
        String url = _server.arg("url");
        String token = _server.arg("token");
        
        // Basic cleanup of URL
        if (url.endsWith("/")) {
            url = url.substring(0, url.length() - 1);
        }
        
        _config.saveHAConfig(url, token);
        _setupComplete = true;
        
        _server.send(200, "text/html", "<html><body style='background:#121212;color:#4CAF50;text-align:center;font-family:sans-serif;padding-top:50px;'><h2>Setup Complete!</h2><p>You can close this page. Cardputer is connecting...</p></body></html>");
    } else {
        _server.send(400, "text/plain", "Missing URL or Token.");
    }
}
