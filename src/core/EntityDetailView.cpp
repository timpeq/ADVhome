#include "EntityDetailView.h"
#include "TextScroller.h"
#include "Graphics.h"
#include "Format.h"

namespace {
// The detail window spans x 4..236 and y 20..134, with a title bar through y 34.
// Everything below is measured off that frame so the domain icon, the state
// text and the hint line land in the same place for every domain.
constexpr int CONTENT_LEFT = 10;
constexpr int STATE_Y = 40;
constexpr int ICON_CX = 216;
constexpr int ICON_CY = 46;
constexpr int HINT_Y = 123;
constexpr uint16_t HINT_COLOR = 0x6B6D;
// Characters that fit left of the icon column at text size 1.
constexpr int STATE_MAX_CHARS = 30;
// While a key is held, the running value goes out at this interval so the
// device follows the gesture instead of waiting for the key to come up.
constexpr uint32_t HELD_SEND_MS = 250;
// Every seek makes the player rebuffer, so a scrub is sent less often.
constexpr uint32_t HELD_SEEK_SEND_MS = 500;
// After the final send, echoes of the earlier in-flight sends can still
// arrive. "The server value moved" is only trusted once they have landed.
constexpr uint32_t ECHO_SETTLE_MS = 600;
constexpr int BRIGHTNESS_STEP = 5;
}

EntityDetailView::EntityDetailView(EntityManager& entityManager, ConfigManager& config, std::function<void()> onBack, std::function<void(String, String)> onCallService, std::function<void(String, float)> onSetVolume, std::function<void(String, float)> onSeekMedia, std::function<void(String, String, String, String)> onSecureService, std::function<void(String, float)> onSetClimateTemp, std::function<void(String, float, float)> onSetClimateRange, std::function<void(String, String)> onSetHvacMode, std::function<void(String, int)> onSetBrightness, bool showEntityName)
    : _entityManager(entityManager), _onBack(onBack), _onCallService(onCallService), _onSetVolume(onSetVolume), _onSeekMedia(onSeekMedia), _onSecureService(onSecureService), _onSetClimateTemp(onSetClimateTemp), _onSetClimateRange(onSetClimateRange), _onSetHvacMode(onSetHvacMode), _onSetBrightness(onSetBrightness), _config(config), _scrollRepeater(config), _seekRepeater(config), _climateRepeater(config), _showEntityName(showEntityName) {}

void EntityDetailView::setEntityId(const String& id) {
    _entityId = id;
    // Do not reuse the Enter press that selected this entity as play/pause.
    _playPauseKeyHeld = true;
    _climateChangedLocally = false;
    _climatePendingSend = false;
    _volumeChangedLocally = false;
    _volumePendingSend = false;
    _seekChangedLocally = false;
    _seekPendingSend = false;
    _brightnessChangedLocally = false;
    _brightnessPendingSend = false;
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
    canvas->setCursor(CONTENT_LEFT, STATE_Y);
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
            canvas->setCursor(CONTENT_LEFT, STATE_Y);
            canvas->print(TextScroller::visible(dispState, STATE_MAX_CHARS));
        } else {
            canvas->print("State: ");
            // "State: " eats seven characters; clip the rest so a long value
            // like "unavailable" cannot run under the domain icon.
            canvas->print(TextScroller::visible(dispState, STATE_MAX_CHARS - 7));
        }
    }

    if (entity.domain == "light") {
        Graphics::drawLightIcon(*canvas, ICON_CX, ICON_CY, entity.state == "on", entity.state == "on" ? TFT_YELLOW : TFT_LIGHTGREY);
    } else if (entity.domain == "switch") {
        Graphics::drawToggle(*canvas, ICON_CX, ICON_CY, entity.state == "on", entity.state == "on" ? TFT_GREEN : TFT_LIGHTGREY);
    } else if (entity.domain == "alarm_control_panel") {
        Graphics::drawAlarmIcon(*canvas, ICON_CX, ICON_CY, entity.state != "disarmed", entity.state != "disarmed" ? TFT_RED : TFT_LIGHTGREY);
    } else if (entity.domain == "media_player") {
        Graphics::drawPlaybackIcon(*canvas, ICON_CX, ICON_CY, entity.state, entity.state == "playing" ? TFT_GREEN : TFT_LIGHTGREY);
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
            String title = TextScroller::visible(ms.title, 16);
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
        } else if (!showMediaTitle && ms.artist.isEmpty()) {
            // Nothing is playing. The corner note would sit on top of the
            // stopped/off state icon, and the middle of the window is empty, so
            // the placeholder moves to the centre and says what it means.
            Graphics::drawMusicIcon(*canvas, 119, 70, 0x4A69);
            canvas->setTextSize(1);
            canvas->setTextColor(HINT_COLOR);
            canvas->setCursor(75, 90);
            canvas->print("Nothing playing");
        } else {
            // Playing, but no duration to draw a bar from (a live stream).
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
        canvas->setCursor(CONTENT_LEFT, HINT_Y);
        canvas->setTextColor(HINT_COLOR);
        canvas->print("ENT:Play +/-:Vol </>:Skip M:Mute");
    } else {
        LightState light;
        bool dimmable = entity.domain == "light" &&
                        _entityManager.getLightState(_entityId, light) && light.dimmable;
        if (dimmable) {
            // Cyan while showing our own target, as the media volume does.
            bool editing = _brightnessChangedLocally;
            int pct = editing ? _brightnessTarget : light.percent();
            canvas->setTextSize(1);
            canvas->setTextColor(TFT_LIGHTGREY);
            canvas->setCursor(CONTENT_LEFT, 58);
            canvas->print("Brightness");
            canvas->setTextSize(2);
            canvas->setTextColor(editing ? TFT_CYAN : TFT_WHITE);
            canvas->setCursor(CONTENT_LEFT, 70);
            canvas->print(String(pct) + "%");
            canvas->setTextSize(1);
            canvas->fillRect(CONTENT_LEFT, 90, 180, 5, 0x2124);
            canvas->fillRect(CONTENT_LEFT, 90, 180 * pct / 100, 5, editing ? TFT_CYAN : TFT_YELLOW);
        }

        // Was floating at y=95 in light grey while media and climate used the
        // window footer; all three now share one line.
        canvas->setCursor(CONTENT_LEFT, HINT_Y);
        canvas->setTextColor(HINT_COLOR);
        if (dimmable) {
            canvas->print("ENTER: Toggle  +/-: Brightness");
        } else if (entity.domain == "light" || entity.domain == "switch" || entity.domain == "fan" || entity.domain == "input_boolean") {
            canvas->print("ENTER: Toggle on/off");
        } else if (entity.domain == "cover") {
            canvas->print("ENTER: Toggle open/close");
        } else if (entity.domain == "lock") {
            if (entity.state == "locked") {
                canvas->print("ENTER: Unlock");
            } else {
                canvas->print("ENTER: Lock");
            }
        } else if (entity.domain == "script" || entity.domain == "button") {
            canvas->print("ENTER: Execute");
        } else if (entity.domain == "scene") {
            canvas->print("ENTER: Activate");
        } else if (entity.domain == "automation") {
            canvas->print("ENTER: Trigger");
        } else if (entity.domain == "alarm_control_panel") {
            canvas->print("ENTER: Arm/disarm");
        } else {
            canvas->print("No actions available");
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
    canvas->setCursor(CONTENT_LEFT, STATE_Y);
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
    canvas->print(Format::temperature(c.currentTemperature));
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
        canvas->print(Format::temperature(lo) + "-" + Format::temperature(hi));
    } else {
        float t = editing ? _climateTarget : c.targetTemperature;
        canvas->print(Format::temperature(t));
    }
    canvas->setTextSize(1);
    canvas->print(" set");

    if (!isnan(c.currentHumidity)) {
        canvas->setCursor(160, 99);
        canvas->setTextColor(TFT_LIGHTGREY);
        canvas->print(Format::rounded(c.currentHumidity) + "% RH");
    }

    canvas->setCursor(CONTENT_LEFT, HINT_Y);
    canvas->setTextColor(HINT_COLOR);
    canvas->print(c.hvacModes.isEmpty() ? "+/- Temp" : "+/- Temp   ENTER Mode");
}

bool EntityDetailView::handleClimateInput(KeyboardManager& keyboard, const Entity& entity) {
    ClimateState c;
    if (!_entityManager.getClimateState(entity.id, c)) return false;
    
    // Temp Step: 0 follows Home Assistant, 1 forces half degrees, 2 whole.
    int stepMode = _config.getTempStep();
    bool wholeDegrees = stepMode == 2;
    float step = stepMode == 1 ? 0.5f
               : stepMode == 2 ? 1.0f
               : (c.targetTempStep > 0.0f ? c.targetTempStep : 0.5f);
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
        // In whole-degree mode snap onto the integer grid in the direction of
        // travel, so a setpoint sitting on 21.5 moves to 22 rather than 22.5.
        auto stepped = [&](float value) {
            if (wholeDegrees) return dir > 0 ? floorf(value) + 1.0f : ceilf(value) - 1.0f;
            return value + dir * step;
        };
        _climateTarget = constrain(stepped(_climateTarget), c.minTemp, c.maxTemp);
        _climateLow = constrain(stepped(_climateLow), c.minTemp, c.maxTemp);
        _climateHigh = constrain(stepped(_climateHigh), c.minTemp, c.maxTemp);
        _climateChangedLocally = true;
        _climatePendingSend = true;
        _lastClimateChangeTime = millis();
        handled = true;
    }

    if (_climatePendingSend) {
        uint32_t now = millis();
        bool keysIdle = !keyboard.isUpHeld() && !keyboard.isDownHeld() &&
                        !keyboard.isPlusHeld() && !keyboard.isMinusHeld();
        if (now - _lastClimateChangeTime > 700 ||
            (keysIdle && dir == 0 && now - _lastClimateChangeTime > 250)) {
            if (c.hasRange()) {
                if (_onSetClimateRange) _onSetClimateRange(entity.id, _climateLow, _climateHigh);
            } else if (_onSetClimateTemp) {
                _onSetClimateTemp(entity.id, _climateTarget);
            }
            _climatePendingSend = false;
            _climateSentAt = now;
        }
        handled = true;
    } else if (_climateChangedLocally) {
        // Hold our own setpoint on screen until Home Assistant echoes a matching
        // one back. Dropping it at send time made the display fall back to the
        // stale server value for the length of the round trip, which read as the
        // number bouncing to the new value, back, then forward again.
        float tolerance = step * 0.5f;
        bool serverAgrees = c.hasRange()
            ? (fabsf(c.targetTempLow - _climateLow) < tolerance &&
               fabsf(c.targetTempHigh - _climateHigh) < tolerance)
            : (!isnan(c.targetTemperature) &&
               fabsf(c.targetTemperature - _climateTarget) < tolerance);
        // Give up eventually so a clamped or rejected setpoint cannot stick.
        if (serverAgrees || millis() - _climateSentAt > 6000) {
            _climateChangedLocally = false;
        }
        handled = true;
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

bool EntityDetailView::handleBrightnessInput(KeyboardManager& keyboard, const Entity& entity) {
    LightState light;
    if (!_entityManager.getLightState(entity.id, light) || !light.dimmable) return false;

    int dir = 0;
    for (char ch : keyboard.getNewChars()) {
        if (ch == '+' || ch == '=') dir += 1;
        else if (ch == '-' || ch == '_') dir -= 1;
    }
    int rep = _scrollRepeater.update(keyboard); // -1 up, 1 down
    if (rep != 0) dir += (rep < 0 ? 1 : -1);

    uint32_t now = millis();
    int serverPct = light.percent();
    bool stepped = false;

    if (dir != 0) {
        if (!_brightnessChangedLocally) {
            _brightnessTarget = serverPct;
            _brightnessBaseline = serverPct;
        }
        // Snap onto the step grid in the direction of travel, so 47% moves to
        // 50 or 45 rather than 52 or 42.
        int next = dir > 0
            ? (_brightnessTarget / BRIGHTNESS_STEP + 1) * BRIGHTNESS_STEP
            : ((_brightnessTarget + BRIGHTNESS_STEP - 1) / BRIGHTNESS_STEP - 1) * BRIGHTNESS_STEP;
        next = constrain(next, 0, 100);
        if (next != _brightnessTarget) {
            _brightnessTarget = next;
            _brightnessChangedLocally = true;
            _brightnessPendingSend = true;
            stepped = true;
        }
    }

    // Same throttle and echo handling as the media volume below.
    bool keysHeld = keyboard.isUpHeld() || keyboard.isDownHeld() ||
                    keyboard.isPlusHeld() || keyboard.isMinusHeld();
    if (_brightnessPendingSend) {
        if (!keysHeld || now - _brightnessSentAt >= HELD_SEND_MS) {
            if (_onSetBrightness) _onSetBrightness(entity.id, _brightnessTarget);
            _brightnessPendingSend = false;
            _brightnessSentAt = now;
        }
    } else if (_brightnessChangedLocally && !keysHeld) {
        // Within 1% counts as agreement: 0-255 does not map evenly onto 0-100.
        uint32_t sinceSend = now - _brightnessSentAt;
        bool agrees = abs(serverPct - _brightnessTarget) <= 1;
        bool moved = serverPct != _brightnessBaseline;
        if (agrees || (moved && sinceSend > ECHO_SETTLE_MS) || sinceSend > 3000) {
            _brightnessChangedLocally = false;
        }
    }

    return stepped;
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
            _seekPendingSend = false;
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
            _seekPendingSend = true;
            handled = true;
        }

        // Send the scrub position while Left/Right is still held, at a slower
        // rate than volume, and once more the moment the key comes up.
        bool seekKeysHeld = keyboard.isLeftHeld() || keyboard.isRightHeld();
        if (_seekPendingSend) {
            if (!seekKeysHeld || now - _seekSentAt >= HELD_SEEK_SEND_MS) {
                if (_onSeekMedia) {
                    _onSeekMedia(entity.id, _seekTarget);
                }
                _seekPendingSend = false;
                _seekSentAt = now;
            }
        } else if (_seekChangedLocally && !seekKeysHeld) {
            // Same reasoning as the climate setpoint: dropping the scrub target
            // at send time let the bar snap back to the pre-seek position for
            // the length of the round trip. Hold it until the reported position
            // lands near it, allowing for playback drift while we wait. Not
            // while the key is still down: the echo of a mid-gesture send would
            // end the hold, and the next step would rebase on that old position.
            MediaPlayerState msSeek;
            bool landed = _entityManager.getMediaPlayerState(_entityId, msSeek) &&
                          fabsf(msSeek.position - _seekTarget) < 5.0f;
            if (landed || now - _seekSentAt > 4000) {
                _seekChangedLocally = false;
            }
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
                _volumeBaseline = _volumeTarget;
            }
            _volumeTarget = constrain(_volumeTarget + volumeDelta, 0.0f, 1.0f);
            _volumeChangedLocally = true;
            _volumePendingSend = true;
            handled = true;
        }

        // While Up/Down or +/- is held, send the running level every
        // HELD_SEND_MS so the player follows the gesture, and send the final
        // level as soon as the key comes up. A gesture's first step goes out
        // at once, since nothing has been sent for longer than the interval.
        bool volumeKeysHeld = keyboard.isUpHeld() || keyboard.isDownHeld() ||
                              keyboard.isPlusHeld() || keyboard.isMinusHeld();
        if (_volumePendingSend) {
            if (!volumeKeysHeld || now - _volumeSentAt >= HELD_SEND_MS) {
                if (_onSetVolume) {
                    _onSetVolume(entity.id, _volumeTarget);
                }
                _volumePendingSend = false;
                _volumeSentAt = now;
            }
        } else if (_volumeChangedLocally && !volumeKeysHeld) {
            // Hold our own level until Home Assistant echoes one back. Accept any
            // movement away from where the level started, not just an exact
            // match: players that quantize volume land near the request, so
            // waiting for equality would stall until the timeout. Mid-gesture
            // sends echo too, so movement only counts once those have had
            // ECHO_SETTLE_MS to land after the final send.
            MediaPlayerState msVol;
            uint32_t sinceSend = now - _volumeSentAt;
            if (_entityManager.getMediaPlayerState(_entityId, msVol)) {
                bool agrees = fabsf(msVol.volumeLevel - _volumeTarget) < 0.02f;
                bool moved = fabsf(msVol.volumeLevel - _volumeBaseline) > 0.001f;
                if (agrees || (moved && sinceSend > ECHO_SETTLE_MS) || sinceSend > 3000) {
                    _volumeChangedLocally = false;
                }
            } else if (sinceSend > 3000) {
                _volumeChangedLocally = false;
            }
        }
    }

    if (entity.domain == "light" && handleBrightnessInput(keyboard, entity)) {
        handled = true;
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
