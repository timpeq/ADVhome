#ifndef CONFIG_VIEW_H
#define CONFIG_VIEW_H

#include "View.h"
#include "ConfigManager.h"
#include "DiagnosticView.h"
#include "ScrollRepeater.h"
#include <vector>

class ConfigView : public View {
public:
    ConfigView(ConfigManager& config, DiagnosticView& diagnosticView);
    
    void onEnter() override;
    void draw(DisplayManager& display) override;
    bool handleInput(KeyboardManager& keyboard) override;

private:
    ConfigManager& _config;
    DiagnosticView& _diagnosticView;
    
    struct Setting {
        String name;
        int type; // 0 = bool, 1 = int (interval), 2 = int (enum)
    };
    
    std::vector<Setting> _settings;
    int _selectedIndex = 0;
    
    // Temporary variables for drawing
    bool _showBattery;
    bool _showDiagnostics = false;
    int _reconInt;
    int _scrollDelay;
    int _scrollSpeed;
    int _seekStep;
    ScrollRepeater _scrollRepeater;
    
    void refreshValues();
    void toggleCurrent();
};

#endif // CONFIG_VIEW_H
