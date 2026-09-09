#include "EntityDetailView.h"
#include "TextScroller.h"
#include "Graphics.h"
#include "Format.h"

EntityDetailView::EntityDetailView(EntityManager& entityManager, ConfigManager& config, std::function<void()> onBack, std::function<void(String, String)> onCallService, std::function<void(String, float)> onSetVolume, std::function<void(String, float)> onSeekMedia, std::function<void(String, String, String, String)> onSecureService, std::function<void(String, float)> onSetClimateTemp, std::function<void(String, float, float)> onSetClimateRange, std::function<void(String, String)> onSetHvacMode, bool showEntityName)
    : _entityManager(entityManager), _onBack(onBack), _onCallService(onCallService), _onSetVolume(onSetVolume), _onSeekMedia(onSeekMedia), _onSecureService(onSecureService), _onSetClimateTemp(onSetClimateTemp), _onSetClimateRange(onSetClimateRange), _onSetHvacMode(onSetHvacMode), _config(config), _scrollRepeater(config), _seekRepeater(config), _climateRepeater(config), _showEntityName(showEntityName) {}

void EntityDetailView::setEntityId(const String& id) {
    _entityId = id;
    // Do not reuse the Enter press that selected this entity as play/pause.
    _playPauseKeyHeld = true;
    _climateChangedLocally = false;
    _volumeChangedLocally = false;
    _seekChangedLocally = false;
}

void EntityDetailView::draw(DisplayManager& display) {
    auto canvas = display.getCanvas();
    Entity entity = _entityManager.getEntity(_entityId);
    
    // Frame the detail widget like a small window; back remains a keyboard action.
    canvas->fillRect(4, 20, 232, 114, TFT_BLACK); // Darken background to hide list below
    canvas->drawRect(3, 19, 234, 116, TFT_DARKGREY); // Outer drop shadow effect
    canvas->drawRect(4, 20, 232, 114, TFT_LIGHTGREY); // Inner window border
    canvas->fillRect(5, 21, 230, 14, TFT_DARKCYAN); // Distinct title bar color
    canvas->setTextColor(TFT_WHITE);
    canvas->setTextSize(1);
    canvas->setCursor(8, 24);
    if (_showEntityName) {
        canvas->print(TextScroller::visible(entity.friendlyName, 30));
    } else {
        String domainText = entity.domain;
        domainText.replace("_", " ");
        canvas->print(domainText);

    }
    
    if (entity.domain == "climate") {
        drawClimate(canvas, entity);
        _securityModal.draw(*canvas);
        return;
    }

    // Content
    canvas->setCursor(10, 40);
    canvas->setTextColor(TFT_WHITE);
    
    String dispState = entity.state;
    dispState.replace("_", " ");
    
    if (entity.domain == "scene") {
        canvas->print("Last run: ");
        canvas->setTextColor(TFT_YELLOW);
        canvas->println(TextScroller::visible(dispState, 28));
    } else {
        if (entity.state == "on" || entity.state == "unlocked" || entity.state == "open" || entity.state == "playing") {
            canvas->setTextColor(TFT_GREEN);
        } else if (entity.state == "off" || entity.state == "locked" || entity.state == "closed" || entity.state == "paused" || entity.state == "idle") {
            canvas->setTextColor(TFT_RED);
        } else {
            canvas->setTextColor(TFT_YELLOW);
        }
        
        if (entity.domain == "media_player") {
            canvas->setCursor(30, 40); // Tastefully indented for media player
            canvas->println(dispState);
        } else {
            canvas->print("State: ");
            canvas->println(dispState);
        }
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
        
        MediaPlayerState ms;
        _entityManager.getMediaPlayerState(_entityId, ms);

        bool showMediaTitle = !ms.title.isEmpty() &&
            (!_showEntityName || !ms.title.equalsIgnoreCase(entity.friendlyName));
        if (showMediaTitle) {
            canvas->setCursor(10, infoY);
            canvas->setTextColor(TFT_WHITE);
            canvas->setTextSize(2);
            String title = TextScroller::visible(ms.title, 18);
            canvas->print(title);
            infoY += 18;
        }
        
        if (!ms.artist.isEmpty()) {
            canvas->setCursor(10, infoY);
            canvas->setTextColor(TFT_CYAN);
            canvas->setTextSize(1);
            String artist = ms.artist;
            if (!ms.album.isEmpty()) {
                artist += " - " + ms.album;
            }
            artist = TextScroller::visible(artist, 35);
            canvas->print(artist);
            infoY += 13;
        }
        
        // Progress bar
        if (ms.duration > 0) {
            float currentPosition = ms.position;
            if (_seekChangedLocally) {
                currentPosition = _seekTarget;
            } else if (entity.state == "playing" && ms.positionUpdatedAt > 0) {
                currentPosition += (millis() - ms.positionUpdatedAt) / 1000.0f;
            }
            if (currentPosition > ms.duration) currentPosition = ms.duration;

            float progress = currentPosition / ms.duration;
            if (progress > 1.0f) progress = 1.0f;
            
            // Time labels
            canvas->setCursor(10, infoY);
            canvas->setTextSize(1);
            canvas->setTextColor(TFT_LIGHTGREY);
            canvas->print(Format::clock((int)currentPosition) + " / " + Format::clock((int)ms.duration));

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
        
        float displayVolume = _volumeChangedLocally ? _volumeTarget : ms.volumeLevel;
        int volPct = (int)(displayVolume * 100);
        
        canvas->print("Vol: ");
        if (_volumeChangedLocally) {
            canvas->setTextColor(TFT_CYAN); // Highlight when adjusting locally
        }
        canvas->print(String(volPct) + "%");
        if (ms.isVolumeMuted) {
            canvas->setTextColor(TFT_RED);
            canvas->print(" [MUTED]");
        }
        
        // Controls help at bottom
        canvas->setCursor(10, 123);
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

void EntityDetailView::drawClimate(M5Canvas* canvas, const Entity& entity) {
    ClimateState c;
    if (!_entityManager.getClimateState(entity.id, c)) return;

    String mode = entity.state;
    mode.replace("_", " ");
    uint16_t modeColor = TFT_LIGHTGREY;
    if (entity.state == "heat") modeColor = TFT_ORANGE;
    else if (entity.state == "cool") modeColor = TFT_CYAN;
    else if (entity.state == "heat_cool" || entity.state == "auto") modeColor = TFT_GREEN;
    else if (entity.state == "dry") modeColor = TFT_YELLOW;

    canvas->setTextSize(1);
    canvas->setCursor(10, 40);
    canvas->setTextColor(TFT_LIGHTGREY);
    canvas->print("Mode: ");
    canvas->setTextColor(modeColor);
    canvas->print(mode);

    if (!c.hvacAction.isEmpty() && c.hvacAction != "off") {
        String act = c.hvacAction;
        act.replace("_", " ");
        canvas->setCursor(150, 40);
        canvas->setTextColor(c.hvacAction == "heating" ? TFT_RED
                             : c.hvacAction == "cooling" ? TFT_CYAN
                             : TFT_LIGHTGREY);
        canvas->print(act);
    }

    // Current ambient temperature, large.
    canvas->setTextColor(TFT_WHITE);
    canvas->setTextSize(3);
    canvas->setCursor(14, 58);
    canvas->print(Format::oneDecimal(c.currentTemperature));
    canvas->setTextSize(1);
    canvas->print(" now");

    // Target setpoint(s).
    bool editing = _climateChangedLocally;
    canvas->setCursor(14, 92);
    canvas->setTextColor(editing ? TFT_CYAN : TFT_YELLOW);
    canvas->setTextSize(2);
    if (c.hasRange()) {
        float lo = editing ? _climateLow : c.targetTempLow;
        float hi = editing ? _climateHigh : c.targetTempHigh;
        canvas->print(Format::oneDecimal(lo) + "-" + Format::oneDecimal(hi));
    } else {
        float t = editing ? _climateTarget : c.targetTemperature;
        canvas->print(Format::oneDecimal(t));
    }
    canvas->setTextSize(1);
    canvas->print(" set");

    if (!isnan(c.currentHumidity)) {
        canvas->setCursor(160, 99);
        canvas->setTextColor(TFT_LIGHTGREY);
        canvas->print(Format::rounded(c.currentHumidity) + "% RH");
    }

    canvas->setCursor(10, 123);
    canvas->setTextColor(0x6B6D);
    canvas->print(c.hvacModes.isEmpty() ? "+/- Temp" : "+/- Temp   ENTER Mode");
}

bool EntityDetailView::handleClimateInput(KeyboardManager& keyboard, const Entity& entity) {
    ClimateState c;
    if (!_entityManager.getClimateState(entity.id, c)) return false;
    
    float step = c.targetTempStep > 0.0f ? c.targetTempStep : 0.5f;
    bool handled = false;

    // _playPauseKeyHeld is set true by setEntityId() to swallow the Enter press
    // that opened this view; release it once that key is up.
    if (!keyboard.isEnterHeld()) _playPauseKeyHeld = false;

    int dir = 0;
    for (char ch : keyboard.getNewChars()) {
        if (ch == '+' || ch == '=') dir += 1;
        else if (ch == '-' || ch == '_') dir -= 1;
    }
    int rep = _climateRepeater.update(keyboard); // -1 up, 1 down
    if (rep != 0) dir += (rep < 0 ? 1 : -1);

    if (dir != 0) {
        if (!_climateChangedLocally) {
            float base = !isnan(c.targetTemperature) ? c.targetTemperature
                        : !isnan(c.currentTemperature) ? c.currentTemperature : 20.0f;
            _climateTarget = base;
            _climateLow = !isnan(c.targetTempLow) ? c.targetTempLow : base;
            _climateHigh = !isnan(c.targetTempHigh) ? c.targetTempHigh : base;
        }
        float d = dir * step;
        _climateTarget = constrain(_climateTarget + d, c.minTemp, c.maxTemp);
        _climateLow = constrain(_climateLow + d, c.minTemp, c.maxTemp);
        _climateHigh = constrain(_climateHigh + d, c.minTemp, c.maxTemp);
        _climateChangedLocally = true;
        _lastClimateChangeTime = millis();
        handled = true;
    }

    if (_climateChangedLocally) {
        uint32_t now = millis();
        bool keysIdle = !keyboard.isUpHeld() && !keyboard.isDownHeld() &&
                        !keyboard.isPlusHeld() && !keyboard.isMinusHeld();
        if (now - _lastClimateChangeTime > 700 ||
            (keysIdle && dir == 0 && now - _lastClimateChangeTime > 250)) {
            if (c.hasRange()) {
                if (_onSetClimateRange) _onSetClimateRange(entity.id, _climateLow, _climateHigh);
            } else if (_onSetClimateTemp) {
                if (_onSetClimateTemp) _onSetClimateTemp(entity.id, _climateTarget);
            }
            _climateChangedLocally = false;
        }
        handled = true; // keep redrawing while an adjustment is live
    }

    if (keyboard.wasEnterPressed() && !_playPauseKeyHeld && !c.hvacModes.isEmpty()) {
        String modes = c.hvacModes;
        int start = 0, comma, curIdx = -1, count = 0;
        String next;
        String first;
        while (true) {
            comma = modes.indexOf(',', start);
            String m = (comma == -1) ? modes.substring(start) : modes.substring(start, comma);
            if (count == 0) first = m;
            if (curIdx >= 0 && next.isEmpty()) next = m;      // mode right after current
            if (m == entity.state) curIdx = count;
            count++;
            if (comma == -1) break;
            start = comma + 1;
        }
        if (next.isEmpty()) next = first; // wrapped past the end (or current not found)
        if (!next.isEmpty() && _onSetHvacMode) _onSetHvacMode(entity.id, next);
        handled = true;
    }

    return handled;
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

    if (entity.domain == "climate") {
        if (keyboard.wasBackspacePressed() || keyboard.wasTabPressed()) {
            if (_onBack) _onBack();
            return true;
        }
        return handleClimateInput(keyboard, entity);
    }

    bool actionBack = false;
    bool actionPlayPause = false;
    bool actionStop = false;
    bool actionMute = false;
    float volumeDelta = 0.0f;
    bool actionEnter = false;
    int tapSeekDir = 0;
    int scrubSeekDir = 0;

    // 1. Evaluate explicit keys
    if (keyboard.wasBackspacePressed() || keyboard.wasTabPressed()) {
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
            MediaPlayerState ms;
            if (!_seekChangedLocally) {
                if (_entityManager.getMediaPlayerState(_entityId, ms)) {
                    _seekTarget = ms.position;
                } else {
                    _seekTarget = 0.0f;
                }
                if (entity.state == "playing" && _entityManager.getMediaPlayerState(_entityId, ms) && ms.positionUpdatedAt > 0) {
                    _seekTarget += (now - ms.positionUpdatedAt) / 1000.0f;
                }
            }
            
            float holdDurationMs = now - _seekHoldStartTime;
            float rampTimeMs = 3000.0f; // 3 seconds to reach max speed
            float progress = holdDurationMs / rampTimeMs;
            if (progress > 1.0f) progress = 1.0f;
            
            float minStep = _config.getSeekStep();
            float maxStep = _config.getSeekStepMax();
            float currentStep = minStep + (maxStep - minStep) * progress;
            
            _seekTarget += (scrubSeekDir * currentStep);
            if (_seekTarget < 0.0f) _seekTarget = 0.0f;
            if (_entityManager.getMediaPlayerState(_entityId, ms) && _seekTarget > ms.duration) _seekTarget = ms.duration;
            
            _seekChangedLocally = true;
            _lastSeekChangeTime = now;
            handled = true;
        }

        // Debounce seek sending
        if (_seekChangedLocally && (now - _lastSeekChangeTime > 500 || (!keyboard.isLeftHeld() && !keyboard.isRightHeld() && scrubSeekDir == 0))) {
            if (_onSeekMedia) {
                _onSeekMedia(entity.id, _seekTarget);
            }
            _seekChangedLocally = false;
        }

        // Volume logic
        if (volumeDelta != 0.0f) {
            if (!_volumeChangedLocally) {
                MediaPlayerState msVol;
                if (_entityManager.getMediaPlayerState(_entityId, msVol)) {
                    _volumeTarget = msVol.volumeLevel;
                } else {
                    _volumeTarget = 0.0f;
                }
            }
            _volumeTarget = constrain(_volumeTarget + volumeDelta, 0.0f, 1.0f);
            _volumeChangedLocally = true;
            _lastVolumeChangeTime = now;
            handled = true;
        }

        // Debounce volume sending (500ms after last change)
        if (_volumeChangedLocally && (now - _lastVolumeChangeTime > 500 || (!keyboard.isUpHeld() && !keyboard.isDownHeld() && volumeDelta == 0.0f))) {
            if (_onSetVolume) {
                _onSetVolume(entity.id, _volumeTarget);
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
