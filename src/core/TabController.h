#ifndef TAB_CONTROLLER_H
#define TAB_CONTROLLER_H

#include "View.h"
#include <vector>

class TabController {
public:
    void addView(View* view, const String& name);
    void setViewVisible(View* view, bool visible);
    
    void update(KeyboardManager& keyboard, DisplayManager& display, bool forceRedraw = false, bool showBattery = false);
    
    void nextTab();
    void prevTab();
    void showView(View* view); // jump straight to a tab (no-op if hidden/not found/current)
    void drawTabBar(DisplayManager& display, bool showBattery);
    void drawActiveView(DisplayManager& display);

private:
    struct TabInfo {
        View* view;
        String name;
        bool visible;
    };
    
    std::vector<TabInfo> _tabs;
    int _currentTabIndex = 0;
    
};

#endif // TAB_CONTROLLER_H
