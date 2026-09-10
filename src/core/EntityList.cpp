#include "EntityList.h"
#include "TextScroller.h"
#include "Format.h"

namespace {

uint16_t stateColor(const String& state) {
    if (state == "on" || state == "playing" || state == "open" ||
        state == "unlocked" || state == "home") {
        return TFT_GREEN;
    }
    if (state == "off" || state == "closed" || state == "locked" ||
        state == "idle" || state == "standby" || state == "paused") {
        return TFT_RED;
    }
    if (state == "unavailable" || state == "unknown") return TFT_DARKGREY;
    return TFT_CYAN;
}

} // namespace

EntityList::EntityList(EntityManager& entityManager, ConfigManager& config)
    : _entityManager(entityManager), _config(config),
      _scrollRepeater(config), _valueRepeater(config) {}

const Entity* EntityList::current() const {
    if (_selectedIndex < 0 || _selectedIndex >= (int)items.size()) return nullptr;
    return items[_selectedIndex];
}

void EntityList::clampSelection() {
    if (items.empty()) {
        _selectedIndex = 0;
        _scrollOffset = 0;
        return;
    }
    if (_selectedIndex >= (int)items.size()) _selectedIndex = items.size() - 1;
    if (_selectedIndex < 0) _selectedIndex = 0;
    scrollToSelection();
}

void EntityList::scrollToSelection() {
    int rows = rowsPerPage();
    if (rows < 1) rows = 1;
    if (_selectedIndex < _scrollOffset) _scrollOffset = _selectedIndex;
    else if (_selectedIndex >= _scrollOffset + rows) _scrollOffset = _selectedIndex - rows + 1;

    int maxOffset = (int)items.size() - rows;
    if (maxOffset < 0) maxOffset = 0;
    if (_scrollOffset > maxOffset) _scrollOffset = maxOffset;
    if (_scrollOffset < 0) _scrollOffset = 0;
}

String EntityList::secondaryText(const Entity& entity, uint16_t& color) const {
    if (entity.domain == "media_player") {
        MediaPlayerState media;
        if ((entity.state == "playing" || entity.state == "paused") &&
            _entityManager.getMediaPlayerState(entity.id, media) && !media.title.isEmpty()) {
            color = entity.state == "playing" ? TFT_GREEN : TFT_LIGHTGREY;
            return (entity.state == "playing" ? ">" : "||") + media.title;
        }
    } else if (entity.domain == "climate") {
        ClimateState climate;
        if (_entityManager.getClimateState(entity.id, climate)) {
            color = entity.state == "heat"   ? TFT_ORANGE
                  : entity.state == "cool"   ? TFT_CYAN
                  : entity.state == "off"    ? TFT_DARKGREY
                                             : TFT_GREEN;
            if (climate.hasRange()) {
                return Format::temperature(climate.targetTempLow) + "-" + Format::temperature(climate.targetTempHigh);
            }
            if (climate.hasTarget()) {
                return Format::temperature(climate.currentTemperature) + ">" + Format::temperature(climate.targetTemperature);
            }
            return Format::temperature(climate.currentTemperature);
        }
    } else if (entity.domain == "scene") {
        // Scenes store their last-activated timestamp as state; keep it compact.
        color = TFT_CYAN;
        return "Scene";
    }

    color = stateColor(entity.state);
    String text = entity.state;
    text.replace("_", " ");
    return text;
}

void EntityList::drawScrollbar(M5Canvas& canvas) {
    int rows = rowsPerPage();
    int total = items.size();
    if (rows < 1 || total <= rows) return;

    int trackHeight = rows * kRowHeight;
    int barHeight = trackHeight * rows / total;
    if (barHeight < 6) barHeight = 6;
    int barY = _topY - 2 + (trackHeight - barHeight) * _scrollOffset / (total - rows);

    canvas.fillRect(236, _topY - 2, 3, trackHeight, 0x2124);
    canvas.fillRect(236, barY, 3, barHeight, TFT_DARKCYAN);
}

void EntityList::draw(M5Canvas& canvas, bool showStar) {
    int rows = rowsPerPage();
    int nameX = showStar ? 17 : 5;

    for (int i = 0; i < rows; i++) {
        int idx = _scrollOffset + i;
        if (idx >= (int)items.size()) break;

        const Entity& entity = *items[idx];
        int rowY = _topY + (i * kRowHeight);
        bool isSelected = idx == _selectedIndex;

        if (isSelected) canvas.fillRect(0, rowY - 2, 240, kRowHeight,
                                        (_reorderable && _ctrlHeld) ? TFT_DARKCYAN : TFT_BLUE);

        uint16_t valueColor;
        String value = secondaryText(entity, valueColor);
        value = TextScroller::visible(value, 20, isSelected);

        // Give the name whatever the right-hand column does not need, so long
        // states ("heat cool") and track titles both stay on one row.
        int valueX = 233 - (int)value.length() * 6;
        int nameChars = (valueX - nameX - 6) / 6;
        if (nameChars < 6) nameChars = 6;

        canvas.setTextSize(1);

        if (showStar) {
            canvas.setCursor(5, rowY);
            if (_config.isFavorite(entity.id)) {
                canvas.setTextColor(TFT_YELLOW);
                canvas.print("*");
            }
        }

        canvas.setTextColor(isSelected ? TFT_WHITE : TFT_LIGHTGREY);
        canvas.setCursor(nameX, rowY);
        canvas.print(TextScroller::visible(entity.friendlyName, nameChars, isSelected));

        canvas.setTextColor(valueColor);
        canvas.setCursor(valueX, rowY);
        canvas.print(value);
    }

    drawScrollbar(canvas);
}

void EntityList::moveSelection(int direction) {
    int target = _selectedIndex + direction;
    if (target < 0 || target >= (int)items.size()) return;
    _selectedIndex = target;
    scrollToSelection();
}

EntityList::Input EntityList::handleInput(KeyboardManager& keyboard) {
    Input result;
    if (items.empty()) return result;

    int rows = rowsPerPage();
    if (rows < 1) rows = 1;

    _ctrlHeld = keyboard.isCtrlHeld();

    int direction = _scrollRepeater.update(keyboard);

    // Carry the row rather than moving the highlight past it. The view performs
    // the swap and then calls moveSelection() so the highlight follows the row
    // it is holding.
    if (direction != 0 && _reorderable && _ctrlHeld) {
        int target = _selectedIndex + direction;
        if (target >= 0 && target < (int)items.size()) {
            result.reorder = direction;
            result.changed = true;
        }
        return result;
    }

    if (direction < 0) {
        if (_selectedIndex == 0) {
            result.hitTop = true;
        } else {
            _selectedIndex--;
            scrollToSelection();
        }
        result.changed = true;
    } else if (direction > 0) {
        if (_selectedIndex < (int)items.size() - 1) {
            _selectedIndex++;
            scrollToSelection();
        }
        result.changed = true;
    } else {
        if (_config.getListAdjustEnabled()) {
            result.adjust = _valueRepeater.updatePlusMinus(keyboard);
            if (result.adjust != 0) result.changed = true;
        }
    }

    for (char c : keyboard.getNewChars()) {
        if ((c == 'f' || c == 'F') && keyboard.isCtrlHeld()) {
            result.favToggle = true;
            result.changed = true;
        } else if (isAlphaNumeric(c)) {
            // Type-ahead: consecutive keys within a second extend the prefix.
            if (millis() - _lastSearchTime > 1000) _searchPrefix = "";
            _searchPrefix += c;
            _lastSearchTime = millis();

            for (size_t i = 0; i < items.size(); i++) {
                if (items[i]->friendlyName.substring(0, _searchPrefix.length())
                        .equalsIgnoreCase(_searchPrefix)) {
                    _selectedIndex = i;
                    scrollToSelection();
                    result.changed = true;
                    break;
                }
            }
        }
    }

    return result;
}
