#ifndef ENTITY_LIST_H
#define ENTITY_LIST_H

#include "EntityManager.h"
#include "ConfigManager.h"
#include "ScrollRepeater.h"
#include "KeyboardManager.h"
#include <vector>

// Shared scrolling entity list used by both the Home favourites widget and the
// Entities browser. Owns selection, scrolling, type-ahead search and row
// rendering so the two views only differ in how they fill the item vector and
// what they do when a row is activated.
//
// Items are borrowed pointers into EntityManager's map. That map is only ever
// inserted into (never erased), so nodes -- and therefore these pointers --
// stay valid for the life of the connection.
class EntityList {
public:
    // What handleInput() observed this frame. The owning view performs the
    // action so each view can keep its own callbacks.
    struct Input {
        bool changed = false;    // something moved; the view needs a redraw
        bool hitTop = false;     // Up pressed while already on the first row
        int adjust = 0;          // -1 / +1 from the +/- keys
        bool favToggle = false;  // Ctrl-F
    };

    EntityList(EntityManager& entityManager, ConfigManager& config);

    std::vector<const Entity*> items;

    void setTopY(int topY) { _topY = topY; }
    int rowsPerPage() const { return (135 - _topY) / kRowHeight; }
    int selectedIndex() const { return _selectedIndex; }
    const Entity* current() const;

    // Keeps the selection inside the item vector and visible on screen. Call
    // after refilling items.
    void clampSelection();

    // showStar draws the favourite marker column (Entities browser only).
    void draw(M5Canvas& canvas, bool showStar);

    Input handleInput(KeyboardManager& keyboard);

private:
    static constexpr int kRowHeight = 15;

    // Domain-aware right-hand column: what's playing, the thermostat's
    // ambient/target pair, or the raw state as a fallback.
    String secondaryText(const Entity& entity, uint16_t& color) const;
    void drawScrollbar(M5Canvas& canvas);
    void scrollToSelection();

    EntityManager& _entityManager;
    ConfigManager& _config;
    int _topY = 25;
    int _selectedIndex = 0;
    int _scrollOffset = 0;
    ScrollRepeater _scrollRepeater;
    ScrollRepeater _valueRepeater;
    String _searchPrefix;
    uint32_t _lastSearchTime = 0;
};

#endif // ENTITY_LIST_H
