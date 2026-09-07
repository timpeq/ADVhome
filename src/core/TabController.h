#ifndef TAB_CONTROLLER_H
#define TAB_CONTROLLER_H

#include "View.h"
#include <vector>

class TabController {
public:
    void addView(View* view, const String& name);
    
    void update(KeyboardManager& keyboard, DisplayManager& display, bool forceRedraw = false, bool showBattery = false);
    
    void nextTab();
    void prevTab();
    void drawTabBar(DisplayManager& display, bool showBattery);

private:
    struct TabInfo {
        View* view;
        String name;
    };
    
    std::vector<TabInfo> _tabs;
    int _currentTabIndex = 0;
    
};

#endif // TAB_CONTROLLER_H
