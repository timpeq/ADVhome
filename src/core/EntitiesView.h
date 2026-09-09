#ifndef ENTITIES_VIEW_H
#define ENTITIES_VIEW_H

#include "View.h"
#include "EntityList.h"
#include <functional>

class EntitiesView : public View {
public:
    EntitiesView(EntityManager& entityManager, ConfigManager& config, std::function<void(String)> onEntitySelect, std::function<void(String)> onEntityToggle = nullptr, std::function<void(String, int)> onEntityAdjust = nullptr);

    void onEnter() override;
    void draw(DisplayManager& display) override;
    bool handleInput(KeyboardManager& keyboard) override;

private:
    EntityManager& _entityManager;
    ConfigManager& _config;
    std::function<void(String)> _onEntitySelect;
    std::function<void(String)> _onEntityToggle;
    std::function<void(String, int)> _onEntityAdjust;

    EntityList _list;
    bool _subTabFocus = false;

    // Every domain the firmware understands, in sub-tab order. Domains with no
    // entities are skipped by _visibleTabs so the carousel only shows what this
    // Home Assistant actually has.
    static const char* const kDomains[];
    static const int kDomainCount;
    std::vector<uint8_t> _visibleTabs;
    int _currentTab = 0; // index into _visibleTabs
    size_t _tabsBuiltFor = (size_t)-1;

    String currentDomain() const;
    void rebuildVisibleTabs();
    void refreshCache();
    void stepTab(int direction);
};

#endif // ENTITIES_VIEW_H
