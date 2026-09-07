#include "EntityDetailView.h"
#include "TextScroller.h"
#include "Graphics.h"

EntityDetailView::EntityDetailView(EntityManager& entityManager, ConfigManager& config, std::function<void()> onBack, std::function<void(String, String)> onCallService, std::function<void(String, float)> onSetVolume, std::function<void(String, String, String, String)> onSecureService, bool showEntityName)
    : _entityManager(entityManager), _onBack(onBack), _onCallService(onCallService), _onSetVolume(onSetVolume), _onSecureService(onSecureService), _config(config), _scrollRepeater(config), _showEntityName(showEntityName) {}

void EntityDetailView::setEntityId(const String& id) {
    _entityId = id;
}

void EntityDetailView::draw(DisplayManager& display) {
    auto canvas = display.getCanvas();
    Entity entity = _entityManager.getEntity(_entityId);
    
    // Frame the detail widget like a small window; back remains a keyboard action.
    canvas->fillRect(2, 18, 236, 116, 0x18E3);
    canvas->drawRect(2, 18, 236, 116, TFT_DARKGREY);
    canvas->fillRect(4, 20, 232, 14, TFT_BLUE);
    canvas->setTextColor(TFT_WHITE);
    canvas->setTextSize(1);
    canvas->setCursor(8, 24);
    if (_showEntityName) {
        canvas->print(TextScroller::visible(entity.friendlyName, 30));
    } else {
        canvas->print(entity.domain);
    }
    canvas->drawRect(2, 18, 236, 116, TFT_DARKGREY);
    
    // Content
    canvas->setCursor(4, 40);
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

    if (entity.domain == "light") {
        Graphics::drawLightIcon(*canvas, 218, 42, entity.state == "on", entity.state == "on" ? TFT_YELLOW : TFT_LIGHTGREY);
    } else if (entity.domain == "switch") {
        Graphics::drawToggle(*canvas, 218, 42, entity.state == "on", entity.state == "on" ? TFT_GREEN : TFT_LIGHTGREY);
    } else if (entity.domain == "alarm_control_panel") {
        Graphics::drawAlarmIcon(*canvas, 218, 42, entity.state != "disarmed", entity.state != "disarmed" ? TFT_RED : TFT_LIGHTGREY);
    } else if (entity.domain == "media_player") {
        Graphics::drawPlaybackIcon(*canvas, 218, 42, entity.state, entity.state == "playing" ? TFT_GREEN : TFT_LIGHTGREY);
    }
    
    // Instructions
    // Instructions / media info
    canvas->setTextSize(1);
    
    if (entity.domain == "media_player") {
        // Keep the footer clear while giving the track title more visual weight.
        int infoY = 54;

        bool showMediaTitle = !entity.mediaTitle.isEmpty() &&
            (!_showEntityName || !entity.mediaTitle.equalsIgnoreCase(entity.friendlyName));
        if (showMediaTitle) {
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
            artist = TextScroller::visible(artist, 35);
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

            Graphics::drawMusicIcon(*canvas, 215, infoY + 7, TFT_CYAN);
            
            infoY += 10;
            // Bar background
            canvas->fillRect(10, infoY, 180, 5, 0x2124);
            // Bar fill
            int fillWidth = (int)(180.0f * progress);
            canvas->fillRect(10, infoY, fillWidth, 5, TFT_CYAN);
            infoY += 8;
        } else {
            Graphics::drawMusicIcon(*canvas, 215, infoY + 7, TFT_CYAN);
        }
        
        // Volume
        infoY = 104;
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
        canvas->setCursor(5, 123);
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
        } else if (entity.domain == "alarm_control_panel") {
            canvas->println("ENTER: Arm/disarm");
        } else {
            canvas->println("No actions available");
        }
    }

    _securityModal.draw(*canvas);
}

bool EntityDetailView::handleInput(KeyboardManager& keyboard) {
    if (_securityModal.isActive()) {
        bool handled = _securityModal.handleInput(keyboard);
        String code;
        if (_securityModal.takeSubmittedCode(code)) {
            Entity entity = _entityManager.getEntity(_entityId);
            if (_onSecureService) {
                _onSecureService(entity.domain, _pendingSecureService, entity.id, code);
            }
            _pendingSecureService = "";
            handled = true;
        }
        return handled;
    }

    Entity entity = _entityManager.getEntity(_entityId);

    if (entity.domain == "media_player") {
        if (keyboard.wasLeftPressed()) {
            if (_onCallService) _onCallService(entity.domain, "media_previous_track");
            return true;
        }
        if (keyboard.wasRightPressed()) {
            if (_onCallService) _onCallService(entity.domain, "media_next_track");
            return true;
        }
        if (keyboard.wasEnterPressed()) {
            if (_onCallService) _onCallService(entity.domain, "media_play_pause");
            return true;
        }
    }

    if (keyboard.wasBackspacePressed()) {
        if (_onBack) _onBack();
        return true;
    }

    auto changeVolume = [&](float delta) {
        if (_onSetVolume) {
            float nextVolume = constrain(entity.volumeLevel + delta, 0.0f, 1.0f);
            _onSetVolume(entity.id, nextVolume);
        }
    };
    
    // Check for character-based inputs (media player controls)
    auto chars = keyboard.getNewChars();
    for (char c : chars) {
        if (entity.domain == "media_player") {
            if (c == '+' || c == '=') {
                changeVolume(0.01f);
                return true;
            } else if (c == '-' || c == '_') {
                changeVolume(-0.01f);
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
        int volumeDirection = _scrollRepeater.update(keyboard);
        if (volumeDirection != 0) {
            changeVolume(volumeDirection < 0 ? 0.01f : -0.01f);
            return true;
        }
    }

    if (keyboard.wasEnterPressed()) {
        if (entity.domain == "light" || entity.domain == "switch" || entity.domain == "fan" || entity.domain == "input_boolean") {
            if (_onCallService) _onCallService(entity.domain, "toggle");
        } else if (entity.domain == "cover") {
            _pendingSecureService = entity.state == "open" ? "close_cover" : "open_cover";
            _securityModal.open("Security code");
        } else if (entity.domain == "lock") {
            _pendingSecureService = entity.state == "locked" ? "unlock" : "lock";
            _securityModal.open("Security code");
        } else if (entity.domain == "alarm_control_panel") {
            _pendingSecureService = entity.state == "disarmed" ? "alarm_arm_home" : "alarm_disarm";
            _securityModal.open("Security code");
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
