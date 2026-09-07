#include "EntityDetailView.h"
#include "TextScroller.h"
#include "Graphics.h"

EntityDetailView::EntityDetailView(EntityManager& entityManager, ConfigManager& config, std::function<void()> onBack, std::function<void(String, String)> onCallService, std::function<void(String, float)> onSetVolume, std::function<void(String, float)> onSeekMedia, std::function<void(String, String, String, String)> onSecureService, bool showEntityName)
    : _entityManager(entityManager), _onBack(onBack), _onCallService(onCallService), _onSetVolume(onSetVolume), _onSeekMedia(onSeekMedia), _onSecureService(onSecureService), _config(config), _scrollRepeater(config), _seekRepeater(config), _showEntityName(showEntityName) {}

void EntityDetailView::setEntityId(const String& id) {
    _entityId = id;
    // Do not reuse the Enter press that selected this entity as play/pause.
    _playPauseKeyHeld = true;
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
            float currentPosition = entity.mediaPosition;
            if (_seekChangedLocally) {
                currentPosition = _targetSeekPosition;
            } else if (entity.state == "playing" && entity.mediaPositionUpdatedAt > 0) {
                currentPosition += (millis() - entity.mediaPositionUpdatedAt) / 1000.0f;
            }
            if (currentPosition > entity.mediaDuration) currentPosition = entity.mediaDuration;

            float progress = currentPosition / entity.mediaDuration;
            if (progress > 1.0f) progress = 1.0f;
            
            // Time labels
            int posMin = (int)currentPosition / 60;
            int posSec = (int)currentPosition % 60;
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
        
        float displayVolume = _volumeChangedLocally ? _targetVolume : entity.volumeLevel;
        int volPct = (int)(displayVolume * 100);
        
        canvas->print("Vol: ");
        if (_volumeChangedLocally) {
            canvas->setTextColor(TFT_CYAN); // Highlight when adjusting locally
        }
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
    bool handled = false;

    bool actionBack = false;
    bool actionPlayPause = false;
    bool actionStop = false;
    bool actionMute = false;
    float volumeDelta = 0.0f;
    bool actionEnter = false;
    int tapSeekDir = 0;
    int scrubSeekDir = 0;

    // 1. Evaluate explicit keys
    if (keyboard.wasBackspacePressed()) {
        actionBack = true;
    }

    if (keyboard.wasEnterPressed()) {
        actionEnter = true;
    }

    // 2. Evaluate Media Player controls
    if (entity.domain == "media_player") {
        bool playPauseHeld = keyboard.isEnterHeld() || keyboard.isCharHeld('p') || keyboard.isCharHeld('P');
        if (playPauseHeld && !_playPauseKeyHeld) {
            actionPlayPause = true;
        }
        _playPauseKeyHeld = playPauseHeld;

        auto chars = keyboard.getNewChars();
        for (char c : chars) {
            if (c == '+' || c == '=') volumeDelta += 0.01f;
            else if (c == '-' || c == '_') volumeDelta -= 0.01f;
            else if (c == 'm' || c == 'M') actionMute = true;
            else if (c == 's' || c == 'S') actionStop = true;
        }

        int volDir = _scrollRepeater.update(keyboard);
        if (volDir != 0) {
            volumeDelta += (volDir < 0 ? 0.01f : -0.01f);
        }

        uint32_t current_now = millis();
        // Custom Seek Logic
        if (keyboard.wasLeftPressed() || keyboard.wasRightPressed()) {
            _seekChangedLocally = false;
            _wasSeekHeld = false;
            _seekHoldStartTime = current_now;
        }

        int sDir = _seekRepeater.updateLeftRight(keyboard);
        if (sDir != 0) {
            if (!keyboard.wasLeftPressed() && !keyboard.wasRightPressed()) {
                _wasSeekHeld = true;
                scrubSeekDir = sDir;
            }
        }

        if (keyboard.wasLeftReleased() && !_wasSeekHeld) tapSeekDir = -1;
        if (keyboard.wasRightReleased() && !_wasSeekHeld) tapSeekDir = 1;
    }

    // 3. Execute Actions
    if (actionBack) {
        if (_onBack) _onBack();
        handled = true;
    }
    
    if (entity.domain == "media_player") {
        if (actionPlayPause && _onCallService) {
            _onCallService(entity.domain, "media_play_pause");
            handled = true;
        }
        if (actionStop && _onCallService) {
            _onCallService(entity.domain, "media_stop");
            handled = true;
        }
        if (actionMute && _onCallService) {
            _onCallService(entity.domain, "volume_mute");
            handled = true;
        }
        if (tapSeekDir < 0 && _onCallService) {
            _onCallService(entity.domain, "media_previous_track");
            handled = true;
        }
        if (tapSeekDir > 0 && _onCallService) {
            _onCallService(entity.domain, "media_next_track");
            handled = true;
        }

        uint32_t now = millis();

        // Scrub logic
        if (scrubSeekDir != 0) {
            if (!_seekChangedLocally) {
                _targetSeekPosition = entity.mediaPosition;
                if (entity.state == "playing" && entity.mediaPositionUpdatedAt > 0) {
                    _targetSeekPosition += (now - entity.mediaPositionUpdatedAt) / 1000.0f;
                }
            }
            
            float holdDurationMs = now - _seekHoldStartTime;
            float rampTimeMs = 3000.0f; // 3 seconds to reach max speed
            float progress = holdDurationMs / rampTimeMs;
            if (progress > 1.0f) progress = 1.0f;
            
            float minStep = _config.getSeekStep();
            float maxStep = _config.getSeekStepMax();
            float currentStep = minStep + (maxStep - minStep) * progress;
            
            _targetSeekPosition += (scrubSeekDir * currentStep);
            if (_targetSeekPosition < 0.0f) _targetSeekPosition = 0.0f;
            if (_targetSeekPosition > entity.mediaDuration) _targetSeekPosition = entity.mediaDuration;
            
            _seekChangedLocally = true;
            _lastSeekChangeTime = now;
            handled = true;
        }

        // Debounce seek sending
        if (_seekChangedLocally && (now - _lastSeekChangeTime > 500 || (!keyboard.isLeftHeld() && !keyboard.isRightHeld() && scrubSeekDir == 0))) {
            if (_onSeekMedia) {
                _onSeekMedia(entity.id, _targetSeekPosition);
            }
            _seekChangedLocally = false;
        }

        // Volume logic
        if (volumeDelta != 0.0f) {
            if (!_volumeChangedLocally) {
                _targetVolume = entity.volumeLevel;
            }
            _targetVolume = constrain(_targetVolume + volumeDelta, 0.0f, 1.0f);
            _volumeChangedLocally = true;
            _lastVolumeChangeTime = now;
            handled = true;
        }

        // Debounce volume sending (500ms after last change)
        if (_volumeChangedLocally && (now - _lastVolumeChangeTime > 500 || (!keyboard.isUpHeld() && !keyboard.isDownHeld() && volumeDelta == 0.0f))) {
            if (_onSetVolume) {
                _onSetVolume(entity.id, _targetVolume);
            }
            _volumeChangedLocally = false;
        }
    }

    // Handle generic enter actions
    if (actionEnter && !handled) {
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
        handled = true;
    }
    
    return handled;
}
