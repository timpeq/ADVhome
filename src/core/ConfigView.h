#ifndef CONFIG_VIEW_H
#define CONFIG_VIEW_H

#include "View.h"
#include "ConfigManager.h"
#include <vector>

class ConfigView : public View {
public:
    ConfigView(ConfigManager& config);
    
    void onEnter() override;
    void draw(DisplayManager& display) override;
    bool handleInput(KeyboardManager& keyboard) override;

private:
    ConfigManager& _config;
    
    struct Setting {
        String name;
        int type; // 0 = bool, 1 = int (interval), 2 = int (enum)
    };
    
    std::vector<Setting> _settings;
    int _selectedIndex = 0;
    
    // Temporary variables for drawing
    bool _showBattery;
    int _reconInt;
    int _backStyle;
    int _scrollStyle;
    
    void refreshValues();
    void toggleCurrent();
};

#endif // CONFIG_VIEW_H
