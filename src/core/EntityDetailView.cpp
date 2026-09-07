#include "EntityDetailView.h"
#include "TextScroller.h"

EntityDetailView::EntityDetailView(EntityManager& entityManager, ConfigManager& config, std::function<void()> onBack, std::function<void(String, String)> onCallService)
    : _entityManager(entityManager), _onBack(onBack), _onCallService(onCallService), _config(config) {}

void EntityDetailView::setEntityId(const String& id) {
    _entityId = id;
}

void EntityDetailView::draw(DisplayManager& display) {
    auto canvas = display.getCanvas();
    Entity entity = _entityManager.getEntity(_entityId);
    
    // Top bar
    canvas->fillRect(0, 0, 240, 20, TFT_BLUE);
    canvas->setTextColor(TFT_WHITE);
    canvas->setTextSize(1);
    canvas->setCursor(5, 5);
    canvas->print("< Bksp ");
    canvas->print(entity.domain);
    display.drawBatteryIndicator();
    
    // Content
    canvas->setCursor(4, 24);
    canvas->setTextColor(TFT_CYAN);
    canvas->setTextSize(1);
    
    String dispName = TextScroller::visible(entity.friendlyName, 38);
    canvas->println(dispName);
    
    canvas->setCursor(4, 38);
    canvas->setTextColor(TFT_WHITE);
    if (entity.domain == "scene") {
        canvas->print("Last run: ");
        canvas->setTextColor(TFT_YELLOW);
        canvas->println(TextScroller::visible(entity.state, 28));
    } else {
        canvas->print("State: ");
    
        if (entity.state == "on" || entity.state == "unlocked" || entity.state == "open" || entity.state == "playing") {
            canvas->setTextColor(TFT_GREEN);
        } else if (entity.state == "off" || entity.state == "locked" || entity.state == "closed" || entity.state == "paused" || entity.state == "idle") {
            canvas->setTextColor(TFT_RED);
        } else {
            canvas->setTextColor(TFT_YELLOW);
        }
    
        canvas->println(entity.state);
    }
    
    // Instructions
    // Instructions / media info
    canvas->setTextSize(1);
    
    if (entity.domain == "media_player") {
        // Keep the footer clear while giving the track title more visual weight.
        int infoY = 54;
        
        if (!entity.mediaTitle.isEmpty()) {
            canvas->setCursor(10, infoY);
            canvas->setTextColor(TFT_WHITE);
            canvas->setTextSize(2);
            String title = TextScroller::visible(entity.mediaTitle, 18);
            canvas->print(title);
            infoY += 18;
        }
        
        if (!entity.mediaArtist.isEmpty()) {
            canvas->setCursor(10, infoY);
            canvas->setTextColor(TFT_CYAN);
            canvas->setTextSize(1);
            String artist = entity.mediaArtist;
            if (!entity.mediaAlbum.isEmpty()) {
                artist += " - " + entity.mediaAlbum;
            }
            artist = TextScroller::visible(artist, 35, false);
            canvas->print(artist);
            infoY += 13;
        }
        
        // Progress bar
        if (entity.mediaDuration > 0) {
            float progress = entity.mediaPosition / entity.mediaDuration;
            if (progress > 1.0f) progress = 1.0f;
            
            // Time labels
            int posMin = (int)entity.mediaPosition / 60;
            int posSec = (int)entity.mediaPosition % 60;
            int durMin = (int)entity.mediaDuration / 60;
            int durSec = (int)entity.mediaDuration % 60;
            
            canvas->setCursor(10, infoY);
            canvas->setTextSize(1);
            canvas->setTextColor(TFT_LIGHTGREY);
            char timeBuf[20];
            snprintf(timeBuf, sizeof(timeBuf), "%d:%02d / %d:%02d", posMin, posSec, durMin, durSec);
            canvas->print(timeBuf);
            
            infoY += 10;
            // Bar background
            canvas->fillRect(10, infoY, 180, 5, 0x2124);
            // Bar fill
            int fillWidth = (int)(180.0f * progress);
            canvas->fillRect(10, infoY, fillWidth, 5, TFT_CYAN);
            infoY += 8;
        }
        
        // Volume
        infoY = 108;
        canvas->setCursor(10, infoY);
        canvas->setTextSize(1);
        canvas->setTextColor(TFT_LIGHTGREY);
        int volPct = (int)(entity.volumeLevel * 100);
        canvas->print("Vol: ");
        canvas->print(String(volPct) + "%");
        if (entity.isVolumeMuted) {
            canvas->setTextColor(TFT_RED);
            canvas->print(" [MUTED]");
        }
        
        // Controls help at bottom
        canvas->setCursor(5, 128);
        canvas->setTextColor(0x6B6D);
        canvas->print("ENT:Play +/-:Vol </>:Skip M:Mute");
    } else {
        canvas->setCursor(10, 95);
        canvas->setTextColor(TFT_LIGHTGREY);
        if (entity.domain == "light" || entity.domain == "switch" || entity.domain == "fan" || entity.domain == "input_boolean") {
            canvas->println("ENTER: Toggle on/off");
        } else if (entity.domain == "cover") {
            canvas->println("ENTER: Toggle open/close");
        } else if (entity.domain == "lock") {
            if (entity.state == "locked") {
                canvas->println("ENTER: Unlock");
            } else {
                canvas->println("ENTER: Lock");
            }
        } else if (entity.domain == "script" || entity.domain == "button") {
            canvas->println("ENTER: Execute");
        } else if (entity.domain == "scene") {
            canvas->println("ENTER: Activate");
        } else if (entity.domain == "automation") {
            canvas->println("ENTER: Trigger");
        } else {
            canvas->println("No actions available");
        }
    }
}

bool EntityDetailView::handleInput(KeyboardManager& keyboard) {
    bool canBack = false;
    int backStyle = _config.getBackButtonStyle();
    
    if (backStyle == 0) {
        canBack = keyboard.wasBackspacePressed() || keyboard.wasLeftPressed();
    } else if (backStyle == 1) {
        canBack = keyboard.wasLeftPressed();
    } else if (backStyle == 2) {
        canBack = keyboard.wasBackspacePressed();
    }
    
    if (canBack) {
        if (_onBack) _onBack();
        return true;
    }
    
    Entity entity = _entityManager.getEntity(_entityId);
    
    // Check for character-based inputs (media player controls)
    auto chars = keyboard.getNewChars();
    for (char c : chars) {
        if (entity.domain == "media_player") {
            if (c == '+' || c == '=') {
                if (_onCallService) _onCallService(entity.domain, "volume_up");
                return true;
            } else if (c == '-' || c == '_') {
                if (_onCallService) _onCallService(entity.domain, "volume_down");
                return true;
            } else if (c == 'm' || c == 'M') {
                if (_onCallService) _onCallService(entity.domain, "volume_mute");
                return true;
            } else if (c == 's' || c == 'S') {
                if (_onCallService) _onCallService(entity.domain, "media_stop");
                return true;
            }
        }
    }
    
    if (entity.domain == "media_player") {
        if (keyboard.wasUpPressed()) {
            if (_onCallService) _onCallService(entity.domain, "volume_up");
            return true;
        }
        if (keyboard.wasDownPressed()) {
            if (_onCallService) _onCallService(entity.domain, "volume_down");
            return true;
        }
    }

    // Media player prev/next with arrow keys (only when back style doesn't use left)
    if (entity.domain == "media_player") {
        if (keyboard.wasLeftPressed()) {
            if (_onCallService) _onCallService(entity.domain, "media_previous_track");
            return true;
        }
        if (keyboard.wasRightPressed()) {
            if (_onCallService) _onCallService(entity.domain, "media_next_track");
            return true;
        }
    }
    
    if (keyboard.wasEnterPressed()) {
        if (entity.domain == "light" || entity.domain == "switch" || entity.domain == "fan" || entity.domain == "input_boolean") {
            if (_onCallService) _onCallService(entity.domain, "toggle");
        } else if (entity.domain == "cover") {
            if (entity.state == "open") {
                if (_onCallService) _onCallService(entity.domain, "close_cover");
            } else {
                if (_onCallService) _onCallService(entity.domain, "open_cover");
            }
        } else if (entity.domain == "lock") {
            if (entity.state == "locked") {
                if (_onCallService) _onCallService(entity.domain, "unlock");
            } else {
                if (_onCallService) _onCallService(entity.domain, "lock");
            }
        } else if (entity.domain == "media_player") {
            if (_onCallService) _onCallService(entity.domain, "media_play_pause");
        } else if (entity.domain == "script") {
            if (_onCallService) _onCallService(entity.domain, "turn_on");
        } else if (entity.domain == "button") {
            if (_onCallService) _onCallService(entity.domain, "press");
        } else if (entity.domain == "scene") {
            if (_onCallService) _onCallService(entity.domain, "turn_on");
        } else if (entity.domain == "automation") {
            if (_onCallService) _onCallService(entity.domain, "trigger");
        }
        return true;
    }
    
    return false;
}
